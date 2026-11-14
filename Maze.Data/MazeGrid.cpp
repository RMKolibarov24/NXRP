#include "MazeGrid.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

MazeGrid::MazeGrid() noexcept
    : size_(0, 0)
{
}

MazeGrid::MazeGrid(const MazeSize& size)
    : size_(size)
{
    if (!size_.isValid())
    {
        throw std::invalid_argument("Maze rows and columns must each be between 10 and 100.");
    }

    cells_ = std::make_unique<MazeCell[]>(cellCount());
    fill(MazeCell::Wall);
}

MazeGrid::MazeGrid(MazeGrid&& other) noexcept
    : size_(other.size_), cells_(std::move(other.cells_))
{
    other.size_ = MazeSize(0, 0);
}

MazeGrid& MazeGrid::operator=(MazeGrid&& other) noexcept
{
    if (this != &other)
    {
        cells_ = std::move(other.cells_);
        size_ = other.size_;
        other.size_ = MazeSize(0, 0);
    }
    return *this;
}

const MazeSize& MazeGrid::size() const noexcept
{
    return size_;
}

std::size_t MazeGrid::cellCount() const noexcept
{
    return static_cast<std::size_t>(size_.rows) * static_cast<std::size_t>(size_.cols);
}

bool MazeGrid::empty() const noexcept
{
    return cells_ == nullptr;
}

bool MazeGrid::contains(MazePosition position) const noexcept
{
    return !empty()
        && position.row >= 0 && position.row < size_.rows
        && position.col >= 0 && position.col < size_.cols;
}

std::size_t MazeGrid::checkedIndex(int row, int col) const
{
    if (!contains({ row, col }))
    {
        throw std::out_of_range("Maze cell position is outside the grid.");
    }
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(size_.cols)
        + static_cast<std::size_t>(col);
}

MazeCell& MazeGrid::at(int row, int col)
{
    return cells_[checkedIndex(row, col)];
}

const MazeCell& MazeGrid::at(int row, int col) const
{
    return cells_[checkedIndex(row, col)];
}

void MazeGrid::fill(MazeCell cell) noexcept
{
    if (!empty())
    {
        std::fill_n(cells_.get(), cellCount(), cell);
    }
}
