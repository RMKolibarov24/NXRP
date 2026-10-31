#pragma once

#include <chrono>
#include <cstdint>

struct GameStatistics
{
    std::chrono::steady_clock::duration elapsed{};
    std::uint64_t successfulMoves = 0;
};
