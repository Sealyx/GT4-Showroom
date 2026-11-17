#pragma once
#include "StudioUI.hpp"

namespace gt4
{
    struct SearchPanel
    {
        float scale;
        UIRect box;

        SearchPanel(float width, float height, bool keyboard, float offset = 0)
            : scale(std::min(height / 720.f, width / 960.f))
        {
            const float w = 520 * scale, h = 174 * scale;
            box = {(width - w) * .5f, height * (keyboard ? .10f : .43f) + offset, w, h};
        }

        UIRect field(int row) const
        {
            return {box.x + box.w * .36f, box.y + (33 + row * 34) * scale,
                    box.w * .61f, 24 * scale};
        }

        UIRect button(int index) const
        {
            const float w = (box.w - 76 * scale) * .5f;
            return {box.x + 25 * scale + index * (w + 26 * scale),
                    box.y + 140 * scale, w, 21 * scale};
        }

        UIRect option(int index) const
        {
            const auto by = field(0);
            return {by.x, by.y + by.h + index * 26 * scale, by.w, 26 * scale};
        }

        UIRect target(int row, bool picker, bool byId) const
        {
            if (picker) {
                return option(byId ? 1 : 0);
            }
            return row < 2 ? field(row) : button(row - 2);
        }

        int hit(float x, float y) const
        {
            for (int i = 0; i < 4; ++i) {
                const auto rect = i < 2 ? field(i) : button(i - 2);
                if (rect.contains(x, y)) {
                    return i;
                }
            }
            return -1;
        }
    };
}
