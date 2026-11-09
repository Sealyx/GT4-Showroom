#pragma once
#include <windows.h>
#include "Catalogue.hpp"

namespace gt4
{
    enum class SpawnMode
    {
        InFront = 1,
        DriverSeat = 2,
        AnimatedEntry = 3
    };

    struct StudioConfig
    {
        SortSettings sort{SortBy::Id, false};
        SpawnMode spawn = SpawnMode::InFront;
        bool invalidSort = false;
    };

    inline StudioConfig readStudioConfig(const std::wstring& ini)
    {
        StudioConfig c;
        wchar_t spawn[16]{};
        if (GetPrivateProfileStringW(L"GT4Showroom", L"Spawn", L"", spawn, 16, ini.c_str())) {
            const int mode = GetPrivateProfileIntW(L"GT4Showroom", L"Spawn", 1, ini.c_str());
            if (mode >= 1 && mode <= 3) {
                c.spawn = static_cast<SpawnMode>(mode);
            }
        } else if (GetPrivateProfileIntW(L"GT4Showroom", L"SpawnInside", 0, ini.c_str()) != 0) {
            c.spawn = SpawnMode::AnimatedEntry;
        }
        return c;
    }

    inline SortSettings menuSort(int mode)
    {
        switch (mode) {
            case 1:
                return {SortBy::Name, true};
            case 2:
                return {SortBy::Name, false};
            case 3:
                return {SortBy::TopSpeed, true};
            case 4:
                return {SortBy::Country, false};
            case 5:
                return {SortBy::Id, false};
            default:
                return {SortBy::Name, false};
        }
    }
}
