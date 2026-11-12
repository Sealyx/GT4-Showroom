#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace gt4
// vehicle viewer's main setup: inspired by gran turismo 7 spec iv update's car viewer
{
    enum class ViewerScene
    {
        Softbox,
        Ribbons,
        Wave,
        Custom = 4
    };

    struct ViewerPalette
    {
        const char* name;
        uint32_t color;
        uint32_t halo;
    };

    inline constexpr std::array<const char*, 5> viewerSceneNames = {
        "Softbox Studio", "Ribbon Studio", "Wave Studio", nullptr, "Custom Studio"};

    inline bool viewerWaveScene(ViewerScene scene)
    {
        return scene == ViewerScene::Wave;
    }

    inline constexpr std::array<ViewerPalette, 11> viewerPalettes = {{
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

    inline constexpr int viewerBlack = int(viewerPalettes.size());
    inline constexpr int viewerWhite = viewerBlack + 1;

    inline const char* viewerSurfaceName(int selection)
    {
        if (selection < 0) {
            return "Default";
        }
        if (selection == viewerBlack) {
            return "Black";
        }
        if (selection == viewerWhite) {
            return "Pure White";
        }
        return viewerPalettes[size_t(selection)].name;
    }

    inline uint32_t viewerSurfaceColor(int selection)
    {
        if (selection == viewerBlack) {
            return 0;
        }
        if (selection == viewerWhite) {
            return 0xFFFFFF;
        }
        return viewerPalettes[size_t(selection)].color;
    }

    struct ViewerSettings
    {
        ViewerScene scene = ViewerScene::Softbox;
        int palette = 0;
        int wallColor = -1;
        int floorColor = -1;
        bool animate = true;
        bool reflections = true, shadows = true;

        const ViewerPalette& lighting() const
        {
            return viewerPalettes[size_t(std::clamp(palette, 0, int(viewerPalettes.size()) - 1))];
        }

        void validate()
        {
            scene = ViewerScene(std::clamp(int(scene), 0, int(viewerSceneNames.size()) - 1));
            if (int(scene) == 3) {
                scene = ViewerScene::Wave;
            }
            palette = std::clamp(palette, 0, int(viewerPalettes.size()) - 1);
            wallColor = std::clamp(wallColor, -1, viewerWhite);
            floorColor = std::clamp(floorColor, -1, viewerWhite);
        }
    };

    enum class ViewerPhase
    {
        Stable,
        FadeOut,
        Black,
        FadeIn
    };

    struct ViewerOrbitProfile
    {
        double duration;
        double frontTime;
    };

    inline constexpr std::array<ViewerOrbitProfile, 5> viewerOrbitProfiles = {
        {{26, 4.5}, {27, .75}, {32, 2.5}, {32, 2.5}, {26, 4.5}}};

    inline constexpr double studioEntryDelay = 2.;

    struct VehicleViewer
    {
        static constexpr double idleDelay = 150;
        static constexpr double fadeDuration = .3;
        static constexpr float fullTurn = 6.283185307179586f;

        ViewerSettings settings;
        ViewerPhase phase = ViewerPhase::Stable;
        bool active = false, targetActive = false, automaticEntry = false, paused = false;
        bool hotspots = false, entryAudio = false;
        float yawOffset = 0, pitch = .18f, zoom = 1, zoomTarget = 1;
        float fromAlpha = 0;
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
            return phase != ViewerPhase::Stable;
        }

        float opacity(double now) const
        {
            const float amount = ease(float(std::max(0., now - began) / fadeDuration));
            if (phase == ViewerPhase::FadeOut) {
                return fromAlpha + (1 - fromAlpha) * amount;
            }
            if (phase == ViewerPhase::Black) {
                return 1;
            }
            if (phase == ViewerPhase::FadeIn) {
                return fromAlpha * (1 - amount);
            }
            return 0;
        }

        bool request(bool enter, bool automatic, double now)
        {
            if (!std::isfinite(now) || (targetActive == enter && (active == enter || transitioning()))) {
                return false;
            }
            const float currentAlpha = opacity(now);
            targetActive = enter;
            blackPresented = -1;
            began = now;
            fromAlpha = currentAlpha;
            if (enter) {
                automaticEntry = automatic;
                entryAudio = true;
            }
            phase = !enter && !active ? ViewerPhase::FadeIn : ViewerPhase::FadeOut;
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
            if (phase == ViewerPhase::Black && blackPresented < 0 && std::isfinite(now)) {
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
            if (phase == ViewerPhase::FadeOut && now - began >= fadeDuration) {
                phase = ViewerPhase::Black;
            }
            if (phase == ViewerPhase::Black && blackPresented >= 0 &&
                (!targetActive || now - began >= studioEntryDelay)) {
                active = targetActive;
                phase = ViewerPhase::FadeIn;
                began = now;
                fromAlpha = 1;
                if (active) {
                    hotspots = false;
                    motionBegan = now;
                    heldMotion = 0;
                    backgroundBegan = now;
                    heldBackground = 0;
                    backgroundPaused = false;
                    yawOffset = 0;
                    paused = false;
                }
            }
            if (phase == ViewerPhase::FadeIn && now - began >= fadeDuration) {
                phase = ViewerPhase::Stable;
                entryAudio = false;
                if (!active) {
                    automaticEntry = false;
                }
            }
            if (!eligible) {
                lastActivity = now;
            } else if (!active && !transitioning() && now - lastActivity >= idleDelay) {
                request(true, true, now);
            }
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

        void syncBackground(double now)
        {
            const bool freeze = paused && !settings.animate;
            if (backgroundPaused != freeze) {
                heldBackground = backgroundElapsed(now);
                backgroundBegan = now;
                backgroundPaused = freeze;
            }
        }

        const ViewerOrbitProfile& orbitProfile() const
        {
            const size_t scene = size_t(std::clamp(int(settings.scene), 0, int(viewerOrbitProfiles.size()) - 1));
            return viewerOrbitProfiles[scene];
        }

        float yaw(double now) const
        {
            const auto& profile = orbitProfile();
            const double phase = (profile.frontTime - std::fmod(elapsed(now), profile.duration)) / profile.duration;
            return float(phase) * fullTurn + yawOffset;
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
