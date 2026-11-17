#pragma once
#include "StudioUI.hpp"
#include <array>
#include <string>
#include <random>
#include <cstring>
#include <windows.h>

namespace gt4
{
    struct PlatePanel
    {
        float scale;
        UIRect box;

        PlatePanel(float width, float height, bool keyboard, float offset = 0)
            : scale(std::min(height / 720.f, width / 960.f))
        {
            const float w = 520 * scale;
            box = {(width - w) * .5f, height * (keyboard ? .10f : .43f) + offset, w, 151 * scale};
        }

        UIRect field() const
        {
            return {box.x + box.w * .36f, box.y + 33 * scale, box.w * .61f, 24 * scale};
        }

        UIRect button(int index) const
        {
            const float w = (box.w - 102 * scale) / 3;
            return {box.x + 25 * scale + index * (w + 26 * scale), box.y + 112 * scale, w, 21 * scale};
        }

        UIRect target(int row) const
        {
            return row == 0 ? field() : button(row - 1);
        }

        int hit(float x, float y) const
        {
            for (int i = 0; i < 4; ++i) {
                if (target(i).contains(x, y)) {
                    return i;
                }
            }
            return -1;
        }
    };

    inline std::string plateText(const std::string& input)
    {
        std::string result;
        for (unsigned char c : input) {
            if (c >= 'a' && c <= 'z') {
                c = static_cast<unsigned char>(c - 'a' + 'A');
            }
            if (c >= 32 && c <= 126) {
                result += char(c);
                if (result.size() == 8) {
                    break;
                }
            }
        }
        return result;
    }

    template <class Random> std::string randomPlate(Random& random)
    {
        std::uniform_int_distribution<int> digit(0, 9), letter(0, 25);
        std::string result(8, '0');
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = i >= 1 && i <= 3 ? char('A' + letter(random)) : char('0' + digit(random));
        }
        return result;
    }

    inline bool plateHasSymbols(const std::string& text)
    {
        return std::any_of(text.begin(), text.end(), [](unsigned char c) {
            return c != ' ' && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9');
        });
    }

    // keep gta's letter shapes; only the punctuation missing from its atlas needs a small mask
    inline bool plateSymbolMask(const std::string& text, std::array<uint8_t, 64 * 16>& mask)
    {
        mask.fill(0);
        HDC dc = CreateCompatibleDC(nullptr);
        if (!dc) {
            return false;
        }
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = 64;
        info.bmiHeader.biHeight = -16;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        void* pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        HFONT face = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                 OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH, L"Liberation Sans");
        if (!bitmap || !face) {
            if (bitmap) {
                DeleteObject(bitmap);
            }
            if (face) {
                DeleteObject(face);
            }
            DeleteDC(dc);
            return false;
        }
        const auto oldBitmap = SelectObject(dc, bitmap);
        const auto oldFont = SelectObject(dc, face);
        std::memset(pixels, 0, 64 * 16 * 4);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkMode(dc, TRANSPARENT);
        for (size_t i = 0; i < std::min(size_t(8), text.size()); ++i) {
            const std::string character(1, text[i]);
            if (!plateHasSymbols(character)) {
                continue;
            }
            SIZE extent{};
            GetTextExtentPoint32A(dc, character.data(), 1, &extent);
            RECT clip{LONG(i * 8), 0, LONG(i * 8 + 8), 16};
            ExtTextOutA(dc, int(i * 8) + std::max(0, (8 - int(extent.cx)) / 2),
                        std::max(0, (16 - int(extent.cy)) / 2), ETO_CLIPPED, &clip,
                        character.data(), 1, nullptr);
        }
        GdiFlush();
        for (size_t i = 0; i < mask.size(); ++i) {
            const DWORD value = static_cast<const DWORD*>(pixels)[i];
            mask[i] = uint8_t(std::max({value & 255, (value >> 8) & 255, (value >> 16) & 255}));
        }
        SelectObject(dc, oldFont);
        SelectObject(dc, oldBitmap);
        DeleteObject(face);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return true;
    }

    // create first; a failed allocation must leave the vehicle's original plate intact
    template <class Create, class Drop>
    bool replacePlateTexture(void*& owned, const std::string& input, Create create, Drop drop)
    {
        const auto text = plateText(input);
        if (!owned || text.find_first_not_of(' ') == std::string::npos) {
            return true;
        }
        std::array<char, 9> nativeText{};
        nativeText.fill(' ');
        nativeText.back() = 0;
        std::copy(text.begin(), text.end(), nativeText.begin());
        void* replacement = create(nativeText.data());
        if (!replacement) {
            return false;
        }
        void* previous = owned;
        owned = replacement;
        drop(previous);
        return true;
    }
}
