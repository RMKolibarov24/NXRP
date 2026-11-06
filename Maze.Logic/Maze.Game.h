#pragma once

#include "GameState.h"

enum class MoveDirection
{
    Up,
    Down,
    Left,
    Right
};

enum class MoveResult
{
    Blocked,
    Moved,
    Won
};

// A fixed 10x10 maze for development. This is not the random generator.
MazeGrid createDemoMaze();

// Takes ownership of a prepared grid and starts at (0,0) in Playing phase.
// Resets elapsed time and successful moves after validation succeeds.
// The start must be a passage; the only exit must be at the bottom-right.
// Empty grids, unknown cell values and invalid layouts throw invalid_argument
// before changing either the game or preparedMaze. Reachability is not checked.
// Supply the seed actually used for a generated grid; fixtures may omit it.
void startGame(GameState& game, MazeGrid&& preparedMaze,
    std::optional<std::uint32_t> seed = std::nullopt);

// Restarts a Playing, Paused or Won session at (0,0) without replacing its grid.
// Uses the existing layout already validated by startGame.
// Resets elapsed time and successful moves.
// Preserves the next-game size. Returns false without changes if no session exists.
bool restartGame(GameState& game) noexcept;

// Releases maze data and resets the player/phase/statistics, preserving the next-game size.
// Safe when already empty; performs no allocation.
void endGame(GameState& game) noexcept;

// Adds positive time only to a nonempty Playing session. Does not read a clock.
// Elapsed time saturates instead of overflowing.
void advanceGameTime(GameState& game, std::chrono::steady_clock::duration elapsed) noexcept;

// True only when the player occupies the bottom-right cell marked Exit.
bool hasWon(const GameState& game);

// Only Playing games can move. A blocked request changes no game data.
// Every successful step counts once, including the winning step.
// Reaching the exit returns Won and changes the phase to Won.
MoveResult tryMove(GameState& game, MoveDirection direction);
