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
// The start must be a passage; the only exit must be at the bottom-right.
// Empty grids, unknown cell values and invalid layouts throw invalid_argument
// before changing either the game or preparedMaze. Reachability is not checked.
void startGame(GameState& game, MazeGrid&& preparedMaze);

// True only when the player occupies the bottom-right cell marked Exit.
bool hasWon(const GameState& game);

// Only Playing games can move. A blocked request changes no game data.
// Reaching the exit returns Won and changes the phase to Won.
MoveResult tryMove(GameState& game, MoveDirection direction);
