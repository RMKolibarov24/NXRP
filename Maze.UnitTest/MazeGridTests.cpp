#include "pch.h"
#include "CppUnitTest.h"
#include "MazeGrid.h"

#include <climits>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Owning a dynamic array must not allow accidental copies or throwing moves.
static_assert(!std::is_copy_constructible_v<MazeGrid>);
static_assert(!std::is_copy_assignable_v<MazeGrid>);
static_assert(std::is_nothrow_move_constructible_v<MazeGrid>);
static_assert(std::is_nothrow_move_assignable_v<MazeGrid>);

namespace MazeUnitTest
{
    namespace
    {
        void AssertEmpty(const MazeGrid& grid)
        {
            Assert::IsTrue(grid.empty());
            Assert::AreEqual(0, grid.size().rows);
            Assert::AreEqual(0, grid.size().cols);
            Assert::AreEqual(std::size_t{ 0 }, grid.cellCount());
            Assert::IsFalse(grid.contains({ 0, 0 }));
            Assert::ExpectException<std::out_of_range>([&grid] { (void)grid.at(0, 0); });
        }

        void AssertAllCells(const MazeGrid& grid, MazeCell expected)
        {
            for (int row = 0; row < grid.size().rows; ++row)
                for (int col = 0; col < grid.size().cols; ++col)
                    Assert::IsTrue(grid.at(row, col) == expected, L"A grid cell has an unexpected value.");
        }
    }

