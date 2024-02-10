#pragma once

#include<chrono>

constexpr int maxWork = 28;
enum class WorkType : uint32_t
{
    Add = 1,
    Sub = 2,
    TermWork = 0xdead,
};

struct message
{
    WorkType m_type;
    uint8_t m_data[maxWork];
};

void DisplayTiming(const std::chrono::time_point<std::chrono::high_resolution_clock> &start, uint64_t result, int totalItemCount);

