#pragma once
#include <windows.h>
#include <cwchar>
#include <initializer_list>

namespace gt4
{
    inline constexpr unsigned welcomeTutorialBit = 1u << 4;

    inline bool loadTutorialEnabled(const wchar_t* ini)
    {
        wchar_t value[64]{};
        GetPrivateProfileStringW(L"TutorialBox",L"Enabled",L"",value,64,ini);
        if (!value[0]) {
            GetPrivateProfileStringW(L"GT4Showroom",L"TutorialBox",L"1",value,64,ini);
            if (!WritePrivateProfileStringW(L"TutorialBox",L"Enabled",value[0]==L'0' ? L"0" : L"1",ini))
                return value[0]!=L'0';
        }
        const bool enabled = value[0] != L'0';
        WritePrivateProfileStringW(L"GT4Showroom",L"TutorialBox",nullptr,ini);
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


    inline constexpr const wchar_t* tutorialProgressKeys[]{
        L"Cinematic", L"CustomizationIntro", L"Viewer", L"Customization", L"Welcome"
    };

    inline unsigned loadTutorialProgress(const wchar_t* ini, bool enabled)
    {
        unsigned shown = 0;
        for (unsigned i=0;i<5;++i) {
            if (GetPrivateProfileIntW(L"TutorialBox",tutorialProgressKeys[i],0,ini)==1) shown |= 1u<<i;
        }
        if (enabled && shown==31u) {
            for (const auto* key:tutorialProgressKeys)
                WritePrivateProfileStringW(L"TutorialBox",key,L"0",ini);
            return 0;
        }
        return shown;
    }

    inline bool saveTutorialProgress(const wchar_t* ini, bool& enabled, unsigned shown)
    {
        for (unsigned i=0;i<5;++i) {
            if ((shown & (1u<<i)) &&
                !WritePrivateProfileStringW(L"TutorialBox",tutorialProgressKeys[i],L"1",ini)) return false;
        }
        if (shown==31u) {
            if (!WritePrivateProfileStringW(L"TutorialBox",L"Enabled",L"0",ini)) return false;
            enabled=false;
        }
        return true;
    }
}