    TEST_CLASS(MazeGridTests)
    {
    public:
        TEST_METHOD(DefaultGridIsEmpty)
        {
            MazeGrid grid;
            AssertEmpty(grid);
            Assert::ExpectException<std::out_of_range>([&grid] { (void)grid.at(0, 0); });

            grid.fill(MazeCell::Exit);
            AssertEmpty(grid);
        }

        TEST_METHOD(MinimumGridStartsWithWalls)
        {
            const MazeGrid grid(MazeSize(10, 10));
            Assert::IsFalse(grid.empty());
            Assert::AreEqual(10, grid.size().rows);
            Assert::AreEqual(10, grid.size().cols);
            Assert::AreEqual(std::size_t{ 100 }, grid.cellCount());
            AssertAllCells(grid, MazeCell::Wall);
        }

        TEST_METHOD(MaximumGridStartsWithWalls)
        {
            const MazeGrid grid(MazeSize(MAX_MAZE_SIZE, MAX_MAZE_SIZE));
            Assert::IsFalse(grid.empty());
            Assert::AreEqual(MAX_MAZE_SIZE, grid.size().rows);
            Assert::AreEqual(MAX_MAZE_SIZE, grid.size().cols);
            Assert::AreEqual(std::size_t{ 10000 }, grid.cellCount());
            AssertAllCells(grid, MazeCell::Wall);
        }

        TEST_METHOD(RectangularGridsKeepTheirDimensions)
        {
            const MazeSize sizes[] = { { 10, 50 }, { 50, 10 } };
            for (const auto& size : sizes)
            {
                const MazeGrid grid(size);
                Assert::AreEqual(size.rows, grid.size().rows);
                Assert::AreEqual(size.cols, grid.size().cols);
                Assert::AreEqual(std::size_t{ 500 }, grid.cellCount());
                Assert::IsTrue(grid.contains({ size.rows - 1, size.cols - 1 }));
                Assert::IsFalse(grid.contains({ size.rows, size.cols - 1 }));
                Assert::IsFalse(grid.contains({ size.rows - 1, size.cols }));
                AssertAllCells(grid, MazeCell::Wall);
            }
        }

        TEST_METHOD(InvalidDimensionsThrow)
        {
            const MazeSize sizes[] = {
                { 9, 10 }, { 101, 10 }, { 10, 9 }, { 10, 101 },
                { 0, 10 }, { 10, 0 }, { -1, 10 }, { 10, -1 },
                { INT_MIN, 10 }, { 10, INT_MIN },
                { INT_MAX, 10 }, { 10, INT_MAX }
            };
            for (const auto& size : sizes)
            {
                Assert::ExpectException<std::invalid_argument>([size] {
                    const MazeGrid grid(size);
                    (void)grid;
                });
            }
        }

        TEST_METHOD(WritingCellsDoesNotAliasOtherCells)
        {
            MazeGrid grid(MazeSize(10, 50));
            grid.at(0, 10) = MazeCell::Passage;
            grid.at(1, 0) = MazeCell::Exit;
            grid.at(9, 49) = MazeCell::Passage;

            const MazeGrid& readOnly = grid;
            Assert::IsTrue(readOnly.at(0, 10) == MazeCell::Passage);
            Assert::IsTrue(readOnly.at(1, 0) == MazeCell::Exit);
            Assert::IsTrue(readOnly.at(9, 49) == MazeCell::Passage);
            Assert::IsTrue(readOnly.at(0, 9) == MazeCell::Wall);
            Assert::IsTrue(readOnly.at(0, 11) == MazeCell::Wall);
            Assert::IsTrue(readOnly.at(1, 1) == MazeCell::Wall);
            Assert::IsTrue(readOnly.at(9, 48) == MazeCell::Wall);
        }

        TEST_METHOD(ContainsChecksAllFourEdges)
        {
            const MazeGrid grid(MazeSize(10, 50));
            Assert::IsTrue(grid.contains({ 0, 0 }));
            Assert::IsTrue(grid.contains({ 0, 49 }));
            Assert::IsTrue(grid.contains({ 9, 0 }));
            Assert::IsTrue(grid.contains({ 9, 49 }));
            Assert::IsTrue(grid.contains({ 4, 25 }));
            Assert::IsFalse(grid.contains({ -1, 0 }));
            Assert::IsFalse(grid.contains({ 0, -1 }));
            Assert::IsFalse(grid.contains({ 10, 0 }));
            Assert::IsFalse(grid.contains({ 0, 50 }));
        }

        TEST_METHOD(AccessOutsideGridThrows)
        {
            MazeGrid grid(MazeSize(10, 50));
            const MazeGrid& readOnly = grid;
            const MazePosition positions[] = {
                { -1, 0 }, { 0, -1 }, { 10, 0 }, { 0, 50 },
                { 10, 50 }, { INT_MIN, 0 }, { 0, INT_MAX }
            };
            for (const auto position : positions)
            {
                Assert::ExpectException<std::out_of_range>([&grid, position] {
                    (void)grid.at(position.row, position.col);
                });
                Assert::ExpectException<std::out_of_range>([&readOnly, position] {
                    (void)readOnly.at(position.row, position.col);
                });
            }
        }

        TEST_METHOD(FillReplacesEveryCell)
        {
            MazeGrid grid(MazeSize(10, 50));
            grid.fill(MazeCell::Passage);
            AssertAllCells(grid, MazeCell::Passage);
            grid.at(9, 49) = MazeCell::Exit;
            grid.fill(MazeCell::Wall);
            AssertAllCells(grid, MazeCell::Wall);
            grid.fill(MazeCell::Exit);
            AssertAllCells(grid, MazeCell::Exit);
        }

        TEST_METHOD(MoveConstructionTransfersOwnership)
        {
            MazeGrid source(MazeSize(10, 50));
            source.at(0, 0) = MazeCell::Passage;
            source.at(9, 49) = MazeCell::Exit;
            const MazeCell* firstCell = &source.at(0, 0);

            const MazeGrid destination(std::move(source));
            Assert::IsFalse(destination.empty());
            Assert::AreEqual(10, destination.size().rows);
            Assert::AreEqual(50, destination.size().cols);
            Assert::AreEqual(std::size_t{ 500 }, destination.cellCount());
            Assert::IsTrue(firstCell == &destination.at(0, 0), L"A move must transfer the owned array.");
            Assert::IsTrue(destination.at(0, 0) == MazeCell::Passage);
            Assert::IsTrue(destination.at(9, 49) == MazeCell::Exit);
            AssertEmpty(source);
        }

        TEST_METHOD(MoveAssignmentReplacesExistingCells)
        {
            MazeGrid source(MazeSize(50, 50));
            source.at(0, 0) = MazeCell::Passage;
            source.at(49, 49) = MazeCell::Exit;
            const MazeCell* firstCell = &source.at(0, 0);
            MazeGrid destination(MazeSize(10, 10));
            destination.fill(MazeCell::Exit);

            destination = std::move(source);
            Assert::IsFalse(destination.empty());
            Assert::AreEqual(50, destination.size().rows);
            Assert::AreEqual(50, destination.size().cols);
            Assert::AreEqual(std::size_t{ 2500 }, destination.cellCount());
            Assert::IsTrue(firstCell == &destination.at(0, 0));
            Assert::IsTrue(destination.at(0, 0) == MazeCell::Passage);
            Assert::IsTrue(destination.at(49, 49) == MazeCell::Exit);
            Assert::IsTrue(destination.at(9, 9) == MazeCell::Wall);
            AssertEmpty(source);
        }

        TEST_METHOD(MovingEmptyGridClearsDestination)
        {
            MazeGrid source;
            MazeGrid destination(MazeSize(10, 10));
            destination = std::move(source);
            AssertEmpty(source);
            AssertEmpty(destination);
        }

        TEST_METHOD(MovedFromGridCanBeReused)
        {
            MazeGrid source(MazeSize(10, 10));
            source.at(0, 0) = MazeCell::Passage;
            const MazeGrid destination(std::move(source));

            source = MazeGrid(MazeSize(50, 50));
            source.at(0, 0) = MazeCell::Exit;
            Assert::AreEqual(50, source.size().rows);
            Assert::AreEqual(50, source.size().cols);
            Assert::AreEqual(std::size_t{ 2500 }, source.cellCount());
            Assert::IsTrue(source.at(0, 0) == MazeCell::Exit);
            Assert::IsTrue(source.at(49, 49) == MazeCell::Wall);
            Assert::AreEqual(std::size_t{ 100 }, destination.cellCount());
            Assert::IsTrue(destination.at(0, 0) == MazeCell::Passage);
        }
    };
}
