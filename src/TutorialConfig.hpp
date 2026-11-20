#pragma once
#include <windows.h>
#include <array>
#include <cwchar>
#include <string>

namespace gt4
{
    inline constexpr unsigned welcomeTutorialBit = 1u << 4;
    inline constexpr wchar_t tutorialSection[] = L"GT4Showroom";
    inline constexpr wchar_t tutorialKey[] = L"TutorialBox";
    inline constexpr std::array<const wchar_t*, 5> legacyTutorialKeys{
        L"CinematicTutorialSeen", L"CustomizationIntroSeen", L"ViewerTutorialSeen",
        L"CustomizationTutorialSeen", L"WelcomeSeen"};

    struct TutorialConfig
    {
        bool enabled = true;
        unsigned seen = 0;
    };

    inline bool saveTutorialConfig(const wchar_t* ini, TutorialConfig options)
    {
        const std::wstring value = std::wstring(options.enabled ? L"1," : L"0,") +
                                   std::to_wstring(options.seen & 31u);
        return WritePrivateProfileStringW(tutorialSection, tutorialKey, value.c_str(), ini) != 0;
    }

    inline TutorialConfig loadTutorialConfig(const wchar_t* ini)
    {
        wchar_t value[64]{};
        GetPrivateProfileStringW(tutorialSection, tutorialKey, L"1", value, 64, ini);
        wchar_t* end = nullptr;
        const auto enabled = std::wcstol(value, &end, 10);
        TutorialConfig options;
        options.enabled = end == value || enabled != 0;
        end += std::wcsspn(end, L" \t");
        if (*end == L',') {
            wchar_t* maskEnd = nullptr;
            const auto mask = std::wcstoul(end + 1, &maskEnd, 10);
            if (maskEnd != end + 1 && mask <= 31 && !maskEnd[std::wcsspn(maskEnd, L" \t")]) {
                options.seen = static_cast<unsigned>(mask);
            }
        }
        constexpr std::array<const wchar_t*, 2> sections{tutorialSection, tutorialKey};
        bool migrate = false;
        for (const auto* section : sections) {
            for (size_t i = 0; i < legacyTutorialKeys.size(); ++i) {
                const auto* key = legacyTutorialKeys[i];
                if (GetPrivateProfileStringW(section, key, L"", value, 64, ini) != 0) {
                    migrate = true;
                    if (std::wcstol(value, nullptr, 10) != 0) {
                        options.seen |= 1u << i;
                    }
                }
            }
        }
        // Write the merged value first so an interrupted migration can safely run again.
        if (migrate && saveTutorialConfig(ini, options)) {
            for (const auto* section : sections) {
                for (const auto* key : legacyTutorialKeys) {
                    WritePrivateProfileStringW(section, key, nullptr, ini);
                }
            }
        }
        return options;
    }
}
