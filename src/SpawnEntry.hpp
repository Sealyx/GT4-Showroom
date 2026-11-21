#pragma once
#include "Game.hpp"
#include "Showroom.hpp"

namespace gt4
{
    struct EntryCameraPose
    {
        V3 source, target, upOffset;
        float fov = 50;
    };

    inline float entrySmooth(float x)
    {
        x = std::clamp(x, 0.f, 1.f);
        return x * x * x * (x * (x * 6 - 15) + 10);
    }

    inline EntryCameraPose entryCameraPose(double age, V3 low, V3 high)
    {
        const float t = float(std::clamp(age, 0., 3.)), settle = entrySmooth((t - .55f) / 2.45f),
                    pan = entrySmooth(t / .95f);
        const float length = std::max(1.f, high.y - low.y), width = std::max(.65f, high.x - low.x),
                    height = std::max(.5f, high.z - low.z), scale = std::max(length / 4.7f, width / 2.1f);
        const V3 center = (low + high) * .5f;
        const float distance = scale * (4.05f + 1.15f * settle);
        EntryCameraPose p;
        p.source = {center.x + distance * .74f, center.y + distance * .82f, low.z + height * (.48f + .25f * settle)};
        p.target = {center.x - (1 - pan) * 5.8f * scale, center.y + length * .05f, low.z + height * .49f};
        const float envelope = entrySmooth(t / .30f), motionTime = float(std::max(0., age));
        const float dx = envelope * (.025f * std::sin(3.1f * motionTime + .2f) + .01f * std::sin(7.7f * motionTime));
        const float dy =
            envelope * (.018f * std::sin(2.7f * motionTime - .6f) + .007f * std::sin(6.1f * motionTime + .7f));
        const float roll =
            envelope * (.0035f * std::sin(2.6f * motionTime) + .0011f * std::sin(5.4f * motionTime + .3f));
        const V3 direction = unit(p.target - p.source), right = unit(cross(direction, V3{0, 0, 1})),
                 up = unit(cross(right, direction));
        p.source = p.source + right * (dx * scale * .35f) + up * (dy * scale * .3f);
        p.target = p.target + right * (dx * scale) + up * (dy * scale);
        p.upOffset = right * std::tan(roll);
        return p;
    }

    struct EntryPlayback
    {
        bool active = false;
        double age = 0;

        void begin()
        {
            active = true;
            age = 0;
        }

        bool step(double dt, bool paused, bool valid, bool seated, bool skipped, bool taskAlive)
        {
            if (!active) {
                return false;
            }
            if (!valid || skipped) {
                active = false;
                return false;
            }
            if (!paused) {
                age += std::clamp(dt, 0., .1);
            }
            if (age >= 12 || (age >= 3 && seated) || (age > .8 && !seated && !taskAlive)) {
                active = false;
            }
            return active;
        }
    };
}

namespace gt4
{
    namespace entryNative
    {
#ifdef GTA_ENTRY_TEST
        inline uintptr_t (*resolve)(uintptr_t) = nullptr;
#else
        inline constexpr uintptr_t resolve(uintptr_t address)
        {
            return address;
        }
#endif
        template <class F> F function(uintptr_t address)
        {
            return reinterpret_cast<F>(resolve(address));
        }
    }

    inline bool beginDriverEntry(void* ped, void* vehicle, void*& task)
    {
        void* intel = ped ? game::field<void*>(ped, 0x47C) : nullptr;
        if (!intel || !vehicle) {
            return false;
        }
        task = entryNative::function<void*(__cdecl*)(unsigned)>(0x61A5A0)(0x50);
        if (!task) {
            return false;
        }
        entryNative::function<void(__thiscall*)(void*, void*)>(0x6402F0)(task, vehicle);
        entryNative::function<void(__thiscall*)(void*, void*, int, bool)>(0x681AF0)(
            static_cast<uint8_t*>(intel) + 4, task, 3, false);
        return true;
    }

    inline bool seatPlayerAsDriver(void* ped, void* vehicle)
    {
        void* intel = ped ? game::field<void*>(ped, 0x47C) : nullptr;
        if (!intel || !vehicle || game::field<float>(ped, 0x540) <= 0 ||
            game::field<void*>(vehicle, 0x460)) {
            return false;
        }

        entryNative::function<void(__thiscall*)(void*, bool)>(0x601640)(intel, false);
        runDriverTask(ped,
                      vehicle,
                      entryNative::function<DriverCtor>(0x6470E0),
                      entryNative::function<DriverProcess>(0x64B950));
        return game::field<void*>(vehicle, 0x460) == ped && game::field<void*>(ped, 0x58C) == vehicle;
    }

    struct SpawnEntry
    {
        EntryPlayback playback;
        int pedRef = -1, vehicleRef = -1, model = -1;
        void* entryTask = nullptr;
        bool cameraOwned = false, inputReleased = false;
        V3 low, high, lastSource, lastTarget, lastUp;
        uint32_t nativeFrames = 0;
        float desiredFov = 50;
        const char* lastStop = "not started";

        void* camera() const
        {
            return reinterpret_cast<void*>(0xB6F028);
        }

        bool ownsCamera() const
        {
            if (!cameraOwned) {
                return false;
            }
            void* c = camera();
            if (game::field<uint32_t>(c, 0xB0) != 1 || !game::field<uint8_t>(c, 0x2C)) {
                return false;
            }
            return dot(game::field<V3>(c, 0x86C) - lastSource, game::field<V3>(c, 0x86C) - lastSource) < .01f &&
                   dot(game::field<V3>(c, 0x860) - lastTarget, game::field<V3>(c, 0x860) - lastTarget) < .01f;
        }

