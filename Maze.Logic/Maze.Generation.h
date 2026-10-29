#pragma once

#include <cstdint>
#include "MazeGrid.h"

// Builds a connected random maze with a passage at (0,0) and exactly one Exit
// at the bottom-right. Each dimension must be in the inclusive range 10..50.
// A fixed seed reproduces the map with the same generator/toolchain.
MazeGrid generateMaze(const MazeSize& size, std::uint32_t seed);

// Uses a seed from std::random_device. Entropy/allocation failures propagate.
// Invalid dimensions throw std::invalid_argument before generation.
MazeGrid generateMaze(const MazeSize& size);
