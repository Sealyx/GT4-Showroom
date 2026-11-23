#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace gt4
{
    struct PhotoCrop
    {
        unsigned x = 0, y = 0, width = 0, height = 0;
    };

    struct PhotoCameraState
    {
        bool valid = false;
        int ratio = 0;
        float yaw = 0, pitch = .18f, zoom = 1, fov = 39.30765f, roll = 0;
        float panX = 0, panY = 0, panZ = 0, entryYaw = 0;
    };

    struct ViewerPhoto : PhotoCameraState
    {
        bool active = false, hidden = false, requested = false;
        bool dragging = false, movingPanel = false, viewerContext = false;
        static constexpr int settingCount = 5, saveRow = 5, loadRow = 6, captureRow = 7, backRow = 8, rowCount = 9;
        bool visuals = false, settingsPressed = false;
        float settingsTravel = 0;
        int background = 0, style = 0;
        VehicleViewer originalViewer;
        float visualX=.65f, visualY=.06f;
        bool draggingVisual=false;
        UIRect visualPanel(float w,float h) const {
            const float unit=uiScale(w,h), width=420*unit,height=(25+28*visualCount())*unit;
            return {std::clamp(w*visualX,0.f,std::max(0.f,w-width)),
                    std::clamp(h*visualY,0.f,std::max(0.f,h*.907f-height)),width,height};
        }
        int visualCount() const { return viewerContext ? 7 : 5; }
        UIRect visualItem(float w,float h,int index) const {
            const auto box=visualPanel(w,h);
            const float scale=uiScale(w,h);
            return {box.x+2*scale,box.y+(22+28*index)*scale,box.w-4*scale,28*scale};
        }
        int row = captureRow, side = 1, preset = 0;
        std::array<PhotoCameraState, 3> presets;
        std::array<PhotoCameraState, 2> cameras;
        float panelX = .025f, panelY = .06f, dragX = 0, dragY = 0;
        std::string status;
        double statusUntil = 0;
        double began = 0, captureAt = -1., flashBegan = -10.;
        void triggerCapture(double now) { requested = true; captureAt = now + .72; }

        static constexpr std::array<const char*, 7> ratios{"Original", "2.39:1", "1.85:1", "16:9", "4:3", "4:5", "1:1"};

        // photo views belong to this inspection session, never the gameplay camera
        void rememberCamera()
        {
            if (valid) {
                cameras[size_t(viewerContext)] = static_cast<const PhotoCameraState&>(*this);
            }
        }

        void beginView(bool viewer, double now, bool resume)
        {
            rememberCamera();
            const auto saved = cameras;
            const auto retainedPresets = presets;
            const int retainedSlot = preset;
            const float x = panelX, y = panelY;
            *this = {};
            cameras = saved;
            presets = retainedPresets;
            preset = retainedSlot;
            panelX = x;
            panelY = y;
            viewerContext = viewer;
            if (resume && saved[size_t(viewer)].valid) {
                static_cast<PhotoCameraState&>(*this) = saved[size_t(viewer)];
            }
            active = true;
            began = now;
        }

        void begin(const VehicleViewer& viewer, double now, bool resume = true)
        {
            beginView(true, now, resume);
            if (!valid) {
                yaw = viewer.yaw(now);
                pitch = viewer.pitch;
                zoom = viewer.zoom;
            }
            entryYaw = viewer.yaw(now);
            valid = true;
        }

        void beginCinematic(double now, bool resume = true)
        {
            beginView(false, now, resume);
            if (!valid) {
                yaw = pitch = 0;
            }
            valid = true;
        }

        void end(const VehicleViewer& viewer, double now)
        {
            if (viewerContext) {
                yaw += viewer.yaw(now) - entryYaw;
                entryYaw = viewer.yaw(now);
            }
            rememberCamera();
            active = requested = dragging = movingPanel = false;
        }

        bool adjust(int direction)
        {
            if (!direction || row >= settingCount) {
                return false;
            }
            side = direction < 0 ? -1 : 1;
            if (row == 4) {
                preset = (preset + direction + 3) % 3;
                return true;
            }
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

        // presets are explicit saved poses; session cameras are still cleared when leaving the showroom
        static bool validPreset(const PhotoCameraState& camera)
        {
            const float values[]{camera.yaw, camera.pitch, camera.zoom, camera.fov, camera.roll,
                                 camera.panX, camera.panY, camera.panZ, camera.entryYaw};
            for (float value : values) {
                if (!std::isfinite(value)) {
                    return false;
                }
            }
            return camera.ratio >= 0 && camera.ratio < int(ratios.size()) &&
                camera.pitch >= -.3f && camera.pitch <= 1.3f && camera.zoom >= .3f && camera.zoom <= 3 &&
                camera.fov >= 15 && camera.fov <= 100 && std::abs(camera.roll) <= 360 &&
                std::abs(camera.panX) <= 8 && std::abs(camera.panY) <= 8 && camera.panZ >= -3 && camera.panZ <= 8;
        }

        bool storePreset()
        {
            auto pose = static_cast<const PhotoCameraState&>(*this);
            pose.valid = true;
            if (!validPreset(pose)) {
                return false;
            }
            presets[size_t(preset)] = pose;
            return true;
        }

        bool restorePreset(const VehicleViewer& viewer, double now)
        {
            const auto pose = presets[size_t(preset)];
            if (!pose.valid || !validPreset(pose)) {
                return false;
            }
            static_cast<PhotoCameraState&>(*this) = pose;
            entryYaw = viewerContext ? viewer.yaw(now) : 0;
            return true;
        }

        bool savePresets(const std::filesystem::path& path) const
        {
            std::array<uint8_t, 132> bytes{'G', 'T', '4', 'C', 1};
            for (size_t i = 0; i < presets.size(); ++i) {
                const auto& pose = presets[i];
                if (pose.valid && !validPreset(pose)) {
                    return false;
                }
                const size_t offset = 8 + i * 40;
                bytes[offset] = uint8_t(pose.valid);
                bytes[offset + 1] = uint8_t(pose.ratio);
                const float values[]{pose.yaw, pose.pitch, pose.zoom, pose.fov, pose.roll,
                                     pose.panX, pose.panY, pose.panZ, pose.entryYaw};
                std::memcpy(bytes.data() + offset + 4, values, sizeof(values));
            }
            bytes.back() = 0x39;
            for (size_t i = 0; i + 1 < bytes.size(); ++i) {
                bytes.back() ^= bytes[i];
            }
            auto temporary = path;
            temporary += L".tmp";
            {
                std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
                if (!output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()) || !output.flush()) {
                    output.close();
                    DeleteFileW(temporary.c_str());
                    return false;
                }
            }
            if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                DeleteFileW(temporary.c_str());
                return false;
            }
            return true;
        }

        bool loadPresets(const std::filesystem::path& path)
        {
            std::array<uint8_t, 132> bytes{};
            std::ifstream input(path, std::ios::binary);
            if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) ||
                input.peek() != std::char_traits<char>::eof() ||
                std::memcmp(bytes.data(), "GT4C", 4) != 0 || bytes[4] != 1) {
                return false;
            }
            uint8_t check = 0x39;
            for (size_t i = 0; i + 1 < bytes.size(); ++i) {
                check ^= bytes[i];
            }
            if (check != bytes.back()) {
                return false;
            }
            std::array<PhotoCameraState, 3> loaded{};
            for (size_t i = 0; i < loaded.size(); ++i) {
                const size_t offset = 8 + i * 40;
                if (bytes[offset] > 1 || bytes[offset + 2] || bytes[offset + 3]) {
                    return false;
                }
                float values[9]{};
                std::memcpy(values, bytes.data() + offset + 4, sizeof(values));
                auto& pose = loaded[i];
                pose = {bytes[offset] != 0, int(bytes[offset + 1]), values[0], values[1], values[2], values[3],
                        values[4], values[5], values[6], values[7], values[8]};
                if (!validPreset(pose)) {
                    return false;
                }
            }
            presets = loaded;
            return true;
        }

        static float uiScale(float width, float height)
        {
            return std::min(height / 720.f, width / 960.f);
        }

        UIRect panel(float width, float height) const
        {
            const float scale = uiScale(width, height);
            const float panelWidth = 420 * scale, panelHeight = 253 * scale;
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
            if (index == captureRow || index == backRow) {
                return {box.x + box.w * (index == captureRow ? .08f : .58f),
                        box.y + 226 * scale, box.w * .34f, 20 * scale};
            }
            if (index == saveRow || index == loadRow) {
                return {box.x + 2 * scale, box.y + (22 + 28 * index) * scale,
                        box.w - 4 * scale, 28 * scale};
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
            if (visuals) return visualItem(width,height,row);
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
                "Arrows Settings    RETURN Select    Space Take Image    BACKSPACE Return",
                "Drag RMB Look    WASD Pan    Wheel Zoom    +/- FOV    Q/E Roll",
                "RMB Settings    I Info    H Hide UI    R Reset    Drag Title Bar Move Panel"};
            static constexpr const char* gamepad[]{
                "D-PAD Settings    CROSS Select    CIRCLE Back",
                "R-STICK Look    SELECT + R-STICK Pan    L2/R2 Zoom    L1/R1 Roll",
                "SELECT Settings    SQUARE+TRIANGLE Info    L3 Hide UI    TRIANGLE Reset    SQUARE + R-STICK Move Panel"};
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
