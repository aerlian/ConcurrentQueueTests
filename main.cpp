#include <iostream>

#include <ring_buffer.hpp>
#include <single_threaded_claim_strategy.hpp>
#include <spin_wait_strategy.hpp>
#include <sequence_barrier.hpp>

#include <vector>
#include <algorithm>
#include <thread>
#include <cassert>
#include <fmt/core.h>

using namespace disruptorplus;

struct message
{
    uint32_t m_type;
    uint8_t m_data[28];
};

void onWriter(const int writer1Count,
              const int writerBatchSize,
              const int dataValue,
              single_threaded_claim_strategy<spin_wait_strategy> &claimStrategy,
              ring_buffer<message> &buffer){
    for (int i = 0; i < writer1Count;)
    {
        sequence_range range = claimStrategy.claim(std::min(writerBatchSize, writer1Count - i));
        for (size_t j = 0; j < range.size(); ++j, ++i)
        {
            auto& item = buffer[range[j]];

            item.m_type = i % 5 == 0 ? 0xdec : 0xadd;
            for (int k = 0; k < 28; ++k)
            {
                item.m_data[k] = (i + k) % 60;
            }
        }
        claimStrategy.publish(range.first());
    }

    sequence_t seq = claimStrategy.claim_one();
    auto& item = buffer[seq];
    item.m_type = 0xdead;
    item.m_data[0] = dataValue;
    claimStrategy.publish(seq);
}

void onReader(std::vector<size_t> &readerBatchSizes,
            single_threaded_claim_strategy<spin_wait_strategy> &claimStrategy,
            ring_buffer<message> &buffer,
            sequence_barrier<spin_wait_strategy> &finishedReading,
            uint64_t &result) {
    bool exit1 = false;
    bool exit2 = false;
    uint64_t sum = 0;
    sequence_t nextToRead = 0;
    while (!exit1 || !exit2)
    {
        sequence_t available = claimStrategy.wait_until_published(nextToRead, nextToRead - 1);
        assert(difference(available, nextToRead) >= 0);
        ++readerBatchSizes[difference(available, nextToRead)];
        do
        {
            auto& message = buffer[nextToRead];
            if (message.m_type == 0xdead)
            {
                if (message.m_data[0] == 1)
                {
                    exit1 = true;
                    exit2 = true;
                }
                else if (message.m_data[0] == 2)
                {
                    exit2 = true;
                }
            }
            else if (message.m_type == 0xadd)
            {
                for (int i = 0; i < 28; ++i)
                {
                    sum += message.m_data[i];
                }
            }
            else if (message.m_type == 0xdec)
            {
                for (int i = 0; i < 28; ++i)
                {
                    sum -= message.m_data[i];
                }
            }
        } while (nextToRead++ != available);
        finishedReading.publish(available);
    }
    result = sum;
}

int main(int argc, char* argv[])
{
    const int itemCount = 12'000'000;
    const size_t bufferSize = size_t(1) << 20;
    const int writerBatchSize = 1;
    const int writer1Count = itemCount;

    spin_wait_strategy waitStrategy;
    single_threaded_claim_strategy<spin_wait_strategy> claimStrategy{bufferSize, waitStrategy};
    sequence_barrier<spin_wait_strategy> finishedReading(waitStrategy);
    claimStrategy.add_claim_barrier(finishedReading);

    ring_buffer<message> buffer(bufferSize);

    std::vector<size_t> readerBatchSizes(bufferSize, 0);

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t result;
    std::thread reader(onReader,
                       std::ref(readerBatchSizes),
                       std::ref(claimStrategy),
                       std::ref(buffer),
                       std::ref(finishedReading),
                       std::ref(result));

    std::thread writer1(onWriter,
                        writer1Count,
                        writerBatchSize,
                        1,
                        std::ref(claimStrategy),
                        std::ref(buffer));
    std::thread writer2(onWriter,
                        writer1Count,
                        writerBatchSize,
                        2,
                        std::ref(claimStrategy),
                        std::ref(buffer));
    reader.join();
    writer1.join();
    writer2.join();

    auto totalItemCount = itemCount + 2;

    auto end = std::chrono::high_resolution_clock::now();
    auto dur = (end - start);
    auto durMS = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    auto durNS = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
    auto nsPerItem = durNS / totalItemCount;
    std::cout << result << "\n"
              << durMS.count() << "ms total time\n"
              << nsPerItem.count() << "ns per item (avg)\n"
              << (1000000000 / nsPerItem.count()) << " items per second (avg)\n"
              << std::flush;

    std::vector<std::pair<size_t, size_t>> sortedBatchSizes;
    for (size_t i = 0; i < readerBatchSizes.size(); ++i)
    {
        if (readerBatchSizes[i] != 0)
        {
            sortedBatchSizes.push_back(std::make_pair(readerBatchSizes[i] * (i + 1), (i + 1)));
        }
    }
    std::sort(sortedBatchSizes.rbegin(), sortedBatchSizes.rend());

    std::cout << "Reader batch sizes:\n";
    for (size_t i = 0; i < 20 && i < sortedBatchSizes.size(); ++i)
    {
        size_t batchSize = sortedBatchSizes[i].second;
        size_t batchItemCount = sortedBatchSizes[i].first;
        size_t percentage = (100 * batchItemCount) / itemCount;
        std::cout << "#" << (i + 1) << ": " << batchSize
                  << " item batch, " << percentage << "%, "
                  << (sortedBatchSizes[i].first / sortedBatchSizes[i].second) << " times\n";
    }
    std::cout << std::flush;

    return 0;
}