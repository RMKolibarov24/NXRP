#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include "MazeGrid.h"

// Builds a connected random maze with a passage at (0,0) and exactly one Exit
// at the bottom-right, including seeded alternate routes. Each dimension must be
// in the inclusive range 10..100.
// A fixed seed reproduces the map with the same generator/toolchain.
MazeGrid generateMaze(const MazeSize& size, std::uint32_t seed);

// Uses a seed from std::random_device. Entropy/allocation failures propagate.
// Invalid dimensions throw std::invalid_argument before generation.
MazeGrid generateMaze(const MazeSize& size);

// Obtain a seed once so callers can record it alongside the generated layout.
std::uint32_t generateMazeSeed();

// Empty input selects random generation. Otherwise require 1..10 decimal digits
// in the uint32 range. Invalid input leaves the output unchanged.
bool parseMazeSeed(std::string_view text, std::optional<std::uint32_t>& seed) noexcept;
