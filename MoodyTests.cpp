#include "MoodyTests.h"

#include <thread>
#include <concurrentqueue.h>
#include <vector>

#include "TestCommon.h"

using namespace moodycamel;

namespace camel_tests {
    void onWriter(const int writerCount,
                  const int dataValue,
                  ConcurrentQueue<message> &queue){
        for (int i = 0; i < writerCount; i++)
        {
            message items[10];
            for (size_t j = 0; j < 10; ++j, ++i)
            {
                auto& item = items[j];

                item.m_type = i % 5 == 0 ? WorkType::Sub : WorkType::Add;
                for (int k = 0; k < maxWork; ++k)
                {
                    item.m_data[k] = (i + k) % 60;
                }
            }

            queue.enqueue_bulk(items, 10);
        }

        message msg;
        auto& item = msg;
        item.m_type = WorkType::TermWork;
        item.m_data[0] = dataValue;

        queue.enqueue(item);
    }

    void onReader(ConcurrentQueue<message> &queue,
                  uint64_t &result) {
        bool exit1 = false;
        bool exit2 = false;
        uint64_t sum = 0;

        do
        {
            message messages[10];
            while(!queue.try_dequeue_bulk(messages, 10)) {
            }

            for(auto j = 0; j < 10; j++)
            {
                auto message = messages[j];
                if (message.m_type == WorkType::TermWork)
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
                else if (message.m_type == WorkType::Add)
                {
                    for (int i = 0; i < maxWork; ++i)
                    {
                        sum += message.m_data[i];
                    }
                }
                else if (message.m_type == WorkType::Sub)
                {
                    for (int i = 0; i < maxWork; ++i)
                    {
                        sum -= message.m_data[i];
                    }
                }
            }
        } while (!exit1 || !exit2);
        result = sum;
    }

    void OnConcurrentQueueTests(const int itemCount, const int readerCount, const int writerCount){
        uint64_t result;

        auto start = std::chrono::high_resolution_clock::now();
        ConcurrentQueue<message> queue;

        auto readers = std::vector<std::thread>{};

        for(auto rdr = 0; rdr < readerCount; ++rdr){
            readers.emplace_back(std::thread(onReader,
                                               std::ref(queue),
                                               std::ref(result)));
        }

        auto writers = std::vector<std::thread>{};

        for(auto wrt = 0; wrt < writerCount; ++wrt) {
            writers.emplace_back(std::thread(onWriter,
                                            itemCount,
                                            1,
                                            std::ref(queue)));
        }

        for(auto &t : readers) {
            t.join();
        }

        for(auto &t : writers) {
            t.join();
        }

        DisplayTiming(start, result, itemCount);
    }
}