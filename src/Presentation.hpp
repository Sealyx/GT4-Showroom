#pragma once
#include "Showroom.hpp"

namespace gt4
{
    struct FooterLayout
    {
        float y, fontHeight, countY, countHeight;

        FooterLayout(float h)
            : y(h * .925f),
              fontHeight(18 * h / 720.f),
              countY(h * .929f),
              countHeight(15 * h / 720.f)
        {
        }
    };

    struct FocusDescriptionClock
    {
        std::array<int, 3> item{-1, -1, -1};
        double began = -1;

        bool describe(std::array<int, 3> selected, double now)
        {
            if (selected != item || began < 0 || now < began) {
                item = selected;
                began = now;
            }
            return std::fmod(now - began, 9.5) < 6.;
        }
    };

    struct FooterHelpClock
    {
        std::string message;
        int target = -1;
        double began = -1;

        bool describe(const std::string& help, int source, double now)
        {
            if (help.empty()) {
                message.clear();
                target = -1;
                began = -1;
                return false;
            }
            if (message != help || target != source || began < 0 || now < began) {
                message = help;
                target = source;
                began = now;
            }
            return std::fmod(std::max(0., now - began), 40.) < 30.;
        }
    };

    inline std::string footerSentence(std::string message)
    {
        if (!message.empty() && message.back() != '.' && message.back() != '?' && message.back() != '!') {
            message += '.';
        }
        return message;
    }

    inline float footerMarquee(float contentWidth, float availableWidth, double elapsed, float scale)
    {
        const float overflow = std::max(0.f, contentWidth - availableWidth);
        if (overflow <= 0 || !std::isfinite(elapsed)) {
            return 0;
        }
        const double travel = overflow / std::max(1.f, 30.f * scale);
        const double phase = std::fmod(std::max(0., elapsed), 4. + 2. * travel);
        if (phase < 2.) {
            return 0;
        }
        if (phase < 2. + travel) {
            return overflow * float((phase - 2.) / travel);
        }
        if (phase < 4. + travel) {
            return overflow;
        }
        return overflow * float(1. - (phase - 4. - travel) / travel);
    }

    inline double footerControlDuration(float content, float available, float scale)
    {
        const float overflow = std::max(0.f, content - available);
        return overflow > 0 ? 9. + 2. * overflow / std::max(1.f, 30.f * scale) : 3.;
    }

    inline float sideOnHeading(V3 player, V3 vehicle)
    {
        return std::atan2(-(vehicle.x - player.x), vehicle.y - player.y) + pi * .5f;
    }

    inline float sideSpawnDistance(float lowX, float highX)
    {
        const float width = std::max(std::abs(lowX), std::abs(highX));
        return std::isfinite(width) && width > .1f && width < 30 ? std::max(2.f, width + 1.f) : 2.6f;
    }

    inline bool ivfLightMarker(uint32_t rgb)
    {
        const auto r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
        return (r == 255 && b == 0 && ((g >= 173 && g <= 175) || (g >= 56 && g <= 60))) ||
               (g == 255 && b == 0 && r >= 181 && r <= 185) || (g == 255 && r == 0 && b >= 198 && b <= 200) ||
               (b == 255 && r == 0 && g >= 16 && g <= 18);
    }

    inline uint32_t previewMaterialColor(uint32_t rgb)
    {
        return ivfLightMarker(rgb) || rgb == 0x01FFFF ? 0xFFFFFF : rgb;
    }
}
