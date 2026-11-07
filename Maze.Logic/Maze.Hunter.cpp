#include "Maze.Hunter.h"

#include "Maze.Safehouse.h"
#include "Maze.Acoustics.h"
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

    bool visible(const MazeGrid& maze, MazePosition from, MazePosition to, const std::vector<MazePosition>& houses={}, unsigned closedDoors=0)
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
            if (!open(maze, { y, x }) || acousticBoundaryCost({previousY,previousX},{y,x},houses,closedDoors)>1) return false;
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

int senseHunterTarget(const MazeGrid& maze,const HunterState& hunter,
    const HunterTarget& target,const std::vector<MazePosition>& safehouses,unsigned closedDoors)
{
    if(!open(maze,hunter.cell) || !open(maze,target.cell))return 0;
    const int dr=hunter.cell.row-target.cell.row,dc=hunter.cell.col-target.cell.col;
    const int distance=dr*dr+dc*dc;
    const bool otherFloor=std::abs(hunter.elevation-target.elevation)>1.f;
    if(!target.audibleOnly && !otherFloor && distance<=(target.crouched?16:64)
        && withinView(hunter,target.cell,distance) && visible(maze,hunter.cell,target.cell,safehouses,closedDoors))return 2;
    const int budget=std::max(target.noise,target.running?6:target.crouched?0:2)
        +static_cast<int>(hunter.vigilance/3u)-(otherFloor?3:0);
    return (target.moving || target.noise>0) && budget>0 && distance<=budget*budget
        && acousticReachable(maze,hunter.cell,target.cell,budget,safehouses,closedDoors)?1:0;
}

