#pragma once

#include <algorithm>
#include <cmath>

#include "MazePosition.h"
#include "MazeSize.h"

struct MazeViewportLayout
{
    float cellSize = 0.f; // Zero means the board cannot be drawn in this area.
    float left = 0.f;
    float top = 0.f;
};

// Camera calculations use no SFML types, so native tests can check real geometry.
class MazeViewport
{
public:
    int zoomLevel() const noexcept { return zoom_; }
    void zoomIn() noexcept { zoom_ = std::min(zoom_ + 1, 4); }
    void zoomOut() noexcept { zoom_ = std::max(zoom_ - 1, 1); }
    void fit() noexcept { zoom_ = 1; }

    MazeViewportLayout layout(const MazeSize& size, MazePosition player,
        float width, float height) const noexcept
    {
        if (!size.isValid() || !std::isfinite(width) || !std::isfinite(height)
            || width <= 4.f || height <= 4.f) return {};

        const float cols = static_cast<float>(size.cols);
        const float rows = static_cast<float>(size.rows);
        // Leave two pixels on every side for the grid border.
        const float fittedCell = std::floor(std::min((width - 4.f) / cols, (height - 4.f) / rows));
        if (fittedCell < 2.f) return {};
        const float cell = fittedCell * static_cast<float>(zoom_);
        const float worldWidth = cols * cell, worldHeight = rows * cell;
        if (!std::isfinite(worldWidth) || !std::isfinite(worldHeight)) return {};

        const float playerX = (static_cast<float>(std::clamp(player.col, 0, size.cols - 1)) + 0.5f) * cell;
        const float playerY = (static_cast<float>(std::clamp(player.row, 0, size.rows - 1)) + 0.5f) * cell;
        return { cell, viewStart(worldWidth, width, playerX), viewStart(worldHeight, height, playerY) };
    }

private:
    static float viewStart(float worldSize, float areaSize, float playerCenter) noexcept
    {
        if (worldSize + 4.f <= areaSize)
            return std::floor((worldSize - areaSize) / 2.f);
        return std::clamp(std::floor(playerCenter - areaSize / 2.f), -2.f, worldSize + 2.f - areaSize);
    }

    int zoom_ = 1;
};
