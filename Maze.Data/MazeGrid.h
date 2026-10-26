#pragma once

#include <cstddef>
#include <memory>

#include "MazeCell.h"
#include "MazePosition.h"
#include "MazeSize.h"

class MazeGrid
{
public:
    // An empty grid represents the absence of maze data and owns no memory.
    MazeGrid() noexcept;

    // Throws std::invalid_argument unless each dimension is within 10..50.
    // All cells initially contain walls. Allocation failures propagate.
    explicit MazeGrid(const MazeSize& size);
    ~MazeGrid() = default;

    MazeGrid(const MazeGrid&) = delete;
    MazeGrid& operator=(const MazeGrid&) = delete;
    MazeGrid(MazeGrid&& other) noexcept;
    MazeGrid& operator=(MazeGrid&& other) noexcept;

    const MazeSize& size() const noexcept;
    std::size_t cellCount() const noexcept;
    bool empty() const noexcept;
    bool contains(MazePosition position) const noexcept;

    // Both overloads throw std::out_of_range for invalid or empty-grid access.
    MazeCell& at(int row, int col);
    const MazeCell& at(int row, int col) const;
    void fill(MazeCell cell) noexcept;

private:
    std::size_t checkedIndex(int row, int col) const;

    MazeSize size_;
    // A contiguous dynamic array; a cell's index is row * cols + col.
    // unique_ptr releases the array with delete[] when the grid is destroyed.
    std::unique_ptr<MazeCell[]> cells_;
};
