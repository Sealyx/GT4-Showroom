#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace gt4::studioLighting
{
    inline constexpr double cycleSeconds = 23.1;

    struct LightFrame
    {
        float wall;
        float floor;
        float wallGlow;
        float glowCenter;
        float glowHeight;
        float floorPool;
        float poolCenter;
        float poolDepth;
        float poolSlope;
        float poolWidth;
        float shadowX;
        float shadowY;
        float shadowOpacity;
    };

    inline float smooth(float value)
    {
        const float t = std::clamp(value, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    inline LightFrame frame(double seconds)
    {
        struct Key
        {
            double seconds;
            LightFrame light;
        };
        static constexpr LightFrame dark{
            .13f, .15f, .082f, .50f, .30f, .083f, .50f, .680f, .00f, .085f, .08f, .15f, .57f};
        static constexpr LightFrame floorLit{
            .152f, .420f, .137f, .50f, .30f, .240f, .50f, .710f, .00f, .103f, .50f, .20f, .58f};
        static constexpr LightFrame sweep{
            .169f, .203f, .356f, .46f, .25f, .345f, .21f, .663f, -.115f, .078f, -.66f, .10f, .67f};
        static constexpr LightFrame wash{
            .416f, .422f, .246f, .12f, .30f, .470f, .21f, .523f, -.047f, .123f, -.28f, .27f, .42f};
        static constexpr std::array<Key, 10> keys{{
            {0., dark},
            {2., dark},
            {6.0, floorLit},
            {9.75, floorLit},
            {11.05, sweep},
            {15.45, sweep},
            {17.45, wash},
            {20.0, wash},
            {23.0, dark},
            {cycleSeconds, dark},
        }};

        double elapsed = std::isfinite(seconds) ? std::fmod(seconds, cycleSeconds) : 0.;
        if (elapsed < 0.) {
            elapsed += cycleSeconds;
            if (elapsed >= cycleSeconds) {
                elapsed = 0.;
            }
        }
        const auto next = std::upper_bound(keys.begin(), keys.end(), elapsed, [](double time, const Key& key) {
            return time < key.seconds;
        });
        const auto& start = *(next - 1);
        const auto& end = *next;
        const float t = smooth(float((elapsed - start.seconds) / (end.seconds - start.seconds)));
        const auto blend = [&](float LightFrame::*field) {
            const float from = start.light.*field;
            return from + (end.light.*field - from) * t;
        };
        return {
            blend(&LightFrame::wall),
            blend(&LightFrame::floor),
            blend(&LightFrame::wallGlow),
            blend(&LightFrame::glowCenter),
            blend(&LightFrame::glowHeight),
            blend(&LightFrame::floorPool),
            blend(&LightFrame::poolCenter),
            blend(&LightFrame::poolDepth),
            blend(&LightFrame::poolSlope),
            blend(&LightFrame::poolWidth),
            blend(&LightFrame::shadowX),
            blend(&LightFrame::shadowY),
            blend(&LightFrame::shadowOpacity),
        };
    }

    inline float luminance(const LightFrame& light, float x, float y)
    {
        const float floorMix = smooth((y - .53f) / .35f);
        const float wall = light.wall + (light.floor - light.wall) * floorMix;
        const float glowX = (x - light.glowCenter) / .38f;
        const float glowY = (y - light.glowHeight) / .35f;
        const float wallGlow = light.wallGlow * std::exp(-glowX * glowX - glowY * glowY) * (1.f - floorMix);

        const float poolX = (x - light.poolCenter) / .68f;
        const float poolY = (y - light.poolDepth - (x - .5f) * light.poolSlope) / light.poolWidth;
        const float floorPool = light.floorPool * std::exp(-poolX * poolX - poolY * poolY);
        return std::clamp(wall + wallGlow + floorPool, 0.f, 1.f);
    }

    inline float wallGrain(float x, float y)
    {
        const auto noise = [](float u, float v) {
            const int ix = int(std::floor(u)), iy = int(std::floor(v));
            const float tx = smooth(u - ix), ty = smooth(v - iy);
            const auto sample = [](int px, int py) {
                uint32_t value = uint32_t(px) * 0x9E3779B9u ^ uint32_t(py) * 0x85EBCA6Bu;
                value ^= value >> 16;
                value *= 0x7FEB352Du;
                value ^= value >> 15;
                return float(value & 0xFFFFu) / 32767.5f - 1.f;
            };
            const float top = sample(ix, iy) * (1.f - tx) + sample(ix + 1, iy) * tx;
            const float bottom = sample(ix, iy + 1) * (1.f - tx) + sample(ix + 1, iy + 1) * tx;
            return top * (1.f - ty) + bottom * ty;
        };
        return 9.f * noise(x * 7.f, y * 6.f) + 5.f * noise(x * 17.f, y * 14.f) +
               2.f * noise(x * 31.f, y * 27.f);
    }

    // wall texture stops at the floor and is omitted from vehicle environment captures
    inline std::array<float, 3> backdrop(int style, float x, float y, const LightFrame& light,
                                         bool wallTexture = true)
    {
        const auto gaussian = [](float a, float b) {
            return std::exp(-a * a - b * b);
        };
        if (style == 1) {
            const float wall = gaussian((x - .71706f) / .47605f, (y - .44441f) / .12729f);
            const float floor = gaussian((x - .51257f) / .44833f, (y - .67265f) / .11901f);
            return {50.6177f * wall + 131.7988f * floor,
                    2.7294f * wall + 8.1050f * floor,
                    .0524f * wall + .1855f * floor};
        }
        if (style == 2) {
            const float floor = smooth((y - .55f) / .10f);
            const float wall = gaussian((x - .56f) / .30f, (y - .04f) / .61f);
            const float pool = gaussian((x - .51f) / .37f, (y - .69f) / .09f);
            const float grain = wallTexture ? wallGrain(x, y) * (1.f - floor) : 0.f;
            const float value = (28.f + 95.f * wall + grain) * (1.f - floor) +
                                (28.f + 48.f * pool) * floor;
            return {value, value, value};
        }
        const float floor = smooth((y - .47f) / .26f);
        const float wall = gaussian((x - .49532f) / .23000f, (y - .33000f) / .12866f);
        const float pool = gaussian((x - .48728f) / .25164f, (y - .58699f) / .08112f);
        const std::array<float, 3> darkest{
            8.7333f + 20.2841f * floor + 45.6214f * wall + 91.3926f * pool,
            9.4693f + 21.7413f * floor + 47.7169f * wall + 92.8008f * pool,
            10.1736f + 22.5691f * floor + 49.0526f * wall + 95.2844f * pool};
        // only the darkest phase is replaced; the existing bright sweep keeps its own lighting
        const float phase = smooth(std::max((light.wall - .13f) / .022f, (light.floor - .15f) / .27f));
        if (phase <= 0) {
            return darkest;
        }
        const float brightness = luminance(light, x, y);
        const float oval = std::exp(-std::pow((x - .5f) / .52f, 2) - std::pow((y - .69f) / .12f, 2));
        const float dark = std::clamp((.23f - brightness) / .14f, 0.f, 1.f);
        const float strength = std::min(1.f, brightness + oval * .10f * dark);
        std::array<float, 3> original{247.f * strength, 252.f * strength, 255.f * strength};
        if (phase < 1) {
            for (size_t channel = 0; channel < original.size(); ++channel) {
                original[channel] = darkest[channel] + (original[channel] - darkest[channel]) * phase;
            }
        }
        return original;
    }
}
