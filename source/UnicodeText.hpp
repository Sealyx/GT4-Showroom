#pragma once
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include "Localization.hpp"

namespace gt4
{
    inline bool needsUnicode(const std::string &value)
    {
        return std::any_of(value.begin(), value.end(), [](unsigned char c) { return c >= 128; });
    }

    struct UnicodeText
    {
        struct Image
        {
            IDirect3DTexture9 *texture = nullptr;
            unsigned width = 0, height = 0;
            float em = 64, advance = 0;
            size_t bytes = 0;
        };
        std::unordered_map<std::string, Image> images;
        size_t bytes = 0;
        int language = -1;
        static constexpr size_t budget = 8 * 1024 * 1024;

        ~UnicodeText()
        {
            reset();
        }

        void reset()
        {
            for (auto &entry : images)
            {
                if (entry.second.texture)
                {
                    entry.second.texture->Release();
                }
            }
            images.clear();
            bytes = 0;
            language = -1;
        }

        static HFONT createFont(int em)
        {
            return CreateFontW(-em, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, localization.family());
        }

        static float measure(const std::string &text)
        {
            static std::unordered_map<std::string, float> measured;
            static int measuredLanguage = -1;
            if (measuredLanguage != localization.selected || measured.size() >= 256)
            {
                measured.clear();
                measuredLanguage = localization.selected;
            }
            const auto cached = measured.find(text);
            if (cached != measured.end())
            {
                return cached->second;
            }
            const auto value = wideLanguageText(text);
            HDC dc = CreateCompatibleDC(nullptr);
            HFONT font = createFont(64);
            SIZE size{};
            if (!dc || !font)
            {
                if (font)
                {
                    DeleteObject(font);
                }
                if (dc)
                {
                    DeleteDC(dc);
                }
                return float(value.size()) * .6f;
            }
            const auto old = SelectObject(dc, font);
            GetTextExtentPoint32W(dc, value.data(), int(value.size()), &size);
            SelectObject(dc, old);
            DeleteObject(font);
            DeleteDC(dc);
            const float width = float(size.cx) / 64;
            measured.emplace(text, width);
            return width;
        }

        // render complete unicode runs through windows font fallback instead of treating utf-8 bytes as glyphs
        const Image *get(IDirect3DDevice9 *device, const std::string &text)
        {
            if (language != localization.selected)
            {
                reset();
                language = localization.selected;
            }
            auto found = images.find(text);
            if (found != images.end())
            {
                return found->second.texture ? &found->second : nullptr;
            }
            if (images.size() >= 96 || bytes >= budget)
            {
                reset();
                language = localization.selected;
            }
            const auto value = wideLanguageText(text);
            if (value.empty() || value.size() > 4096)
            {
                return nullptr;
            }
            D3DCAPS9 caps{};
            if (FAILED(device->GetDeviceCaps(&caps)) || !caps.MaxTextureWidth || !caps.MaxTextureHeight)
            {
                return nullptr;
            }
            HDC dc = CreateCompatibleDC(nullptr);
            if (!dc)
            {
                return nullptr;
            }
            int em = 64;
            HFONT font = nullptr;
            HGDIOBJ previousFont = nullptr;
            SIZE size{};
            for (; em >= 8; em /= 2)
            {
                font = createFont(em);
                if (!font)
                {
                    break;
                }
                previousFont = SelectObject(dc, font);
                GetTextExtentPoint32W(dc, value.data(), int(value.size()), &size);
                if (size.cx > 0 && unsigned(size.cx + 8) <= caps.MaxTextureWidth &&
                    unsigned(em * 2) <= caps.MaxTextureHeight)
                {
                    break;
                }
                SelectObject(dc, previousFont);
                DeleteObject(font);
                font = nullptr;
            }
            Image image;
            image.em = float(em);
            image.advance = float(size.cx);
            image.width = unsigned(std::max(1L, size.cx + 8));
            image.height = unsigned(std::max(1, em * 2));
            if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
            {
                const auto power = [](unsigned value)
                {
                    unsigned n = 1;
                    while (n < value)
                    {
                        n <<= 1;
                    }
                    return n;
                };
                image.width = power(image.width);
                image.height = power(image.height);
            }
            image.bytes = size_t(image.width) * image.height * 4;
            BITMAPINFO bitmapInfo{};
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = LONG(image.width);
            bitmapInfo.bmiHeader.biHeight = -LONG(image.height);
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;
            void *pixels = nullptr;
            HBITMAP bitmap = font && image.bytes <= budget && image.width <= caps.MaxTextureWidth &&
                                     image.height <= caps.MaxTextureHeight
                                 ? CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0)
                                 : nullptr;
            if (bitmap && pixels)
            {
                const auto old = SelectObject(dc, bitmap);
                std::memset(pixels, 0, image.bytes);
                SetTextColor(dc, RGB(255, 255, 255));
                SetBkMode(dc, TRANSPARENT);
                TextOutW(dc, 0, 0, value.data(), int(value.size()));
                GdiFlush();
                if (SUCCEEDED(device->CreateTexture(image.width, image.height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                    &image.texture, nullptr)) &&
                    image.texture)
                {
                    D3DLOCKED_RECT locked{};
                    if (SUCCEEDED(image.texture->LockRect(0, &locked, nullptr, 0)))
                    {
                        for (unsigned y = 0; y < image.height; ++y)
                        {
                            auto *row = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(locked.pBits) +
                                                                     size_t(y) * locked.Pitch);
                            const auto *input = static_cast<uint32_t *>(pixels) + size_t(y) * image.width;
                            for (unsigned x = 0; x < image.width; ++x)
                            {
                                const uint32_t coverage =
                                    (input[x] & 255) + ((input[x] >> 8) & 255) + ((input[x] >> 16) & 255);
                                row[x] = ((coverage / 3) << 24) | 0xFFFFFF;
                            }
                        }
                        image.texture->UnlockRect(0);
                    }
                    else
                    {
                        image.texture->Release();
                        image.texture = nullptr;
                    }
                }
                SelectObject(dc, old);
            }
            if (bitmap)
            {
                DeleteObject(bitmap);
            }
            if (font)
            {
                SelectObject(dc, previousFont);
                DeleteObject(font);
            }
            DeleteDC(dc);
            if (image.texture && bytes + image.bytes > budget)
            {
                reset();
                language = localization.selected;
            }
            bytes += image.texture ? image.bytes : 0;
            const auto inserted = images.emplace(text, image);
            return inserted.first->second.texture ? &inserted.first->second : nullptr;
        }
    };
} // namespace gt4