namespace
{
unsigned hunterRandom(HunterState& hunter)
{
    if(!hunter.random)hunter.random=1;
    hunter.random^=hunter.random<<13;hunter.random^=hunter.random>>17;hunter.random^=hunter.random<<5;
    return hunter.random;
}
void rememberObservedStep(const MazeGrid& maze,HunterState& hunter,MazePosition exit,
    const std::vector<MazePosition>& houses)
{
    const auto junction=hunter.observed;
    if(std::abs(junction.row-exit.row)+std::abs(junction.col-exit.col)!=1)return;
    int branches=0;
    for(const auto d:directions)
    {
        const MazePosition p{junction.row+d.row,junction.col+d.col};
        branches+=open(maze,p) && refugeEdgeAllowed(junction,p,houses);
    }
    if(branches<3)return;
    HunterRoute* replace=&hunter.routes[0];
    for(auto& route:hunter.routes)
    {
        if(same(route.junction,junction) && same(route.exit,exit))
        {route.confidence=std::min(5u,route.confidence+1);return;}
        if(route.confidence<replace->confidence)replace=&route;
    }
    *replace={junction,exit,1};
}
void rememberObservedExit(const MazeGrid& maze,HunterState& hunter,MazePosition exit,
    const std::vector<MazePosition>& houses)
{
    // Samples can span several cells while someone runs. Only infer straight,
    // unobstructed travel between consecutive sightings; never hidden turns.
    const auto start=hunter.observed;
    const int dr=exit.row-start.row,dc=exit.col-start.col;
    if((dr!=0 && dc!=0) || std::abs(dr)+std::abs(dc)>4 || !open(maze,start)
        || !visible(maze,start,exit,houses))return;
    const MazePosition direction{(dr>0)-(dr<0),(dc>0)-(dc<0)};
    while(!same(hunter.observed,exit))
    {
        const MazePosition next{hunter.observed.row+direction.row,hunter.observed.col+direction.col};
        rememberObservedStep(maze,hunter,next,houses);hunter.observed=next;
    }
}
MazePosition searchExit(const MazeGrid& maze,HunterState& hunter,const std::vector<MazePosition>& houses)
{
    MazePosition options[4];unsigned weights[4];int count=0;unsigned total=0;
    for(const auto d:directions)
    {
        const MazePosition p{hunter.cell.row+d.row,hunter.cell.col+d.col};
        if(!open(maze,p) || !refugeEdgeAllowed(hunter.cell,p,houses)
            || std::abs(p.row-hunter.lastKnown.row)+std::abs(p.col-hunter.lastKnown.col)>4)continue;
        unsigned weight=same(p,hunter.previous)?1u:3u;
        for(const auto& route:hunter.routes)
            if(same(route.junction,hunter.cell) && same(route.exit,p))weight+=route.confidence*6;
        options[count]=p;weights[count++]=weight;total+=weight;
    }
    if(!count)return hunter.cell;
    unsigned choice=hunterRandom(hunter)%total;
    for(int i=0;i<count;++i){if(choice<weights[i])return options[i];choice-=weights[i];}
    return options[0];
}
void advanceHunterTargets(const MazeGrid& maze, HunterState& hunter, const HunterTarget* targets,
    std::size_t targetCount, const std::vector<MazePosition>& safehouses, unsigned closedDoors=0)
{
    if (!hunterOpen(maze, hunter.cell, safehouses)) return;
    int bestSignal=0,bestDistance=std::numeric_limits<int>::max(),actor=-1;
    MazePosition sensed{};float sensedElevation=0.f;
    for(std::size_t i=0;i<targetCount;++i)
    {
        const auto& target=targets[i];
        const int signal=senseHunterTarget(maze,hunter,target,safehouses,closedDoors);
        const int dr=hunter.cell.row-target.cell.row,dc=hunter.cell.col-target.cell.col,distance=dr*dr+dc*dc;
        if(signal>bestSignal || (signal>0 && signal==bestSignal && distance<bestDistance))
        {bestSignal=signal;bestDistance=distance;sensed=target.cell;sensedElevation=target.elevation;actor=static_cast<int>(i);}
    }
    if(bestSignal>0)
    {
        if(bestSignal==2 && hunter.observedActor==actor)
            rememberObservedExit(maze,hunter,sensed,safehouses);
        hunter.observed=bestSignal==2?sensed:MazePosition{-1,-1};
        hunter.observedActor=bestSignal==2?actor:-1;
        hunter.lastKnown=sensed;hunter.lastKnownElevation=sensedElevation;
        hunter.vigilance=std::min(9u,hunter.vigilance+1u);
        hunter.memory=10+static_cast<int>(hunter.vigilance);
        hunter.searchGoal=sensed;hunter.searchingArea=false;hunter.linger=0;
        hunter.mode=bestSignal==2?HunterMode::Chase:HunterMode::Search;
    }
    else
    {
        hunter.observedActor=-1;hunter.observed={-1,-1};
        if(hunter.memory>0)
        {
            --hunter.memory;hunter.mode=HunterMode::Search;
            if(!hunter.searchingArea && same(hunter.cell,hunter.lastKnown))
            {hunter.searchingArea=true;hunter.linger=2;}
            if(hunter.searchingArea && hunter.linger==0)
                hunter.searchGoal=searchExit(maze,hunter,safehouses);
        }
        else {hunter.mode=HunterMode::Patrol;hunter.searchingArea=false;hunter.linger=0;}
    }

    MazePosition next=hunter.cell;
    if(hunter.mode!=HunterMode::Patrol && hunterOpen(maze,hunter.searchGoal,safehouses))
    {
        if(hunter.linger>0)--hunter.linger;
        else next=nextToward(maze,hunter.cell,hunter.searchGoal,safehouses);
    }
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
            next = options[hunterRandom(hunter) % static_cast<unsigned>(count)];
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
    const HunterTarget* targets, std::size_t count, const std::vector<MazePosition>& safehouses,unsigned closedDoors)
{
    if (!targets && count != 0) return;
    advanceHunterTargets(maze, hunter, targets, count, safehouses,closedDoors);
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
    hunter.elevation=0.f;hunter.observed={-1,-1};hunter.observedActor=-1;
    hunter.searchingArea=false;hunter.linger=0;
    hunter.random = value ? value : 1;
    return true;
}
