#pragma once

#include "MazeSize.h"

constexpr int MIN_MAZE_SIZE = 10;
constexpr int MAX_MAZE_SIZE = 50;

bool isValidMazeSize(const MazeSize& size);