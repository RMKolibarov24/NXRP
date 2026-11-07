#include "Maze.Hunter.h"

#include "Maze.Safehouse.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

namespace
{
    bool same(MazePosition a, MazePosition b) { return a.row == b.row && a.col == b.col; }
    bool open(const MazeGrid& maze, MazePosition p)
    {
        return maze.contains(p) && maze.at(p.row, p.col) != MazeCell::Wall;
    }
    bool hunterOpen(const MazeGrid& maze, MazePosition p, const std::vector<MazePosition>& safehouses)
    {
        (void)safehouses;
        return open(maze, p);
    }
    constexpr MazePosition directions[] = { { -1,0 }, { 1,0 }, { 0,-1 }, { 0,1 } };

    bool withinView(const HunterState& hunter, MazePosition player, int distanceSquared)
    {
        // Nearby movement is hard to miss. Before the first step, the creature
        // has no meaningful facing direction, so give it all-around awareness.
        if (distanceSquared <= 2 || hunter.previous.row < 0 || hunter.previous.col < 0) return true;
        const int facingRow = hunter.cell.row - hunter.previous.row;
        const int facingCol = hunter.cell.col - hunter.previous.col;
        if (facingRow == 0 && facingCol == 0) return true;
        const int dot = facingRow * (player.row - hunter.cell.row) + facingCol * (player.col - hunter.cell.col);
        return static_cast<float>(dot) >= -.25f * std::sqrt(static_cast<float>(distanceSquared));
    }

    bool soundReaches(const MazeGrid& maze, MazePosition from, MazePosition to, int budget)
    {
        // A passage costs one unit and crossing a hedge costs three. This lets a
        // loud runner be heard around corners or through one hedge, but prevents
        // multiple walls from transmitting a perfect signal across the maze.
        const int cols = maze.size().cols;
        const int start = from.row * cols + from.col, goal = to.row * cols + to.col;
        std::vector<int> cost(maze.cellCount(), std::numeric_limits<int>::max());
        using Entry = std::pair<int, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
        cost[start] = 0; pending.push({0, start});
        while (!pending.empty())
        {
            const auto [currentCost, index] = pending.top(); pending.pop();
            if (currentCost != cost[index]) continue;
            if (index == goal) return true;
            const MazePosition current{ index / cols, index % cols };
            for (const auto d : directions)
            {
                const MazePosition next{ current.row + d.row, current.col + d.col };
                if (!maze.contains(next)) continue;
                const int nextCost = currentCost + (maze.at(next.row, next.col) == MazeCell::Wall ? 3 : 1);
                const int nextIndex = next.row * cols + next.col;
                if (nextCost <= budget && nextCost < cost[nextIndex])
                { cost[nextIndex] = nextCost; pending.push({nextCost, nextIndex}); }
            }
        }
        return false;
    }

    bool visible(const MazeGrid& maze, MazePosition from, MazePosition to)
    {
        int x = from.col, y = from.row;
        const int dx = std::abs(to.col - x), dy = std::abs(to.row - y);
        const int sx = x < to.col ? 1 : -1, sy = y < to.row ? 1 : -1;
        int error = dx - dy;
        while (x != to.col || y != to.row)
        {
            const int previousX = x, previousY = y, doubled = 2 * error;
            if (doubled > -dy) { error -= dy; x += sx; }
            if (doubled < dx) { error += dx; y += sy; }
            if (!open(maze, { y, x })) return false;
            // A ray cannot cut diagonally through the corner of a hedge.
            if (x != previousX && y != previousY &&
                (!open(maze, { previousY, x }) || !open(maze, { y, previousX }))) return false;
        }
        return true;
    }

