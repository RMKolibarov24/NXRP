#include "Maze.Puzzle.h"

#include "Maze.Safehouse.h"
#include <algorithm>
#include <cstdlib>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

MazePuzzle generateMazePuzzle(const MazeGrid& maze, std::uint32_t seed)
{
    if (maze.empty() || maze.at(0, 0) != MazeCell::Passage)
        throw std::invalid_argument("Puzzle requires a maze with an open entrance.");
    const auto size = maze.size();
    const auto safehouses = generateSafehouses(size, seed);
    std::vector<bool> visited(maze.cellCount(), false);
    std::queue<MazePosition> pending;
    std::vector<MazePosition> candidates;
    pending.push({ 0, 0 }); visited[0] = true;
    constexpr MazePosition directions[] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    while (!pending.empty())
    {
        const auto current = pending.front(); pending.pop();
        if (current.row + current.col > 3 && safehouseAt(current, safehouses) < 0) candidates.push_back(current);
        for (const auto d : directions)
        {
            const MazePosition next{ current.row + d.row, current.col + d.col };
            if (!maze.contains(next) || maze.at(next.row, next.col) != MazeCell::Passage) continue;
            const auto index = static_cast<std::size_t>(next.row * size.cols + next.col);
            if (visited[index]) continue;
            visited[index] = true; pending.push(next);
        }
    }
    if (candidates.size() < 3) throw std::invalid_argument("Not enough reachable passages for the stones.");
    std::mt19937 random(seed ^ 0x98fcb321u);
    std::shuffle(candidates.begin(), candidates.end(), random);
    MazePuzzle puzzle;
    std::vector<MazePosition> chosen;
    // Prefer separated stones, then fill any remaining slots from the shuffled list.
    for (const auto p : candidates)
    {
        bool separated = true;
        for (const auto q : chosen)
            if (std::abs(p.row - q.row) + std::abs(p.col - q.col) < 4) separated = false;
        if (separated) chosen.push_back(p);
        if (chosen.size() == puzzle.stones.size()) break;
    }
    for (const auto p : candidates)
    {
        if (chosen.size() == puzzle.stones.size()) break;
        bool alreadyChosen = false;
        for (const auto q : chosen) if (p.row == q.row && p.col == q.col) alreadyChosen = true;
        if (!alreadyChosen) chosen.push_back(p);
    }
    std::copy(chosen.begin(), chosen.end(), puzzle.stones.begin());
    puzzle.order = { 0, 1, 2 };
    std::shuffle(puzzle.order.begin(), puzzle.order.end(), random);
    std::array<int, 9> digits{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::shuffle(digits.begin(), digits.end(), random);
    std::copy_n(digits.begin(), puzzle.numbers.size(), puzzle.numbers.begin());
    puzzle.gateRule = static_cast<GateRule>(random() % 5u);
    puzzle.stoneKind = (seed & 1u) ? StonePuzzleKind::Balance : StonePuzzleKind::Sequence;
    if (puzzle.stoneKind == StonePuzzleKind::Balance)
    {
        // Exactly two stones form the answer. Exclude the one possible
        // ambiguity where their sum is also the remaining stone's number.
        std::array<unsigned, 3> valid{}; std::size_t count = 0;
        for (unsigned mask : { 3u, 5u, 6u })
        {
            const int sum = stoneBalance(puzzle, mask);
            bool unique = true;
            for (int i = 0; i < 3; ++i)
                if (!(mask & (1u << i)) && puzzle.numbers[i] == sum) unique = false;
            if (unique) valid[count++] = mask;
        }
        std::mt19937 balanceRandom(seed ^ 0xd1b54a35u);
        puzzle.balanceTarget = stoneBalance(puzzle, valid[balanceRandom() % count]);
    }
    return puzzle;
}

int stoneBalance(const MazePuzzle& puzzle, unsigned activeMask) noexcept
{
    int sum = 0;
    for (int i = 0; i < 3; ++i) if (activeMask & (1u << i)) sum += puzzle.numbers[i];
    return sum;
}

StoneTouchResult touchStone(const MazePuzzle& puzzle, int stone, int& sequence, unsigned& activeMask) noexcept
{
    if (stone < 0 || stone >= 3 || sequence < 0 || sequence > 3 || activeMask > 7u) return StoneTouchResult::Invalid;
    if (sequence == 3) return StoneTouchResult::AlreadySolved;
    if (puzzle.stoneKind == StonePuzzleKind::Balance)
    {
        activeMask ^= 1u << stone;
        if (activeMask && stoneBalance(puzzle, activeMask) == puzzle.balanceTarget)
        { sequence = 3; return StoneTouchResult::Solved; }
        return StoneTouchResult::Changed;
    }
    if (puzzle.stoneKind != StonePuzzleKind::Sequence) return StoneTouchResult::Invalid;
    if (stone != puzzle.order[sequence]) { sequence = 0; activeMask = 0; return StoneTouchResult::Reset; }
    activeMask |= 1u << stone;
    ++sequence;
    return sequence == 3 ? StoneTouchResult::Solved : StoneTouchResult::Changed;
}

std::array<int, 3> gateCode(const MazePuzzle& puzzle)
{
    std::array<int, 3> answer{};
    for (int i = 0; i < 3; ++i)
    {
        const int orderIndex = puzzle.gateRule == GateRule::Reverse ? 2 - i
            : puzzle.gateRule == GateRule::Rotate ? (i + 1) % 3 : i;
        const int number = puzzle.numbers[puzzle.order[orderIndex]];
        answer[i] = puzzle.gateRule == GateRule::Advance ? number % 9 + 1
            : puzzle.gateRule == GateRule::Complement ? 10 - number : number;
    }
    return answer;
}
