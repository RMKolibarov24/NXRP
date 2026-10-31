#pragma once

#include <filesystem>

// An empty result disables persistence if the user's app-data path is unavailable.
std::filesystem::path settingsFilePath();
