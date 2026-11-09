#pragma once

#include <algorithm>
#include <array>
#include <cmath>

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
}
