#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

inline std::vector<uint32_t> magnifierPixels(unsigned size)
{
    std::vector<uint32_t> pixels(size_t(size) * size);
    for (unsigned y = 0; y < size; ++y) {
        for (unsigned x = 0; x < size; ++x) {
            unsigned coverage = 0;
            for (unsigned sy = 0; sy < 4; ++sy) {
                for (unsigned sx = 0; sx < 4; ++sx) {
                    const float px = (x + (sx + .5f) * .25f) / size;
                    const float py = (y + (sy + .5f) * .25f) / size;
                    const float dx = px - .42f, dy = py - .42f;
                    const float ring = std::abs(std::sqrt(dx * dx + dy * dy) - .215f);
                    const float t = std::clamp(((px - .575f) + (py - .575f)) / .45f, 0.f, 1.f);
                    const float hx = px - (.575f + t * .225f), hy = py - (.575f + t * .225f);
                    coverage += ring < .035f || hx * hx + hy * hy < .0016f;
                }
            }
            pixels[size_t(y) * size + x] = ((coverage * 255 / 16) << 24) | 0x111214;
        }
    }
    return pixels;
}
