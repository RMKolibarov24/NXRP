#pragma once

#include <filesystem>

// The actual process executable directory, independent of argv[0] and cwd.
std::filesystem::path executableDirectory();