        void stop(bool reset = false)
        {
            const bool restore = ownsCamera();
            playback.active = false;
            cameraOwned = false;
            pedRef = vehicleRef = model = -1;
            entryTask = nullptr;
            if (restore && !game::loading() && entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1)) {
                entryNative::function<void(__thiscall*)(void*)>(reset ? 0x50BAB0 : 0x50B930)(camera());
            }
        }

        V3 collisionSafe(V3 target, V3 source, void* vehicle)
        {
            alignas(4) std::array<uint8_t, 0x2C> hit{};
            void* entity = nullptr;
            void*& ignore = game::at<void*>(0xB7CD68);
            void* saved = ignore;
            ignore = vehicle;
            const bool blocked = entryNative::function<bool(__cdecl*)(
                const V3&, const V3&, void*, void**, bool, bool, bool, bool, bool, bool, bool, bool)>(0x56BA00)(
                target, source, hit.data(), &entity, true, false, false, true, true, false, true, false);
            ignore = saved;
            if (blocked) {
                const V3 point = game::field<V3>(hit.data(), 0);
                source = point + unit(target - point) * .25f;
            }
            bool ground = false;
            const float z = entryNative::function<float(__cdecl*)(float, float, float, bool*, void**)>(0x5696C0)(
                source.x, source.y, source.z + 2, &ground, nullptr);
            if (ground && source.z < z + .2f) {
                source.z = z + .2f;
            }
            return source;
        }

        void frame(void* vehicle)
        {
            const auto pose = entryCameraPose(playback.age, low, high);
            void* m = game::field<void*>(vehicle, 0x14);
            if (!m) {
                return;
            }
            const auto& matrix = *static_cast<game::Matrix*>(m);
            const V3 target = game::point(matrix, pose.target),
                     source = collisionSafe(target, game::point(matrix, pose.source), vehicle),
                     up = matrix.right * pose.upOffset.x + matrix.up * pose.upOffset.y +
                          matrix.forward * pose.upOffset.z;
            entryNative::function<void(__thiscall*)(void*, const V3*, const V3*)>(0x50BEC0)(camera(), &source, &up);
            game::field<V3>(camera(), 0x860) = target;
            lastSource = source;
            lastTarget = target;
            lastUp = up;
            desiredFov = pose.fov;
        }

        bool begin(void* vehicle, V3 boundsLow, V3 boundsHigh)
        {
            stop();
            void* p = entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1);
            if (!p || !vehicle) {
                lastStop = "missing player or vehicle";
                return false;
            }
            if (game::field<uint32_t>(camera(), 0xB0) == 1) {
                lastStop = "another scripted camera owns the view";
                return false;
            }
            if (!beginDriverEntry(p, vehicle, entryTask)) {
                lastStop = "native entry task unavailable";
                return false;
            }
            pedRef = entryNative::function<int(__cdecl*)(void*)>(0x54FF60)(p);
            vehicleRef = entryNative::function<int(__cdecl*)(void*)>(0x54FFC0)(vehicle);
            model = game::field<int16_t>(vehicle, 0x22);
            low = boundsLow;
            high = boundsHigh;
            inputReleased = false;
            nativeFrames = 0;
            playback.begin();
            frame(vehicle);
            const V3 target = lastTarget;
            entryNative::function<void(__thiscall*)(void*, const V3*, int16_t, int)>(0x50C8B0)(camera(), &target, 2, 1);
            cameraOwned = true;
            lastStop = "running";
            if (!ownsCamera()) {
                lastStop = "scripted camera control refused";
                stop();
                return false;
            }
            return true;
        }

        bool update(double dt, bool paused, bool skip)
        {
            if (!playback.active) {
                return false;
            }
            void* p = pedRef >= 0 ? entryNative::function<void*(__cdecl*)(int)>(0x54FF90)(pedRef) : nullptr;
            void* v = vehicleRef >= 0 ? entryNative::function<void*(__cdecl*)(int)>(0x54FFF0)(vehicleRef) : nullptr;
            const bool valid = !game::loading() && game::at<uint8_t>(0x96A8B0) == 0 && p &&
                               p == entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1) && v &&
                               game::field<int16_t>(v, 0x22) == model && game::field<float>(p, 0x540) > 0 &&
                               !game::flag(0xB5F851) && !game::flag(0xB5F852) && ownsCamera();
            if (!skip) {
                inputReleased = true;
            }
            const bool seated = valid && (game::field<uint32_t>(p, 0x46C) & 0x100) && game::field<void*>(p, 0x58C) == v;
            void* intel = valid ? game::field<void*>(p, 0x47C) : nullptr;
            const bool taskAlive = intel && game::field<void*>(intel, 4 + 3 * 4) == entryTask;
            if (!playback.step(dt, paused, valid, seated, skip && inputReleased, taskAlive)) {
                lastStop = !valid                  ? "entity/state/camera ownership changed"
                           : skip && inputReleased ? "skipped"
                           : seated                ? "driver seated"
                           : !taskAlive            ? "native entry task ended"
                                                   : "timeout";
                stop();
                return false;
            }
            if (!paused) {
                frame(v);
            }
            return true;
        }
    };

    inline bool updateEntryPresentation(SpawnEntry& entry, ReturnFade& reveal, double dt, bool paused, bool skip)
    {
        if (!paused) {
            reveal.step(dt);
        }
        const bool active = entry.update(dt, paused, skip);
        if (!active) {
            reveal.reset();
        }
        return active;
    }
}
