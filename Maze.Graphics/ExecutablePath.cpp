#include "ExecutablePath.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

std::filesystem::path executableDirectory()
{
    DWORD capacity = MAX_PATH;
    for (;;)
    {
        std::vector<wchar_t> buffer(capacity);
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), capacity);
        if (length == 0)
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                "Could not locate the NXRP executable");
        if (length < capacity)
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        if (capacity > MAXDWORD / 2)
            throw std::length_error("The executable path exceeds the supported buffer size.");
        capacity *= 2;
    }
}
