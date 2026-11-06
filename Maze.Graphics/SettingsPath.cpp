#include "SettingsPath.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <ShlObj.h>

#include <memory>

std::filesystem::path settingsFilePath()
{
    PWSTR rawPath = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &rawPath);
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> pathOwner(rawPath, &CoTaskMemFree);
    if (FAILED(result) || rawPath == nullptr) return {};

    return std::filesystem::path(rawPath) / "NXRP" / "settings.txt";
}
