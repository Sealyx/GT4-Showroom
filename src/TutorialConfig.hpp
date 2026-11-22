#pragma once
#include <windows.h>
#include <cwchar>

namespace gt4
{
    inline constexpr unsigned welcomeTutorialBit = 1u << 4;

    inline bool loadTutorialEnabled(const wchar_t* ini)
    {
        wchar_t value[64]{};
        GetPrivateProfileStringW(L"GT4Showroom", L"TutorialBox", L"1", value, 64, ini);
        const bool enabled = value[0] != L'0';
        if (std::wcschr(value, L',')) {
            WritePrivateProfileStringW(L"GT4Showroom", L"TutorialBox", enabled ? L"1" : L"0", ini);
        }
        // Delete only obsolete keys from older installations
        for (const auto* section : {L"GT4Showroom", L"TutorialBox"}) {
            for (const auto* key : {L"SeenMask", L"TutorialSeenMask", L"WelcomeSeen", L"CinematicTutorialSeen",
                                    L"CustomizationIntroSeen", L"CustomizationTutorialSeen", L"ViewerTutorialSeen"}) {
                wchar_t existing[16]{};
                if (GetPrivateProfileStringW(section, key, L"", existing, 16, ini)) {
                    WritePrivateProfileStringW(section, key, nullptr, ini);
                }
            }
        }
        return enabled;
    }

    inline bool completeTutorials(const wchar_t* ini, bool& enabled, unsigned acknowledged)
    {
        if (!enabled || acknowledged != 31u) return true;
        if (!WritePrivateProfileStringW(L"GT4Showroom", L"TutorialBox", L"0", ini)) return false;
        enabled = false;
        return true;
    }
}
