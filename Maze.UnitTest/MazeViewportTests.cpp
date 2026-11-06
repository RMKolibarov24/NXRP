#include "pch.h"
#include "CppUnitTest.h"
#include "../Maze.Graphics/MazeViewport.h"
#include "Maze.Game.h"

#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        void AssertWholeGrid(const MazeViewportLayout& view, const MazeSize& size, float width, float height)
        {
            Assert::IsTrue(view.cellSize >= 2.f);
            Assert::IsTrue(view.left <= -2.f && view.top <= -2.f);
            Assert::IsTrue(view.left + width >= static_cast<float>(size.cols) * view.cellSize + 2.f);
            Assert::IsTrue(view.top + height >= static_cast<float>(size.rows) * view.cellSize + 2.f);
        }

        void AssertPlayerVisible(const MazeViewportLayout& view, MazePosition player, float width, float height)
        {
            Assert::IsTrue(view.cellSize >= 2.f);
            const float x = static_cast<float>(player.col) * view.cellSize;
            const float y = static_cast<float>(player.row) * view.cellSize;
            Assert::IsTrue(x >= view.left && y >= view.top);
            Assert::IsTrue(x + view.cellSize <= view.left + width);
            Assert::IsTrue(y + view.cellSize <= view.top + height);
        }
    }

    TEST_CLASS(MazeViewportTests)
    {
    public:
        TEST_METHOD(StartsAtFitAndBoundsZoom)
        {
            MazeViewport view;
            Assert::AreEqual(1, view.zoomLevel());
            for (int i = 0; i < 20; ++i) view.zoomOut();
            Assert::AreEqual(1, view.zoomLevel());
            for (int i = 0; i < 20; ++i) view.zoomIn();
            Assert::AreEqual(4, view.zoomLevel());
            view.fit();
            Assert::AreEqual(1, view.zoomLevel());
        }

        TEST_METHOD(FitContainsWholeGridAndBorder)
        {
            MazeViewport view;
            for (const auto size : { MazeSize(10, 10), MazeSize(50, 50), MazeSize(10, 50), MazeSize(50, 10), MazeSize(37, 24) })
            {
                AssertWholeGrid(view.layout(size, { 0, 0 }, 592.f, 310.f), size, 592.f, 310.f);
                AssertWholeGrid(view.layout(size, { size.rows - 1, size.cols - 1 }, 852.f, 350.f), size, 852.f, 350.f);
            }
        }

        TEST_METHOD(ZoomIncreasesCellsAndKeepsPlayerVisible)
        {
            MazeViewport view;
            const MazeSize size(50, 50);
            const MazePosition player{ 25, 25 };
            const auto fitted = view.layout(size, player, 592.f, 310.f);
            view.zoomIn();
            const auto zoomed = view.layout(size, player, 592.f, 310.f);
            Assert::IsTrue(zoomed.cellSize > fitted.cellSize);
            AssertPlayerVisible(zoomed, player, 592.f, 310.f);
        }

        TEST_METHOD(AllCornersStayVisibleAtMaximumZoom)
        {
            MazeViewport view;
            for (int i = 0; i < 3; ++i) view.zoomIn();
            for (const auto size : { MazeSize(10, 10), MazeSize(50, 50), MazeSize(10, 50), MazeSize(50, 10) })
            {
                for (const auto player : { MazePosition{ 0, 0 }, MazePosition{ 0, size.cols - 1 },
                    MazePosition{ size.rows - 1, 0 }, MazePosition{ size.rows - 1, size.cols - 1 } })
                    AssertPlayerVisible(view.layout(size, player, 592.f, 310.f), player, 592.f, 310.f);
            }
        }

        TEST_METHOD(ResizePreservesZoomAndVisibility)
        {
            MazeViewport view;
            view.zoomIn();
            view.zoomIn();
            const MazePosition player{ 49, 49 };
            const auto small = view.layout({ 50, 50 }, player, 592.f, 310.f);
            const auto large = view.layout({ 50, 50 }, player, 1852.f, 850.f);
            Assert::AreEqual(3, view.zoomLevel());
            AssertPlayerVisible(small, player, 592.f, 310.f);
            AssertPlayerVisible(large, player, 1852.f, 850.f);
            Assert::IsTrue(large.cellSize > small.cellSize);
        }

        TEST_METHOD(FitAfterZoomRestoresWholeGrid)
        {
            MazeViewport view;
            for (int i = 0; i < 3; ++i) view.zoomIn();
            view.fit();
            AssertWholeGrid(view.layout({ 50, 50 }, { 49, 49 }, 592.f, 310.f), { 50, 50 }, 592.f, 310.f);
        }

        TEST_METHOD(InvalidOrTinyAreasProduceNoLayout)
        {
            MazeViewport view;
            Assert::IsTrue(view.layout({ 9, 10 }, { 0, 0 }, 592.f, 310.f).cellSize == 0.f);
            Assert::IsTrue(view.layout({ 10, 10 }, { 0, 0 }, 0.f, 310.f).cellSize == 0.f);
            Assert::IsTrue(view.layout({ 10, 10 }, { 0, 0 }, 20.f, 20.f).cellSize == 0.f);
            Assert::IsTrue(view.layout({ 10, 10 }, { 0, 0 }, std::numeric_limits<float>::quiet_NaN(), 310.f).cellSize == 0.f);
            Assert::IsTrue(view.layout({ 10, 10 }, { 0, 0 }, 592.f, std::numeric_limits<float>::infinity()).cellSize == 0.f);
            // At fit, a finite maximum-size world can still be representable.
            // Scaling it beyond float range must produce no drawable layout.
            for (int i = 0; i < 3; ++i) view.zoomIn();
            Assert::IsTrue(view.layout({ 10, 10 }, { 0, 0 }, std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()).cellSize == 0.f);
        }

        TEST_METHOD(CameraFollowsMovementAcrossTheMaze)
        {
            MazeGrid maze({ 50, 50 });
            maze.fill(MazeCell::Passage);
            maze.at(49, 49) = MazeCell::Exit;
            GameState game;
            startGame(game, std::move(maze));
            MazeViewport view;
            for (int i = 0; i < 3; ++i) view.zoomIn();
            for (const auto direction : { MoveDirection::Right, MoveDirection::Down })
            {
                for (int i = 0; i < 49; ++i)
                {
                    Assert::IsTrue(tryMove(game, direction) != MoveResult::Blocked);
                    AssertPlayerVisible(view.layout(game.maze.size(), game.player, 592.f, 310.f), game.player, 592.f, 310.f);
                }
            }
            Assert::IsTrue(game.phase == GamePhase::Won);
        }
    };
}
