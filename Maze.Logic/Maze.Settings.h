#pragma once

#include <filesystem>

#include "MazeSize.h"

// Missing, unreadable or malformed preferences use the default 10x10 size.
MazeSize loadMazeSizePreferences(const std::filesystem::path& filePath);

// Saves only valid dimensions. Returns false on a filesystem/write failure.
bool saveMazeSizePreferences(const std::filesystem::path& filePath, const MazeSize& size);
