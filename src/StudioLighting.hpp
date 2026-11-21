#pragma once

#include "StudioSweep.hpp"
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
        unsigned from = 0, to = 0;
        float mix = 0;
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
            unsigned setup;
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
            {0., dark, 0},
            {2.1, dark, 0},
            {5.9, floorLit, 1},
            {10.0, floorLit, 1},
            {11.4, sweep, 2},
            {15.6, sweep, 2},
            {17.6, wash, 3},
            {20.25, wash, 3},
            {23.05, dark, 0},
            {cycleSeconds, dark, 0},
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
        const float position = float((elapsed - start.seconds) / (end.seconds - start.seconds));
        // Fit the wash-to-dark envelope independently of the moving shadow and peak wall brightness
        const float t = (start.setup == 3 && end.setup == 0)
                            ? 1.f - std::pow(1.f - smooth(position), 1.22f)
                            : smooth(position);
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
            start.setup, end.setup, t,
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
            const float cove = smooth((y - .435f) / .08f);
            const float wall = 30.f - 5.f * smooth((y - .20f) / .66f) +
                105.f * gaussian((x - .503f) / .239f, (y + .075f) / .50f) +
                17.f * gaussian((x - .776f) / .32f, (y - .39f) / .34f) -
                6.f * gaussian((x - .134f) / .186f, (y - .46f) / .31f);
            const float floor = 24.f + 26.f * gaussian((x - .477f) / .557f, (y - .575f) / .111f) +
                68.f * gaussian((x - .485f) / .143f, (y - .578f) / .037f) +
                12.f * gaussian((x - .542f) / .304f, (y - .567f) / .069f);
            const float texture = wallTexture ? .32f * wallGrain(x, y) : 0.f;
            const float value = (wall + texture) * (1.f - cove) + floor * cove;
            return {value, value, value};
        }
        const float u = std::clamp(x, 0.f, 1.f) * 8.f;
        const float v = std::clamp(y, 0.f, 1.f);
        const int column = std::min(7, int(u));
        const auto next = std::upper_bound(std::begin(studioRows) + 1, std::end(studioRows) - 1, v);
        const size_t row = size_t(next - studioRows - 1);
        const auto interpolate = [](float previous, float left, float right, float following,
                                    float before, float width, float after, float fraction) {
            const float slope = (right - left) / width;
            const auto tangent = [](float a, float b) {
                return a * b > 0.f ? 2.f * a * b / (a + b) : 0.f;
            };
            const float start = before > 0 ? tangent((left - previous) / before, slope) : slope;
            const float end = after > 0 ? tangent(slope, (following - right) / after) : slope;
            const float t2 = fraction * fraction, t3 = t2 * fraction;
            return std::clamp(left * (2 * t3 - 3 * t2 + 1) + right * (3 * t2 - 2 * t3) +
                                  width * (start * (t3 - 2 * t2 + fraction) + end * (t3 - t2)),
                              std::min(left, right), std::max(left, right));
        };
        const size_t lastRow = std::size(studioRows) - 1;
        const size_t previous = row > 0 ? row - 1 : row;
        const size_t following = std::min(row + 2, lastRow);
        const float width = studioRows[row + 1] - studioRows[row];
        const float ty = (v - studioRows[row]) / width;
        const auto sample = [&](unsigned setup, unsigned shift) {
            const auto& stops = studioStops[setup];
            const auto line = [&](size_t yIndex) {
                const auto at = [&](int col) { return float((stops[yIndex][col] >> shift) & 255); };
                return interpolate(at(std::max(0, column - 1)), at(column), at(column + 1),
                                   at(std::min(column + 2, 8)), column > 0 ? 1.f : 0.f, 1.f,
                                   column < 7 ? 1.f : 0.f, u - column);
            };
            const float color = interpolate(line(previous), line(row), line(row + 1), line(following),
                                            studioRows[row] - studioRows[previous], width,
                                            studioRows[following] - studioRows[row + 1], ty);
            // The wall drops off before the distinct pool of light on the curved floor
            const float wallShoulder = gaussian((x - .5f) / .68f, (v - .464f) / .066f);
            if (setup == 2) {
                const float ridge = .586f - (x - .5f) * .115f;
                return color - 9.f * wallShoulder +
                       48.f * gaussian((x - .19f) / .27f, (v - ridge) / .036f);
            }
            if (setup == 3) {
                return color + 27.f * gaussian((x - .21f) / .36f, (v - .54f) / .083f);
            }
            return color - (setup == 1 ? 6.f : 5.f) * wallShoulder;
        };
        std::array<float, 3> rgb{};
        for (unsigned i = 0; i < 3; ++i) {
            const unsigned shift = 16 - i * 8;
            const float from = sample(light.from, shift);
            rgb[i] = light.from == light.to ? from : from + (sample(light.to, shift) - from) * light.mix;
        }
        const float value = std::clamp(.2126f * rgb[0] + .7152f * rgb[1] + .0722f * rgb[2], 0.f, 255.f);
        const float cool = std::min(2.5f, value * .027f);
        return {value - cool, value, std::min(255.f, value + cool * .6f)};
    }
}
