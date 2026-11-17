#pragma once
#include "StudioUI.hpp"
#include <windows.h>
#include <array>

namespace gt4
{
    struct InformationPanel
    {
        float scale;
        UIRect box;

        InformationPanel(float width, float height, float offset = 0)
            : scale(std::min(height / 720.f, width / 960.f))
        {
            const float w = 760 * scale, h = 444 * scale;
            box = {(width - w) * .5f, (height - h) * .5f + offset, w, h};
        }

        UIRect target(int index) const
        {
            if (index < 2) {
                return {box.x + 90 * scale, box.y + (330 + 22 * index) * scale, 160 * scale, 20 * scale};
            }
            if (index == 2) {
                return {box.x + 22 * scale, box.y + 408 * scale, 180 * scale, 24 * scale};
            }
            return {box.x + box.w * .35f, box.y + 405 * scale, box.w * .30f, 26 * scale};
        }

        int hit(float x, float y) const
        {
            for (int i = 0; i < 4; ++i) {
                if (target(i).contains(x, y)) {
                    return i;
                }
            }
            return -1;
        }
    };

    inline constexpr std::array<const wchar_t*, 3> informationLinks{
        L"https://www.youtube.com/@Sealyx", L"https://x.com/trulysealy",
        L"https://github.com/Sealyx/GT4-Showroom/releases"};

    inline bool openInformationLink(int index)
    {
        if (index < 0 || index >= int(informationLinks.size())) {
            return false;
        }
        wchar_t system[MAX_PATH]{};
        const UINT length = GetSystemDirectoryW(system, MAX_PATH);
        if (!length || length >= MAX_PATH - 13) {
            return false;
        }
        const std::wstring library = std::wstring(system, length) + L"\\shell32.dll";
        HMODULE module = LoadLibraryW(library.c_str());
        if (!module) {
            return false;
        }
        using Execute = HINSTANCE(WINAPI*)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
        const auto execute = reinterpret_cast<Execute>(GetProcAddress(module, "ShellExecuteW"));
        const bool opened = execute && reinterpret_cast<INT_PTR>(execute(
            nullptr, L"open", informationLinks[size_t(index)], nullptr, nullptr, SW_SHOWNORMAL)) > 32;
        FreeLibrary(module);
        return opened;
    }
}
