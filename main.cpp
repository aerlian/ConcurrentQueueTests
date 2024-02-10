#include <iostream>
#include <cassert>
#include <chrono>

#include "TestCommon.h"

#include "MoodyTests.h"
#include "DisruptorTests.h"

void DisplayTiming(const std::chrono::time_point<std::chrono::high_resolution_clock> &start, uint64_t result, int totalItemCount) {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = (end - start);
    auto durMS = std::__1::chrono::duration_cast<std::chrono::milliseconds>(dur);
    auto durNS = std::__1::chrono::duration_cast<std::chrono::nanoseconds>(dur);
    auto nsPerItem = durNS / totalItemCount;
    std::cout << "result:" << result << "\n"
              << durMS.count() << "ms total time\n"
              << nsPerItem.count() << "ns per item (avg)\n"
              << std::flush;
}

int main(int argc, char* argv[])
{
    const int itemCount = 50'000'000;
    disruptor_tests::OnDisruptorTests(itemCount, 1, 1);
    camel_tests::OnConcurrentQueueTests(itemCount, 1, 1);

    return 0;
}