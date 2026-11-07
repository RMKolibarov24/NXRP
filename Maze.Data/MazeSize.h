#pragma once

constexpr int MIN_MAZE_SIZE = 10;
constexpr int MAX_MAZE_SIZE = 100;
constexpr int DEFAULT_MAZE_SIZE = 30;

class MazeSize
{
public:
    int rows, cols;

    // A size value can be invalid; allocating a grid requires a valid size.
    MazeSize(int rowCount, int colCount) noexcept;
    bool isValid() const noexcept;
};
