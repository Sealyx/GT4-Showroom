#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace gt4
{
    struct PhotoCrop
    {
        unsigned x = 0, y = 0, width = 0, height = 0;
    };

    struct ViewerPhoto
    {
        bool active = false, hidden = false, requested = false;
        bool dragging = false, movingPanel = false;
        static constexpr int settingCount = 4, rowCount = 6;
        int row = 4, ratio = 0, side = 1;
        float yaw = 0, pitch = .18f, zoom = 1, fov = 39.30765f, roll = 0;
        float panX = 0, panY = 0, panZ = 0;
        float entryYaw = 0;
        float panelX = .025f, panelY = .06f, dragX = 0, dragY = 0;
        std::string status;
        double statusUntil = 0;
        double began = 0;

        static constexpr std::array<const char*, 7> ratios{"Original", "2.39:1", "1.85:1", "16:9", "4:3", "4:5", "1:1"};

        void begin(const VehicleViewer& viewer, double now)
        {
            *this = {};
            active = true;
            began = now;
            yaw = entryYaw = viewer.yaw(now);
            pitch = viewer.pitch;
            zoom = viewer.zoom;
        }

        void beginCinematic(double now)
        {
            *this = {};
            active = true;
            began = now;
            yaw = pitch = 0;
        }

        bool adjust(int direction)
        {
            if (!direction || row >= settingCount) {
                return false;
            }
            side = direction < 0 ? -1 : 1;
            if (row == 0) {
                const float before = zoom;
                zoom = std::clamp(zoom * std::exp(direction * .08f), .3f, 3.f);
                return before != zoom;
            } else if (row == 1) {
                const float before = fov;
                fov = std::clamp(fov + direction, 15.f, 100.f);
                return before != fov;
            } else if (row == 2) {
                const float before = roll;
                roll = std::remainder(roll + direction, 360.f);
                return before != roll;
            } else if (row == 3) {
                ratio = (ratio + direction + int(ratios.size())) % int(ratios.size());
                return true;
            }
            return false;
        }

        static float uiScale(float width, float height)
        {
            return std::min(height / 720.f, width / 960.f);
        }

        UIRect panel(float width, float height) const
        {
            const float scale = uiScale(width, height);
            const float panelWidth = 420 * scale, panelHeight = 169 * scale;
            return {std::clamp(width * panelX, 0.f, std::max(0.f, width - panelWidth)),
                    std::clamp(height * panelY, 0.f, std::max(0.f, height * .907f - panelHeight)),
                    panelWidth,
                    panelHeight};
        }

        UIRect header(float width, float height) const
        {
            const auto box = panel(width, height);
            return {box.x, box.y, box.w, 21 * uiScale(width, height)};
        }

        // screen fractions keep the panel reachable after the output size changes
        void place(float x, float y, float width, float height)
        {
            const auto box = panel(width, height);
            panelX = std::clamp(x, 0.f, std::max(0.f, width - box.w)) / std::max(1.f, width);
            panelY = std::clamp(y, 0.f, std::max(0.f, height * .907f - box.h)) / std::max(1.f, height);
        }

        UIRect choice(float width, float height, int index) const
        {
            const auto box = panel(width, height);
            const float scale = uiScale(width, height);
            if (index >= settingCount) {
                return {box.x + box.w * (index == settingCount ? .08f : .58f),
                        box.y + 142 * scale,
                        box.w * .34f,
                        20 * scale};
            }
            return {box.x + 2 * scale, box.y + (22 + 28 * index) * scale, box.w - 4 * scale, 28 * scale};
        }

        UIRect field(float width, float height, int index) const
        {
            const auto box = panel(width, height);
            const auto item = choice(width, height, index);
            const float scale = uiScale(width, height);
            return {box.x + box.w * .43f, item.y + 3 * scale, box.w * .54f, 22 * scale};
        }

        UIRect arrow(float width, float height, int index, int direction) const
        {
            const auto value = field(width, height, index);
            const float edge = std::max(.6f, uiScale(width, height));
            const float arrowHeight = value.h - 2 * edge;
            const float arrowWidth = arrowHeight * 1.27f;
            return {direction < 0 ? value.x + edge : value.x + value.w - edge - arrowWidth,
                    value.y + edge,
                    arrowWidth,
                    arrowHeight};
        }

        UIRect pointerTarget(float width, float height) const
        {
            return row < settingCount ? arrow(width, height, row, side) : choice(width, height, row);
        }

        int hit(float width, float height, float x, float y, int& direction) const
        {
            direction = 0;
            for (int index = 0; index < rowCount; ++index) {
                const auto area = index < settingCount ? field(width, height, index) : choice(width, height, index);
                if (!area.contains(x, y)) {
                    continue;
                }
                if (index < settingCount) {
                    if (arrow(width, height, index, -1).contains(x, y)) {
                        direction = -1;
                    } else if (arrow(width, height, index, 1).contains(x, y)) {
                        direction = 1;
                    }
                }
                return index;
            }
            return -1;
        }

        std::string help(double now, bool controller) const
        {
            static constexpr const char* keyboard[]{
                "ARROWS Settings    ENTER Select    SPACE Take Image    BACKSPACE Back",
                "RMB Look    WASD Pan    WHEEL Zoom    +/- FOV    Q/E Roll",
                "H Hide UI    R Reset Camera    DRAG Title Bar Move Panel"};
            static constexpr const char* gamepad[]{
                "D-PAD Settings    CROSS Select    CIRCLE Back",
                "R-STICK Look    SELECT + R-STICK Pan    L2/R2 Zoom    L1/R1 Roll",
                "L3 Hide UI    TRIANGLE Reset Camera    SQUARE + R-STICK Move Panel"};
            const auto page = size_t(std::fmod(std::max(0., now - began) / 3., 3.));
            return controller ? gamepad[page] : keyboard[page];
        }

        // crop the finished studio image once; changing ratio never builds tiled captures
        PhotoCrop crop(unsigned width, unsigned height) const
        {
            PhotoCrop result{0, 0, width, height};
            constexpr float values[]{0, 2.39f, 1.85f, 16.f / 9, 4.f / 3, 4.f / 5, 1};
            if (!width || !height || ratio <= 0 || ratio >= int(ratios.size())) {
                return result;
            }
            const float aspect = values[ratio];
            if (float(width) / height > aspect) {
                result.width = std::clamp(unsigned(std::lround(height * aspect)), 1u, width);
            } else {
                result.height = std::clamp(unsigned(std::lround(width / aspect)), 1u, height);
            }
            result.x = (width - result.width) / 2;
            result.y = (height - result.height) / 2;
            return result;
        }
    };
}
