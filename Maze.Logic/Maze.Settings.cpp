#include "Maze.Settings.h"
#include "Maze.Validation.h"

#include <fstream>
#include <system_error>

MazeSize loadMazeSizePreferences(const std::filesystem::path& filePath)
{
    const MazeSize defaults(MIN_MAZE_SIZE, MIN_MAZE_SIZE);
    if (filePath.empty()) return defaults;

    std::ifstream file(filePath, std::ios::binary);
    int rows = 0, cols = 0;
    if (!(file >> rows >> cols)) return defaults;

    // Allow whitespace after the two numbers, but reject partial or extra data.
    file >> std::ws;
    if (!file.eof() || file.bad()) return defaults;

    const MazeSize size(rows, cols);
    return isValidMazeSize(size) ? size : defaults;
}

bool saveMazeSizePreferences(const std::filesystem::path& filePath, const MazeSize& size)
{
    if (filePath.empty() || !isValidMazeSize(size)) return false;

    std::error_code error;
    if (!filePath.parent_path().empty())
    {
        std::filesystem::create_directories(filePath.parent_path(), error);
        if (error) return false;
    }

    auto temporaryPath = filePath;
    temporaryPath += ".tmp";
    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file) return false;
        file << size.rows << ' ' << size.cols << '\n';
        file.close();
        if (!file)
        {
            std::filesystem::remove(temporaryPath, error);
            return false;
        }
    }

    // Keep the previous preferences until a complete replacement is ready.
    std::filesystem::rename(temporaryPath, filePath, error);
    if (error)
    {
        std::error_code ignored;
        std::filesystem::remove(temporaryPath, ignored);
        return false;
    }
    return true;
}
