#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Validation.h"

#include <climits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        void AssertSizeValidity(const MazeSize& size, bool expected)
        {
            Assert::AreEqual(expected, size.isValid(), L"The data model rejected the expected size rule.");
            Assert::AreEqual(expected, isValidMazeSize(size), L"Logic validation disagrees with the expected result.");
        }
    }

    TEST_CLASS(MazeSizeTests)
    {
    public:
        TEST_METHOD(AcceptsMinimum)
        {
            AssertSizeValidity(MazeSize(10, 10), true);
        }

        TEST_METHOD(AcceptsMaximum)
        {
            AssertSizeValidity(MazeSize(50, 50), true);
        }

        TEST_METHOD(RejectsTooFewRows)
        {
            AssertSizeValidity(MazeSize(9, 10), false);
        }

        TEST_METHOD(RejectsTooManyRows)
        {
            AssertSizeValidity(MazeSize(51, 10), false);
        }

        TEST_METHOD(RejectsTooFewColumns)
        {
            AssertSizeValidity(MazeSize(10, 9), false);
        }

        TEST_METHOD(RejectsTooManyColumns)
        {
            AssertSizeValidity(MazeSize(10, 51), false);
        }

        TEST_METHOD(AcceptsRectangularSizes)
        {
            AssertSizeValidity(MazeSize(10, 50), true);
            AssertSizeValidity(MazeSize(50, 10), true);
        }

        TEST_METHOD(AcceptsInteriorSize)
        {
            AssertSizeValidity(MazeSize(23, 37), true);
        }

        TEST_METHOD(RejectsZeroNegativeAndExtremeDimensions)
        {
            const MazeSize sizes[] = {
                { 0, 10 }, { 10, 0 }, { -1, 10 }, { 10, -1 },
                { INT_MIN, 10 }, { 10, INT_MIN },
                { INT_MAX, 10 }, { 10, INT_MAX }
            };
            for (const auto& size : sizes)
                AssertSizeValidity(size, false);
        }

        TEST_METHOD(PreservesInputDimensionsForValidation)
        {
            const MazeSize valid(19, 31);
            Assert::AreEqual(19, valid.rows);
            Assert::AreEqual(31, valid.cols);

            const MazeSize invalid(-1, 51);
            Assert::AreEqual(-1, invalid.rows);
            Assert::AreEqual(51, invalid.cols);
            AssertSizeValidity(invalid, false);
        }
    };
}
