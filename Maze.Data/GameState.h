#pragma once

#include "MazeGrid.h"
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

    bool hasActiveGame() const noexcept
    {
        return !maze.empty();
    }
};
