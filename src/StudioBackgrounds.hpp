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
    inline constexpr int prologueReflectionPresentation = studioBackgroundCount;

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
}
