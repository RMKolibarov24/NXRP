#include "MazeSize.h"

MazeSize::MazeSize(int rowCount, int colCount) noexcept
    : rows(rowCount), cols(colCount)
{
}

bool MazeSize::isValid() const noexcept
{
    return rows >= MIN_MAZE_SIZE && rows <= MAX_MAZE_SIZE
        && cols >= MIN_MAZE_SIZE && cols <= MAX_MAZE_SIZE;
}
