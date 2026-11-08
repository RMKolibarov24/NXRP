#pragma once

#include "MazeGrid.h"
#include "GameStatistics.h"
#include "MazePosition.h"

#include <cstdint>
#include <optional>

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
    MazeSize selectedSize{ DEFAULT_MAZE_SIZE, DEFAULT_MAZE_SIZE };
    MazePosition player;
    GamePhase phase = GamePhase::Menu;
    GameStatistics statistics;
    std::optional<std::uint32_t> selectedSeed; // Empty means a random next game.
    std::optional<std::uint32_t> mazeSeed;     // Seed used by the current generated grid.

    bool hasActiveGame() const noexcept
    {
        return !maze.empty();
    }
};
