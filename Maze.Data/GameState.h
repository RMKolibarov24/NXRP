#pragma once

#include "MazeGrid.h"
#include "GameStatistics.h"
#include "MazePosition.h"

enum class GamePhase
{
    Menu,
    Playing,
    Paused,
    Won
};

struct GameState
{
    MazeGrid maze;
    MazeSize selectedSize{ MIN_MAZE_SIZE, MIN_MAZE_SIZE };
    MazePosition player;
    GamePhase phase = GamePhase::Menu;
    GameStatistics statistics;

    bool hasActiveGame() const noexcept
    {
        return !maze.empty();
    }
};
