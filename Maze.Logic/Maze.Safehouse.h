#pragma once

#include "MazePosition.h"
#include "MazeSize.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

// The 3x3 refuge is passable for players and forbidden to the hunter.
inline bool insideSafehouse(MazePosition cell, MazePosition center) noexcept
{
    return std::abs(cell.row - center.row) <= 1 && std::abs(cell.col - center.col) <= 1;
}

inline int safehouseAt(MazePosition cell, const std::vector<MazePosition>& houses) noexcept
{
    for (std::size_t i = 0; i < houses.size(); ++i)
        if (insideSafehouse(cell, houses[i])) return static_cast<int>(i);
    return -1;
}

// Independent of the maze grid so hosts and guests can derive the same refuges
// from its dimensions and seed. Each clearing stays inside the outer boundary.
inline std::vector<MazePosition> generateSafehouses(const MazeSize& size, std::uint32_t seed)
{
    if (!size.isValid()) return {};
    std::vector<MazePosition> candidates;
    for (int row = 2; row <= size.rows - 3; row += 2)
        for (int col = 2; col <= size.cols - 3; col += 2)
            if (row + col >= 6 && (size.rows - 1 - row) + (size.cols - 1 - col) >= 6)
                candidates.push_back({ row, col });
    std::mt19937 random(seed ^ 0x85ebca6bu);
    std::shuffle(candidates.begin(), candidates.end(), random);
    const int smallerSide = std::min(size.rows, size.cols);
    const std::size_t wanted = smallerSide >= 60 ? 4u : smallerSide >= 25 ? 3u : 2u;
    const int separation = std::max(4, smallerSide / 4);
    std::vector<MazePosition> best;
    for (std::size_t start = 0; start < std::min<std::size_t>(16, candidates.size()); ++start)
    {
        std::vector<MazePosition> chosen{ candidates[start] };
        for (std::size_t step = 1; step < candidates.size() && chosen.size() < wanted; ++step)
        {
            const auto candidate = candidates[(start + step) % candidates.size()];
            bool separate = true;
            for (const auto other : chosen)
                if (std::max(std::abs(candidate.row - other.row), std::abs(candidate.col - other.col)) < separation)
                { separate = false; break; }
            if (separate) chosen.push_back(candidate);
        }
        if (chosen.size() > best.size()) best = std::move(chosen);
        if (best.size() == wanted) break;
    }
    return best;
}
