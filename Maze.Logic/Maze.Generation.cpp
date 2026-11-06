#include "Maze.Generation.h"
#include "Maze.Validation.h"

#include <random>
#include <charconv>
#include <stdexcept>
#include <vector>

MazeGrid generateMaze(const MazeSize& size, std::uint32_t seed)
{
    MazeGrid maze(size);
    std::mt19937 random(seed);
    std::vector<MazePosition> path;
    path.reserve(static_cast<std::size_t>((size.rows + 1) / 2)
        * static_cast<std::size_t>((size.cols + 1) / 2));
    path.push_back({ 0, 0 });
    maze.at(0, 0) = MazeCell::Passage;

    // Even row/column positions are rooms; a one-cell wall separates them.
    constexpr MazePosition directions[] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 } };
    while (!path.empty())
    {
        const auto current = path.back();
        MazePosition candidates[4];
        int count = 0;
        for (const auto direction : directions)
        {
            const MazePosition next{ current.row + direction.row, current.col + direction.col };
            if (maze.contains(next) && maze.at(next.row, next.col) == MazeCell::Wall)
                candidates[count++] = next;
        }

        if (count == 0)
        {
            path.pop_back();
            continue;
        }
        std::uniform_int_distribution<int> choose(0, count - 1);
        const auto next = candidates[choose(random)];
        maze.at((current.row + next.row) / 2, (current.col + next.col) / 2) = MazeCell::Passage;
        maze.at(next.row, next.col) = MazeCell::Passage;
        path.push_back(next);
    }

    // For even dimensions the last row/column lies beyond the room lattice.
    // Extend its already-connected bottom-right room by at most two cells.
    const int lastRow = size.rows - 1;
    const int lastCol = size.cols - 1;
    const int roomRow = lastRow - lastRow % 2;
    const int roomCol = lastCol - lastCol % 2;
    for (int row = roomRow; row <= lastRow; ++row)
        maze.at(row, roomCol) = MazeCell::Passage;
    for (int col = roomCol; col <= lastCol; ++col)
        maze.at(lastRow, col) = MazeCell::Passage;
    maze.at(lastRow, lastCol) = MazeCell::Exit;
    return maze;
}

MazeGrid generateMaze(const MazeSize& size)
{
    if (!isValidMazeSize(size))
        throw std::invalid_argument("Maze dimensions must each be between 10 and 50.");
    return generateMaze(size, generateMazeSeed());
}

std::uint32_t generateMazeSeed()
{
    std::random_device entropy;
    return static_cast<std::uint32_t>(entropy());
}

bool parseMazeSeed(std::string_view text, std::optional<std::uint32_t>& seed) noexcept
{
    if (text.empty())
    {
        seed.reset();
        return true;
    }
    if (text.size() > 10) return false;

    std::uint32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    seed = value;
    return true;
}
