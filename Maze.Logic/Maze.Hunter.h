#pragma once

#include "MazeGrid.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class HunterMode { Patrol, Search, Chase };

struct HunterRoute
{
    MazePosition junction{-1,-1},exit{-1,-1};
    unsigned confidence=0;
};

struct HunterState
{
    MazePosition cell{};
    MazePosition previous{ -1, -1 };
    MazePosition lastKnown{};
    HunterMode mode = HunterMode::Patrol;
    int memory = 0;
    unsigned vigilance = 0;
    std::uint32_t random = 1;
    float elevation=0.f,lastKnownElevation=0.f;
    MazePosition searchGoal{-1,-1}, observed{-1,-1};
    int observedActor=-1,linger=0;
    bool searchingArea=false;
    std::array<HunterRoute,24> routes{};
};

struct HunterTarget
{
    MazePosition cell{};
    bool moving = false;
    bool running = false;
    int noise = 0;
    bool crouched = false;
    bool audibleOnly = false;
    float elevation = 0.f;
};

// 2 = an actual sighting, 1 = audible evidence, 0 = no information.
// Shared by grid pursuit and continuous movement inside two-storey refuges.
int senseHunterTarget(const MazeGrid& maze,const HunterState& hunter,
    const HunterTarget& target,const std::vector<MazePosition>& safehouses={},
    unsigned closedDoors=0);

// Select a reproducible patrol start away from the entrance, gate, stones and
// a temporary opening. The encounter number varies the start after a capture.
HunterState makeHunter(const MazeGrid& maze, std::uint32_t seed, std::uint32_t encounter,
    const std::array<MazePosition, 3>& stones, MazePosition temporaryOpening,
    const std::vector<MazePosition>& safehouses = {});

// Move one passage at a time. The hunter watches ahead, hears nearby movement
// through passages and hedges with attenuation, and investigates the last known
// position; otherwise it patrols seeded routes. Sight triggers a chase.
// Inputs must be walkable cells in the current maze.
void advanceHunter(const MazeGrid& maze, HunterState& hunter, MazePosition player,
    bool playerMoving, bool playerRunning);

// In co-op, compare both players' actual sight and sound signals before moving.
// A sighting takes priority over noise; equally strong signals use distance.
void advanceHunterCoop(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget& first, const HunterTarget& second);

// Examines every player before selecting the strongest visible or audible target.
void advanceHunterGroup(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget* targets, std::size_t count,
    const std::vector<MazePosition>& safehouses = {}, unsigned closedDoors = 0);

// Reposition an unseen, distant hunter near the group on very large maps.
// Never selects a refuge, a stone, or a cell within 14 units of any player.
bool relocateDistantHunter(const MazeGrid& maze, HunterState& hunter,
    const HunterTarget* targets, std::size_t count, const std::array<MazePosition, 3>& stones,
    const std::vector<MazePosition>& safehouses, std::uint32_t seed, std::uint32_t event);
