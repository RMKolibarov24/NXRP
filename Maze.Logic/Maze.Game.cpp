#include "Maze.Game.h"

#include <stdexcept>
#include <limits>
#include <utility>

namespace
{
    bool isWalkable(const MazeGrid& maze, MazePosition position)
    {
        if (!maze.contains(position)) return false;
        const auto cell = maze.at(position.row, position.col);
        return cell == MazeCell::Passage || cell == MazeCell::Exit;
    }

    void validatePreparedMaze(const MazeGrid& maze)
    {
        if (maze.empty())
            throw std::invalid_argument("Starting a game requires a non-empty maze.");

        const auto& size = maze.size();
        if (maze.at(0, 0) != MazeCell::Passage)
            throw std::invalid_argument("The top-left cell must be a passage.");
        if (maze.at(size.rows - 1, size.cols - 1) != MazeCell::Exit)
            throw std::invalid_argument("The bottom-right cell must be the exit.");

        for (int row = 0; row < size.rows; ++row)
        {
            for (int col = 0; col < size.cols; ++col)
            {
                const auto cell = maze.at(row, col);
                if (cell != MazeCell::Wall && cell != MazeCell::Passage && cell != MazeCell::Exit)
                    throw std::invalid_argument("The maze contains an unknown cell value.");
                if (cell == MazeCell::Exit && (row != size.rows - 1 || col != size.cols - 1))
                    throw std::invalid_argument("Only the bottom-right cell can be an exit.");
            }
        }
    }
}

MazeGrid createDemoMaze()
{
    constexpr char layout[10][11] = {
        "....######",
        "###.######",
        "#...######",
        "#.########",
        "#.......##",
        "#######.##",
        "###.....##",
        "###.######",
        "###.......",
        "#########."
    };

    MazeGrid maze(MazeSize(10, 10));
    for (int row = 0; row < 10; ++row)
        for (int col = 0; col < 10; ++col)
            maze.at(row, col) = layout[row][col] == '#' ? MazeCell::Wall : MazeCell::Passage;
    maze.at(9, 9) = MazeCell::Exit;
    return maze;
}

void startGame(GameState& game, MazeGrid&& preparedMaze, std::optional<std::uint32_t> seed)
{
    validatePreparedMaze(preparedMaze);
    const MazeSize size = preparedMaze.size();
    game.maze = std::move(preparedMaze);
    game.mazeSeed = seed;
    game.selectedSize = size;
    game.player = { 0, 0 };
    game.phase = GamePhase::Playing;
    game.statistics = {};
}

bool restartGame(GameState& game) noexcept
{
    if (game.maze.empty() || (game.phase != GamePhase::Playing
        && game.phase != GamePhase::Paused && game.phase != GamePhase::Won))
        return false;

    game.player = { 0, 0 };
    game.phase = GamePhase::Playing;
    game.statistics = {};
    return true;
}

void endGame(GameState& game) noexcept
{
    game.maze = MazeGrid();
    game.mazeSeed.reset();
    game.player = { 0, 0 };
    game.phase = GamePhase::Menu;
    game.statistics = {};
}

void advanceGameTime(GameState& game, std::chrono::steady_clock::duration elapsed) noexcept
{
    using Duration = std::chrono::steady_clock::duration;
    if (game.phase != GamePhase::Playing || game.maze.empty() || elapsed <= Duration::zero()) return;

    const auto remaining = Duration::max() - game.statistics.elapsed;
    game.statistics.elapsed += elapsed < remaining ? elapsed : remaining;
}

bool hasWon(const GameState& game)
{
    if (!game.maze.contains(game.player)) return false;
    const auto& size = game.maze.size();
    return game.player.row == size.rows - 1 && game.player.col == size.cols - 1
        && game.maze.at(game.player.row, game.player.col) == MazeCell::Exit;
}

MoveResult tryMove(GameState& game, MoveDirection direction)
{
    if (game.phase != GamePhase::Playing || !isWalkable(game.maze, game.player) || hasWon(game))
        return MoveResult::Blocked;

    // The current position is in bounds, so a one-cell step cannot overflow.
    MazePosition next = game.player;
    switch (direction)
    {
    case MoveDirection::Up:    --next.row; break;
    case MoveDirection::Down:  ++next.row; break;
    case MoveDirection::Left:  --next.col; break;
    case MoveDirection::Right: ++next.col; break;
    default: return MoveResult::Blocked;
    }

    if (!isWalkable(game.maze, next)) return MoveResult::Blocked;
    game.player = next;
    if (game.statistics.successfulMoves < std::numeric_limits<std::uint64_t>::max())
        ++game.statistics.successfulMoves;
    if (hasWon(game))
    {
        game.phase = GamePhase::Won;
        return MoveResult::Won;
    }
    return MoveResult::Moved;
}
