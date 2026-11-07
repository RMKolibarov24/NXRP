#pragma once

#include "MazeGrid.h"
#include <array>
#include <cstdint>

enum class GateRule { Forward, Reverse, Advance, Rotate, Complement };
enum class StonePuzzleKind { Sequence, Balance };
enum class StoneTouchResult { Invalid, Reset, Changed, Solved, AlreadySolved };

struct MazePuzzle
{
    std::array<MazePosition, 3> stones{};
    std::array<int, 3> order{};
    std::array<int, 3> numbers{};
    GateRule gateRule = GateRule::Forward;
    StonePuzzleKind stoneKind = StonePuzzleKind::Sequence;
    int balanceTarget = 0;
};

// Applies the gate's inscription to the ordered stone numbers.
std::array<int, 3> gateCode(const MazePuzzle& puzzle);

// The balance puzzle toggles stones; the target has exactly one subset answer.
int stoneBalance(const MazePuzzle& puzzle, unsigned activeMask) noexcept;

// Mutates a shared sequence count and a three-bit active-stone mask. A solved
// puzzle stays solved; invalid input leaves both values unchanged.
StoneTouchResult touchStone(const MazePuzzle& puzzle, int stone, int& sequence, unsigned& activeMask) noexcept;

// Places three distinct stones in passages reachable before the exit is opened.
// The same grid and seed reproduce placement, puzzle type, answers and gate rule.
// Throws invalid_argument if there are fewer than three eligible passages.
MazePuzzle generateMazePuzzle(const MazeGrid& maze, std::uint32_t seed);
