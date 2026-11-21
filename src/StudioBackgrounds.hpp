#pragma once
#include <array>
#include "Countries.hpp"

namespace gt4
{
    enum StudioBackground { DefaultBackground, RedBackground, DarkGrayBackground,
                            StaticBackground, BrushedSteelBackground, DarkGraphiteBackground, WarmBackground };
    inline constexpr std::array<const char*,7> studioBackgroundNames{
        "Default", "Red", "Dark Gray", "Static", "Brushed Steel", "Dark Graphite", "Wooden Taupe"
    };
    inline constexpr int studioBackgroundCount = int(studioBackgroundNames.size());
    // Even slots belong to backgrounds and odd slots to alternate styles. Append new IDs to keep saved slots stable
    inline constexpr int studioStyleCount = 2;
    inline constexpr size_t reflectionPresetCount = 2 * (studioBackgroundCount > studioStyleCount - 1
        ? studioBackgroundCount : studioStyleCount - 1);
    inline constexpr uint8_t inheritedFloorQuality = 255;
    inline size_t reflectionPreset(int style, int background)
    {
        return style == 0 ? size_t(2 * background) : size_t(2 * style - 1);
    }

    inline int defaultFloorQuality(int style, int background)
    {
        return style == 0 && (background == DefaultBackground || background == DarkGrayBackground) ? 0 : 3;
    }

    inline bool studioCountrySelected(uint32_t mask, int filter, Country country)
    {
        return mask ? mask == (1u << unsigned(country)) : filter == int(country);
    }

    inline bool studioBikeSelected(uint32_t mask, int filter)
    {
        constexpr uint32_t bikes = (1u << 7) | (1u << 8) | (1u << 11);
        return mask ? (mask & ~bikes) == 0
                    : filter >= 0 && filter <= 12 && (bikes & (1u << filter)) != 0;
    }
    inline int studioBackgroundForFilters(uint32_t countryMask, int countryFilter, uint32_t classMask, int classFilter)
    {
        if (studioBikeSelected(classMask, classFilter)) { return DarkGrayBackground; }
        if (studioCountrySelected(countryMask, countryFilter, Country::Japan)) { return RedBackground; }
        if (studioCountrySelected(countryMask, countryFilter, Country::Germany)) { return BrushedSteelBackground; }
        if (studioCountrySelected(countryMask, countryFilter, Country::SouthKorea)) { return DarkGraphiteBackground; }
        if (studioCountrySelected(countryMask, countryFilter, Country::UK)) { return WarmBackground; }
        return DefaultBackground;
    }
}