    MazePosition nextToward(const MazeGrid& maze, MazePosition from, MazePosition target,
        const std::vector<MazePosition>& safehouses)
    {
        const auto size = maze.size();
        std::vector<int> parent(maze.cellCount(), -1);
        std::queue<MazePosition> pending;
        const int start = from.row * size.cols + from.col;
        const int goal = target.row * size.cols + target.col;
        parent[start] = start; pending.push(from);
        while (!pending.empty() && parent[goal] < 0)
        {
            const auto current = pending.front(); pending.pop();
            for (const auto d : directions)
            {
                const MazePosition next{ current.row + d.row, current.col + d.col };
                if (!hunterOpen(maze, next, safehouses) || !refugeEdgeAllowed(current, next, safehouses)) continue;
                const int index = next.row * size.cols + next.col;
                if (parent[index] >= 0) continue;
                parent[index] = current.row * size.cols + current.col; pending.push(next);
            }
        }
        if (parent[goal] < 0 || start == goal) return from;
        int step = goal;
        while (parent[step] != start) step = parent[step];
        return { step / size.cols, step % size.cols };
    }
}

HunterState makeHunter(const MazeGrid& maze, std::uint32_t seed, std::uint32_t encounter,
    const std::array<MazePosition, 3>& stones, MazePosition temporaryOpening,
    const std::vector<MazePosition>& safehouses)
{
    HunterState hunter;
    if (maze.empty()) return hunter;
    const auto size = maze.size();
    std::vector<MazePosition> candidates;
    const int entranceDistance = std::max(4, (size.rows + size.cols) / 4);
    const int gateDistance = std::max(3, (size.rows + size.cols) / 8);
    for (int row = 0; row < size.rows; ++row) for (int col = 0; col < size.cols; ++col)
    {
        const MazePosition cell{row, col};
        if (maze.at(row, col) != MazeCell::Passage || same(cell, temporaryOpening) || safehouseAt(cell, safehouses) >= 0) continue;
        bool isStone = false;
        for (const auto stone : stones) if (same(cell, stone)) isStone = true;
        if (isStone) continue;
        if (row + col >= entranceDistance && (size.rows - 1 - row) + (size.cols - 1 - col) >= gateDistance)
            candidates.push_back(cell);
    }
    std::uint32_t random = seed ^ 0xb5297a4du ^ (encounter * 0x9e3779b9u);
    random ^= random >> 16; random *= 0x7feb352du; random ^= random >> 15;
    hunter.cell = candidates.empty() ? MazePosition{ size.rows - 1, size.cols - 1 }
        : candidates[random % candidates.size()];
    hunter.random = random ? random : 1;
    return hunter;
}

namespace
{
void advanceHunterTargets(const MazeGrid& maze, HunterState& hunter, const HunterTarget* targets,
    std::size_t targetCount, const std::vector<MazePosition>& safehouses)
{
    if (!hunterOpen(maze, hunter.cell, safehouses)) return;
    int bestSignal = 0, bestDistance = std::numeric_limits<int>::max();
    MazePosition sensed{};
    for (std::size_t i = 0; i < targetCount; ++i)
    {
        const auto& target = targets[i];
        if (!hunterOpen(maze, target.cell, safehouses)) continue;
        const int dr = hunter.cell.row - target.cell.row, dc = hunter.cell.col - target.cell.col;
        const int distance = dr * dr + dc * dc;
        const bool seen = target.noise == 0 && distance <= (target.crouched ? 16 : 64) && withinView(hunter, target.cell, distance)
            && visible(maze, hunter.cell, target.cell);
        const int hearingBudget = std::max(target.noise, target.running ? 6 : target.crouched ? 0 : 2) + static_cast<int>(hunter.vigilance / 3u);
        const bool heard = !seen && (target.moving || target.noise > 0) && hearingBudget > 0 && distance <= hearingBudget * hearingBudget
            && soundReaches(maze, hunter.cell, target.cell, hearingBudget);
        const int signal = seen ? 2 : heard ? 1 : 0;
        if (signal > bestSignal || (signal > 0 && signal == bestSignal && distance < bestDistance))
        { bestSignal = signal; bestDistance = distance; sensed = target.cell; }
    }
    if (bestSignal > 0)
    {
        hunter.lastKnown = sensed;
        hunter.vigilance = std::min(9u, hunter.vigilance + 1u);
        hunter.memory = 8 + static_cast<int>(hunter.vigilance);
        // Noise attracts a search; only a sighting confirms a chase.
        hunter.mode = bestSignal == 2 ? HunterMode::Chase : HunterMode::Search;
    }
    else if (hunter.memory > 0 && !same(hunter.cell, hunter.lastKnown))
    {
        --hunter.memory;
        hunter.mode = HunterMode::Search;
    }
    else
    {
        hunter.memory = 0;
        hunter.mode = HunterMode::Patrol;
    }

    MazePosition next = hunter.cell;
    if (hunter.mode != HunterMode::Patrol && hunterOpen(maze, hunter.lastKnown, safehouses))
        next = nextToward(maze, hunter.cell, hunter.lastKnown, safehouses);
    else
    {
        MazePosition options[4]; int count = 0;
        for (const auto d : directions)
        {
            const MazePosition p{ hunter.cell.row + d.row, hunter.cell.col + d.col };
            if (hunterOpen(maze, p, safehouses) && refugeEdgeAllowed(hunter.cell, p, safehouses) && !same(p, hunter.previous)) options[count++] = p;
        }
        if (count == 0 && hunterOpen(maze, hunter.previous, safehouses) && refugeEdgeAllowed(hunter.cell, hunter.previous, safehouses)) options[count++] = hunter.previous;
        if (count > 0)
        {
            hunter.random ^= hunter.random << 13;
            hunter.random ^= hunter.random >> 17;
            hunter.random ^= hunter.random << 5;
            next = options[hunter.random % static_cast<unsigned>(count)];
        }
    }
    hunter.previous = hunter.cell;
    hunter.cell = next;
}
}

