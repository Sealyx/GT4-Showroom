#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "StudioBackgrounds.hpp"

namespace gt4
// vehicle studioView's main setup: inspired by gran turismo 7 spec iv update's car studioView
{
    enum class StudioViewScene
    {
        Softbox,
        Ribbons,
        Wave,
        Custom = 4
    };

    struct StudioViewPalette
    {
        const char* name;
        uint32_t color;
        uint32_t halo;
    };

    inline constexpr std::array<const char*, 5> studioViewSceneNames = {
        "Softbox Studio", "Ribbon Studio", "Wave Studio", nullptr, "Custom Studio"};

    inline bool studioViewWaveScene(StudioViewScene scene)
    {
        return scene == StudioViewScene::Wave;
    }

    inline constexpr std::array<StudioViewPalette, 11> studioViewPalettes = {{
        {"White", 0xEAF3FF, 0x768CA3},
        {"Warm White", 0xFFE1A1, 0xBA6327},
        {"Amber", 0xFF9F45, 0xA82917},
        {"Cyan", 0x44E7FF, 0x0633BB},
        {"Blue", 0x699FFF, 0x122AAE},
        {"Pink", 0xF079AD, 0xA32971},
        {"Red", 0xDF4949, 0x830C29},
        {"Yellow", 0xF6DB57, 0xA97712},
        {"Violet", 0xBB85FF, 0x4821B0},
        {"Mint", 0x7CFFBB, 0x057B7D},
        {"Green", 0x56D76F, 0x1B8340},
    }};

    inline constexpr int studioViewBlack = int(studioViewPalettes.size());
    inline constexpr int studioViewWhite = studioViewBlack + 1;

    inline const char* studioViewSurfaceName(int selection)
    {
        if (selection < 0) {
            return "Default";
        }
        if (selection == studioViewBlack) {
            return "Black";
        }
        if (selection == studioViewWhite) {
            return "Pure White";
        }
        return studioViewPalettes[size_t(selection)].name;
    }

    inline uint32_t studioViewSurfaceColor(int selection)
    {
        if (selection == studioViewBlack) {
            return 0;
        }
        if (selection == studioViewWhite) {
            return 0xFFFFFF;
        }
        return studioViewPalettes[size_t(selection)].color;
    }

    struct StudioViewSettings
    {
        StudioViewScene scene = StudioViewScene::Softbox;
        int palette = 0;
        int wallColor = -1;
        int floorColor = -1;
        bool animate = true;
        bool reflections = true, shadows = true, vehicleReflections = true, inheritStudio = true;
        int reflectionQuality = 3;
        int floorQuality = -1, vehicleQuality = -1, inheritedFloorQuality = -1;
        int showroomBackground=-1,showroomStyle=-1;
        int floorReflectionQuality() const { return floorQuality < 0 ? (reflections ? reflectionQuality : 0) : floorQuality; }
        int vehicleReflectionQuality() const { return vehicleQuality < 0 ? (vehicleReflections ? reflectionQuality : 0) : vehicleQuality; }

        const StudioViewPalette& lighting() const
        {
            static constexpr StudioViewPalette neutral{"White", 0xFFFFFF, 0x768CA3};
            if (scene == StudioViewScene::Softbox || scene == StudioViewScene::Custom) {
                return neutral;
            }
            return studioViewPalettes[size_t(std::clamp(palette, 0, int(studioViewPalettes.size()) - 1))];
        }

        void validate()
        {
            scene = StudioViewScene(std::clamp(int(scene), 0, int(studioViewSceneNames.size()) - 1));
            if (int(scene) == 3) {
                scene = StudioViewScene::Wave;
            }
            floorQuality=std::clamp(floorQuality,-1,4);vehicleQuality=std::clamp(vehicleQuality,-1,4);
            inheritedFloorQuality=std::clamp(inheritedFloorQuality,-1,4);
            showroomBackground=std::clamp(showroomBackground,-1,studioBackgroundCount-1);showroomStyle=std::clamp(showroomStyle,-1,1);
            palette = std::clamp(palette, 0, int(studioViewPalettes.size()) - 1);
            wallColor = std::clamp(wallColor, -1, studioViewWhite);
            floorColor = std::clamp(floorColor, -1, studioViewWhite);
        }
    };

    enum class StudioViewPhase
    {
        Stable,
        FadeOut,
        Black,
        FadeIn
    };

    struct StudioViewOrbitProfile
    {
        double duration;
        double frontTime;
    };

    inline constexpr std::array<StudioViewOrbitProfile, 5> studioViewOrbitProfiles = {
        {{26, 4.5}, {27, .75}, {32, 2.5}, {32, 2.5}, {26, 4.5}}};

    inline constexpr double studioEntryDelay = 2.;

    inline int studioBackdropCode(const StudioViewSettings& v,int style,int background) {
        return v.inheritStudio?(v.showroomStyle==1||(v.showroomStyle<0&&style==1)?200:100+(v.showroomBackground<0?background:v.showroomBackground)):int(v.scene);
    }
    inline bool animatedStudioBackdrop(int code) {return code==100+DefaultBackground||code==200||code==1||code==2;}
    inline std::vector<int> studioBackdropChoices(bool animated) {
        if(animated)return {100+DefaultBackground,200,1,2};
        std::vector<int> codes;for(int i=1;i<studioBackgroundCount;++i)codes.push_back(100+i);
        codes.push_back(0);codes.push_back(4);return codes;
    }
    inline const char* studioBackdropName(int code) {return code==200?"Prologue":code>=100?studioBackgroundNames[size_t(code-100)]:studioViewSceneNames[size_t(code)];}
    inline bool customizableStudioBackdrop(int code) {return code>=0&&code<4;}
    struct CinematicStudio
    {
        static constexpr double idleDelay = 150;
        static constexpr double fadeDuration = .3;
        static constexpr float fullTurn = 6.283185307179586f;

        StudioViewSettings settings;
        StudioViewPhase phase = StudioViewPhase::Stable;
        bool active = false, targetActive = false, automaticEntry = false, paused = false;
        bool hotspots = false, entryAudio = false;
        float yawOffset = 0, pitch = .18f, zoom = 1, zoomTarget = 1;
        float fromAlpha = 0;
        mutable bool showroomCamera = false;
        mutable std::array<float, 18> cameraSeed{};
        mutable float seedYaw = 0, seedPitch = .18f, seedSize = 1, seedAngle = 0, seedZoom = 1;
        double lastActivity = 0, began = 0, blackPresented = -1;
        double motionBegan = 0, heldMotion = 0, lastClock = 0;
        double backgroundBegan = 0, heldBackground = 0;
        bool backgroundPaused = false;

        static float ease(float value)
        {
            value = std::clamp(value, 0.f, 1.f);
            return value * value * (3 - 2 * value);
        }

        bool transitioning() const
        {
            return false;
        }

        float opacity(double now) const
        {
            (void)now;
            return 0;
        }

        bool request(bool enter, bool automatic, double now)
        {
            if (!std::isfinite(now) || (targetActive == enter && (active == enter || transitioning()))) {
                return false;
            }
            targetActive = active = enter;
            automaticEntry = false;
            phase = StudioViewPhase::Stable;
            fromAlpha = 0;
            began = lastClock = lastActivity = now;
            entryAudio = false;
            showroomCamera = false;
            if (enter) {
                hotspots = false;
                motionBegan = backgroundBegan = now;
                heldMotion = heldBackground = 0;
                yawOffset = 0;
                paused = false;
            }
            (void)automatic;
            return true;
        }

        bool activity(double now)
        {
            if (!std::isfinite(now)) {
                return false;
            }
            lastActivity = now;
            if (automaticEntry && (active || targetActive)) {
                request(false, false, now);
                return true;
            }
            return false;
        }

        void presented(double now)
        {
            if (phase == StudioViewPhase::Black && blackPresented < 0 && std::isfinite(now)) {
                blackPresented = now;
            }
        }

        void step(double now, bool eligible)
        {
            if (!std::isfinite(now)) {
                return;
            }
            now = std::max(lastClock, now);
            const double dt = now - lastClock;
            lastClock = now;
            settings.validate();
            syncBackground(now);
            zoom += (zoomTarget - zoom) * float(1 - std::exp(-std::min(dt, 1.) * 12));
            if (!eligible) lastActivity = now;
        }

        double elapsed(double now) const
        {
            return heldMotion + (paused ? 0 : std::max(0., now - motionBegan));
        }

        // background motion has its own clock so pausing the car needn't stop the studio
        double backgroundElapsed(double now) const
        {
            return heldBackground + (backgroundPaused ? 0 : std::max(0., now - backgroundBegan));
        }

        float shadowPhase(double now) const
        {
            return float(backgroundElapsed(now)) * .42f;
        }

        void syncBackground(double now)
        {
            const bool freeze = paused && !settings.animate;
            if (backgroundPaused != freeze) {
                heldBackground = backgroundElapsed(now);
                backgroundBegan = now;
                backgroundPaused = freeze;
            }
        }

        const StudioViewOrbitProfile& orbitProfile() const
        {
            const size_t scene = size_t(std::clamp(int(settings.scene), 0, int(studioViewOrbitProfiles.size()) - 1));
            return studioViewOrbitProfiles[scene];
        }

        float yaw(double now) const
        {
            const auto& profile = orbitProfile();
            const double phase = (profile.frontTime - std::fmod(elapsed(now), profile.duration)) / profile.duration;
            return float(phase) * fullTurn + yawOffset;
        }

        void changeScene(StudioViewScene scene, double now)
        {
            const float previousYaw = yaw(now);
            settings.scene = scene;
            settings.validate();
            yawOffset = std::remainder(yawOffset + previousYaw - yaw(now), fullTurn);
        }

        void toggle(double now)
        {
            heldMotion = elapsed(now);
            motionBegan = now;
            paused = !paused;
            syncBackground(now);
        }

        void orbit(float dx, float dy, double now)
        {
            if (!std::isfinite(dx) || !std::isfinite(dy) || std::abs(dx) + std::abs(dy) < .0001f) {
                return;
            }
            if (!paused) {
                toggle(now);
            }
            yawOffset = std::remainder(yawOffset + dx, fullTurn);
            pitch = std::clamp(pitch + dy, .04f, .65f);
        }

        void wheel(float amount)
        {
            if (std::isfinite(amount)) {
                zoomTarget = std::clamp(zoomTarget * std::exp(amount * .1f), .6f, 1.8f);
            }
        }

        void reset(double now)
        {
            const auto retainedSettings = settings;
            *this = {};
            settings = retainedSettings;
            lastActivity = lastClock = now;
            backgroundBegan = now;
        }
    };
}
