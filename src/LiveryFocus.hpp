#pragma once

#include <d3d9.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

namespace gt4
{
    // Sample the native paintjob once on acquisition. Keep its original texture for drawing
    inline std::pair<float, float> liveryFocus(IDirect3DTexture9* texture)
    {
        constexpr int grid = 40;
        const std::pair<float, float> center{.5f, .5f};
        D3DSURFACE_DESC desc{};
        if (!texture || FAILED(texture->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height) {
            return center;
        }
        const bool bc1 = desc.Format == D3DFMT_DXT1;
        const bool bc23 = desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5;
        const bool argb = desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8;
        if (!bc1 && !bc23 && !argb) {
            return center;
        }
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, D3DLOCK_READONLY))) {
            return center;
        }
        const int pitch = argb ? int(desc.Width * 4) : int((desc.Width + 3) / 4) * (bc1 ? 8 : 16);
        if (!lock.pBits || lock.Pitch < pitch) {
            texture->UnlockRect(0);
            return center;
        }
        using RGB = std::array<float, 3>;
        std::array<RGB, grid * grid> pixels{};
        const auto expand = [](unsigned c) -> RGB {
            return {float((c >> 11) * 255 / 31), float(((c >> 5) & 63) * 255 / 63), float((c & 31) * 255 / 31)};
        };
        const auto* data = static_cast<const uint8_t*>(lock.pBits);
        for (int j = 0; j < grid; ++j) {
            for (int i = 0; i < grid; ++i) {
                const unsigned x = (2 * i + 1) * desc.Width / (2 * grid);
                const unsigned y = (2 * j + 1) * desc.Height / (2 * grid);
                auto& p = pixels[j * grid + i];
                if (argb) {
                    const auto* q = data + y * lock.Pitch + x * 4;
                    const float a = desc.Format == D3DFMT_A8R8G8B8 ? q[3] / 255.f : 1.f;
                    p = {a*q[2], a*q[1], a*q[0]};
                    continue;
                }
                const auto* block = data + (y / 4) * lock.Pitch + (x / 4) * (bc1 ? 8 : 16);
                const auto* q = block + (bc1 ? 0 : 8);
                const unsigned a = q[0] | (q[1] << 8), b = q[2] | (q[3] << 8);
                const unsigned index = (q[4 + (y % 4)] >> (2 * (x % 4))) & 3;
                if (bc1 && a <= b && index == 3) {
                    continue;
                }
                const RGB first = expand(a), second = expand(b);
                const float t = index < 2 ? float(index) : (bc1 && a <= b ? .5f : (index - 1) / 3.f);
                const unsigned k = 4 * (y % 4) + x % 4;
                float alpha = 1;
                if (desc.Format == D3DFMT_DXT3) {
                    alpha = ((block[k / 2] >> (4 * (k % 2))) & 15) / 15.f;
                } else if (desc.Format == D3DFMT_DXT5) {
                    uint64_t bits = 0;
                    std::memcpy(&bits, block + 2, 6);
                    const unsigned code = unsigned((bits >> (3 * k)) & 7);
                    const unsigned last = block[0] > block[1] ? 7 : 5;
                    alpha = code < 2 ? block[code] / 255.f : code > last ? float(code == 7) :
                        ((last + 1 - code) * block[0] + (code - 1) * block[1]) / (255.f * last);
                }
                for (int c = 0; c < 3; ++c) {
                    p[c] = (first[c] + (second[c] - first[c]) * t) * alpha;
                }
            }
        }
        texture->UnlockRect(0);
        // Contrast within a portrait crop favors decal clusters over blank body panels
        std::array<unsigned, 512> bins{};
        std::array<RGB, 512> sums{};
        for (const auto& p : pixels) {
            const unsigned bin = (unsigned(p[0]) / 32) * 64 + (unsigned(p[1]) / 32) * 8 + unsigned(p[2]) / 32;
            ++bins[bin];
            for (int c = 0; c < 3; ++c) {
                sums[bin][c] += p[c];
            }
        }
        const auto dominant = size_t(std::max_element(bins.begin(), bins.end()) - bins.begin());
        std::array<float, grid * grid> energy{};
        for (int y = 1; y < grid; ++y) {
            for (int x = 1; x < grid; ++x) {
                const int n = y * grid + x;
                for (int c = 0; c < 3; ++c) {
                    const float edge = std::abs(pixels[n][c] - pixels[n - 1][c]) +
                                       std::abs(pixels[n][c] - pixels[n - grid][c]);
                    const float distinct = std::abs(pixels[n][c] - sums[dominant][c] / bins[dominant]);
                    energy[n] += .40f * std::min(100.f, edge) + std::min(140.f, distinct);
                }
            }
        }
        const float aspect = (24.f / 64.f) * desc.Height / desc.Width;
        const int halfX = std::clamp(int(12 * std::min(1.f, aspect)), 1, 12);
        const int halfY = std::clamp(int(12 * std::min(1.f, 1.f / aspect)), 1, 12);
        float best = 0;
        auto focus = center;
        for (int y = halfY; y < grid - halfY; y += 2) {
            for (int x = halfX; x < grid - halfX; x += 2) {
                float score = 0;
                for (int j = -halfY; j <= halfY; ++j) {
                    for (int i = -halfX; i <= halfX; ++i) {
                        score += energy[(y+j)*grid+x+i] * (1.f - .6f * std::abs(i) / halfX) *
                                 (1.f - .6f * std::abs(j) / halfY);
                    }
                }
                // A mild lower-atlas preference breaks ties with embedded header labels
                score *= .38f + .62f * (y + .5f) / grid;
                if (score > best) {
                    best = score;
                    focus = {(x + .5f) / grid, (y + .5f) / grid};
                }
            }
        }
        return focus;
    }
}