void advanceHunter(const MazeGrid& maze, HunterState& hunter, MazePosition player, bool playerMoving, bool playerRunning)
{
    const HunterTarget target{ player, playerMoving, playerRunning };
    advanceHunterTargets(maze, hunter, &target, 1, {});
}

void advanceHunterCoop(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget& first, const HunterTarget& second)
{
    const HunterTarget targets[]{ first, second };
    advanceHunterTargets(maze, hunter, targets, 2, {});
}

void advanceHunterGroup(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget* targets, std::size_t count, const std::vector<MazePosition>& safehouses)
{
    if (!targets && count != 0) return;
    advanceHunterTargets(maze, hunter, targets, count, safehouses);
}

bool relocateDistantHunter(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget* targets, std::size_t count, const std::array<MazePosition, 3>& stones,
    const std::vector<MazePosition>& safehouses, std::uint32_t seed, std::uint32_t event)
{
    if (maze.empty() || !targets || count == 0) return false;
    bool validTarget = false;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (!open(maze, targets[i].cell)) continue;
        validTarget = true;
        const int dr = hunter.cell.row - targets[i].cell.row, dc = hunter.cell.col - targets[i].cell.col;
        if (dr * dr + dc * dc < 22 * 22) return false;
    }
    if (!validTarget) return false;
    std::vector<MazePosition> candidates;
    const auto size = maze.size();
    for (int row = 0; row < size.rows; ++row) for (int col = 0; col < size.cols; ++col)
    {
        const MazePosition cell{ row, col };
        if (maze.at(row, col) != MazeCell::Passage || safehouseAt(cell, safehouses) >= 0) continue;
        bool stone = false; for (const auto item : stones) if (same(cell, item)) stone = true;
        if (stone) continue;
        int nearest = std::numeric_limits<int>::max();
        for (std::size_t i = 0; i < count; ++i)
        {
            if (!open(maze, targets[i].cell)) continue;
            const int dr = row - targets[i].cell.row, dc = col - targets[i].cell.col;
            nearest = std::min(nearest, dr * dr + dc * dc);
        }
        if (nearest >= 14 * 14 && nearest <= 22 * 22) candidates.push_back(cell);
    }
    if (candidates.empty()) return false;
    std::uint32_t value = seed ^ (event * 0x9e3779b9u) ^ 0x63d83595u;
    value ^= value >> 16; value *= 0x7feb352du; value ^= value >> 15;
    hunter.cell = candidates[value % candidates.size()];
    hunter.previous = {-1, -1}; hunter.mode = HunterMode::Patrol; hunter.memory = 0;
    hunter.random = value ? value : 1;
    return true;
}
