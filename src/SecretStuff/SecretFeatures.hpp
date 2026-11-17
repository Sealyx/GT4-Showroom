#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gt4
{
    struct FailRect
    {
        float x = 0, y = 0, w = 0, h = 0;

        bool contains(float px, float py) const
        {
            return px >= x && py >= y && px < x + w && py < y + h;
        }
    };

    inline float failSmooth(float v)
    {
        v = std::clamp(v, 0.f, 1.f);
        return v * v * (3 - 2 * v);
    }

    struct SecretFeaturesLayout
    {
        FailRect scene, label, word, buttons[2];
        float labelAlpha = 0, wordAlpha = 0, buttonAlpha = 0, backgroundAlpha = 1, buttonTextHeight = 0;

        SecretFeaturesLayout(float w, float h, double age)
        {
            const float t = failSmooth(float((age - .10) / .55));
            auto mix = [&](float a, float b) {
                return a + (b - a) * t;
            };
            scene = {mix(0, w * .0990f), mix(0, h * .2565f), mix(w, w * .4675f), mix(h, h * .4710f)};
            label = {w * (1166.f / 1920), h * (315.f / 1080), h * (214.f / 1080), h * (40.f / 1080)};
            word = {w * (1168.f / 1920), h * (409.f / 1080), h * (432.f / 1080), h * (117.f / 1080)};
            buttons[0] = {w * (1178.f / 1920), h * (671.f / 1080), w * (434.f / 1920), h * (50.f / 1080)};
            buttons[1] = {w * (1178.f / 1920), h * (742.f / 1080), w * (434.f / 1920), h * (50.f / 1080)};
            labelAlpha = failSmooth(float((age - .63) / .19));
            wordAlpha = failSmooth(float((age - .93) / .24));
            buttonAlpha = failSmooth(float((age - 2.05) / .20));
            buttonTextHeight = h * .0235f;
        }

        int hit(float x, float y) const
        {
            if (buttonAlpha < .99f) {
                return -1;
            }
            for (int i = 0; i < 2; ++i) {
                if (buttons[i].contains(x, y)) {
                    return i;
                }
            }
            return -1;
        }
    };

    struct SecretFeaturesInput
    {
        bool up = false, down = false, accept = false, back = false, click = false;
        bool mouseMoved = false;
        float mouseX = 0, mouseY = 0;
    };
    enum class SecretFeaturesChoice
    {
        None,
        Retry,
        Exit
    };
    enum class FailureKind
    {
        School,
        Wasted,
        Busted
    };

    struct FailureText
    {
        const char* label;
        const char* word;
    };

    inline FailureText failureText(FailureKind kind)
    {
        switch (kind) {
            case FailureKind::Wasted:
                return {"Wasted", "YOU DIED"};
            case FailureKind::Busted:
                return {"Busted", "ARRESTED"};
            default:
                return {"License", "FAIL"};
        }
    }

    class SecretFeaturesState
    {
        bool previousUp = false, previousDown = false, previousAccept = false, previousBack = false,
             previousClick = false;
        bool quarantine = true, docking = false;

    public:
        bool active = false, accepted = false;
        int selected = 0, test = 1;
        double age = 0;
        float pointerX = 0, pointerY = 0;
        bool pointerInitialized = false;
        uint64_t serial = 0;
        FailureKind kind = FailureKind::School;

        void begin(int n, FailureKind k = FailureKind::School)
        {
            active = true;
            accepted = false;
            selected = 0;
            test = std::clamp(n, 1, 16);
            kind = k;
            age = 0;
            quarantine = true;
            pointerInitialized = false;
            docking = false;
            ++serial;
        }

        void reset()
        {
            active = accepted = false;
            kind = FailureKind::School;
            age = 0;
            quarantine = true;
            docking = false;
            pointerInitialized = false;
            previousUp = previousDown = previousAccept = previousBack = previousClick = false;
        }

        bool interactive() const
        {
            return true;
        }

        bool ready() const
        {
            return interactive() && active && !accepted && age >= 2.25;
        }

        void suspended()
        {
            quarantine = true;
        }

        SecretFeaturesChoice update(double dt, const SecretFeaturesInput& i, float w, float h)
        {
            if (!active) {
                return SecretFeaturesChoice::None;
            }
            age += std::clamp(dt, 0., .10);
            if (!interactive()) {
                return SecretFeaturesChoice::None;
            }
            const SecretFeaturesLayout l(w, h, age);
            if (!pointerInitialized) {
                pointerX = l.buttons[0].x + l.buttons[0].w * .5f;
                pointerY = l.buttons[0].y + l.buttons[0].h * .5f;
                pointerInitialized = true;
            }
            const bool any = i.up || i.down || i.accept || i.back || i.click;
            const bool u = i.up && !previousUp, d = i.down && !previousDown, a = i.accept && !previousAccept,
                       b = i.back && !previousBack, c = i.click && !previousClick;
            previousUp = i.up;
            previousDown = i.down;
            previousAccept = i.accept;
            previousBack = i.back;
            previousClick = i.click;
            if (quarantine) {
                if (!any) {
                    quarantine = false;
                }
                return SecretFeaturesChoice::None;
            }
            if (i.mouseMoved) {
                pointerX = std::clamp(i.mouseX, 0.f, w);
                pointerY = std::clamp(i.mouseY, 0.f, h);
                const int hit = l.hit(pointerX, pointerY);
                if (hit >= 0) {
                    selected = hit;
                    docking = false;
                }
            }
            if (u || d || a || b) {
                docking = true;
                if (u || d) {
                    selected = 1 - selected;
                }
            }
            if (docking) {
                const auto& r = l.buttons[selected];
                const float blend = 1 - std::exp(-24.f * float(std::max(0., dt)));
                pointerX += (r.x + r.w * .5f - pointerX) * blend;
                pointerY += (r.y + r.h * .5f - pointerY) * blend;
            }
            if (!ready()) {
                return SecretFeaturesChoice::None;
            }
            if (b) {
                selected = 1;
                accepted = true;
                return SecretFeaturesChoice::Exit;
            }
            if (a || (c && l.hit(pointerX, pointerY) >= 0)) {
                if (c) {
                    selected = l.hit(pointerX, pointerY);
                }
                accepted = true;
                return selected ? SecretFeaturesChoice::Exit : SecretFeaturesChoice::Retry;
            }
            return SecretFeaturesChoice::None;
        }
    };
}

#include <array>
#include <cstdint>
#include <cstring>

namespace gt4
// a lot of the handling for how i replaced wasted & busted screens is all here
{
    struct SecretFeaturesKnownPatches
    {
        template <size_t N>
        static bool normalize(uint8_t* b,
                              size_t size,
                              size_t off,
                              const std::array<uint8_t, N>& before,
                              const std::array<uint8_t, N>& after)
        {
            if (off > size || N > size - off) {
                return false;
            }
            if (!std::memcmp(b + off, before.data(), N)) {
                return true;
            }
            if (std::memcmp(b + off, after.data(), N)) {
                return false;
            }
            std::memcpy(b + off, before.data(), N);
            return true;
        }

        static bool silentPatch(uint8_t* b, size_t size)
        {
            static constexpr std::array<uint8_t, 43> original = {0x04, 0x00, 0x02, 0x20, 0x03, 0x04, 0x00, 0xD6, 0x00,
                                                                 0x04, 0x00, 0x1A, 0x00, 0x04, 0x2E, 0x02, 0x20, 0x03,
                                                                 0x4D, 0x00, 0x01, 0x60, 0x75, 0xFF, 0xFF, 0xBE, 0x00,
                                                                 0x08, 0x01, 0x07, 0x24, 0x03, 0x20, 0x03, 0x2E, 0x80,
                                                                 0x08, 0x00, 0x02, 0x20, 0x03, 0x04, 0x01},
                                                     patched = {0x02, 0x00, 0x01, 0x8B, 0x75, 0xFF, 0xFF, 0xD6, 0x00,
                                                                0x04, 0x00, 0x18, 0x00, 0x02, 0x20, 0x03, 0x04, 0x00,
                                                                0x4D, 0x00, 0x01, 0x60, 0x75, 0xFF, 0xFF, 0xBE, 0x00,
                                                                0x0C, 0x00, 0x02, 0x20, 0x03, 0x04, 0x01, 0x08, 0x01,
                                                                0x07, 0x24, 0x03, 0x20, 0x03, 0x2E, 0x80};
            static constexpr std::array<uint8_t, 7> coneOriginal = {0x1A, 0, 4, 23, 2, 0x20, 3},
                                                    conePatched = {0x1A, 0, 4, 30, 2, 0x20, 3};
            static constexpr std::array<uint8_t, 7> lapOriginal = {0x1A, 0, 4, 35, 2, 0x20, 3},
                                                    lapPatched = {0x1A, 0, 4, 42, 2, 0x20, 3};
            return normalize(b, size, 35438, original, patched) &&
                   normalize(b, size, 45886, coneOriginal, conePatched) &&
                   normalize(b, size, 40359, lapOriginal, lapPatched);
        }
    };
}

#include <array>
#include <cstring>
#include <cstddef>

namespace gt4
{
    struct SecretFeaturesProfile
    {
        static constexpr size_t bytes = 46648;
        static constexpr uint32_t fnv = 0x68EAB6D8;
        static constexpr size_t resultSub = 33258, resultEnd = 35069, choiceButton = 35073;
        static constexpr size_t testGlobal = 988, scoreGlobal = 1084, leaveGlobal = 1300, confirmGlobal = 1304;

        static uint32_t hash(const uint8_t* b, size_t n)
        {
            uint32_t h = 2166136261u;
            for (size_t i = 0; i < n; ++i) {
                h = (h ^ b[i]) * 16777619u;
            }
            return h;
        }

        static bool match(const uint8_t* b, size_t n)
        {
            if (!b || n < bytes) {
                return false;
            }
            if (hash(b, bytes) == fnv) {
                return true;
            }
            std::array<uint8_t, bytes> normalized{};
            std::memcpy(normalized.data(), b, bytes);
            return SecretFeaturesKnownPatches::silentPatch(normalized.data(), normalized.size()) &&
                   hash(normalized.data(), normalized.size()) == fnv;
        }
    };

    struct SecretFeaturesCommand
    {
        uintptr_t owner = 0, base = 0, ip = 0;
        const uint8_t* mission = nullptr;
        size_t length = 0;
        bool active = false, external = false, isMission = false;
        std::array<char, 8> name{};
        int test = 0, score = 100, leave = 0, testRunning = 0;
    };

    struct SecretFeaturesSkip
    {
        bool consume = false, compareFalse = false;
        size_t next = 0;
    };

    inline size_t secretFeaturesArgumentEnd(const uint8_t* b, size_t n, size_t at, int count)
    {
        for (int j = 0; j < count; ++j) {
            if (at >= n) {
                return 0;
            }
            const unsigned t = b[at++];
            size_t size = 0;
            switch (t) {
                case 1:
                case 6:
                    size = 4;
                    break;
                case 2:
                case 3:
                case 5:
                case 10:
                case 11:
                case 16:
                case 17:
                    size = 2;
                    break;
                case 4:
                    size = 1;
                    break;
                case 7:
                case 8:
                case 12:
                case 13:
                case 18:
                case 19:
                    size = 6;
                    break;
                case 9:
                    size = 8;
                    break;
                case 15:
                    size = 16;
                    break;
                case 14:
                    if (at >= n) {
                        return 0;
                    }
                    size = 1 + b[at];
                    break;
                default:
                    return 0;
            }
            if (size > n - at) {
                return 0;
            }
            at += size;
        }
        return at;
    }

    class SecretFeaturesSchoolScript
    {
        uintptr_t verifiedBase = 0, owner = 0, rejectedBase = 0;
        uint64_t touched = 0;
        bool latched = false;
        bool verified = false, testArmed = false, recordStopped = false;
        uint64_t observed = 0;

    public:
        SecretFeaturesState state;
        bool enabled = true, assetsReady = false;
        bool mismatch = false;
        bool recording = false;
        uint64_t recordSerial = 0;

        uintptr_t ownerAddress() const
        {
            return owner;
        }

        bool owns(uintptr_t p) const
        {
            return verified && owner == p;
        }

        bool owns(uintptr_t p, uintptr_t base) const
        {
            return verified && owner == p && verifiedBase == base;
        }

        bool isVerified() const
        {
            return verified;
        }

        bool needsScene() const
        {
            return recording || (testArmed && recordStopped) || state.active;
        }

        void reset()
        {
            verifiedBase = owner = rejectedBase = 0;
            touched = 0;
            latched = false;
            verified = false;
            testArmed = recordStopped = recording = false;
            observed = 0;
            recordSerial = 0;
            mismatch = false;
            state.reset();
        }

        void stop()
        {
            owner = verifiedBase = 0;
            verified = false;
            touched = observed = 0;
            latched = testArmed = recordStopped = recording = false;
            state.reset();
        }

        void suspended(uint64_t now)
        {
            if (verified) {
                if (touched) {
                    touched = now;
                }
                if (observed) {
                    observed = now;
                }
            }
        }

        void expire(uint64_t now)
        {
            const uint64_t latest = std::max(touched, observed);
            if (latest && now - latest > 500) {
                stop();
            }
        }

        SecretFeaturesSkip inspect(const SecretFeaturesCommand& c, uint64_t now)
        {
            SecretFeaturesSkip out;
            if (!enabled || !assetsReady || !c.active || c.external || !c.isMission ||
                std::memcmp(c.name.data(), "DSKOOL\0\0", 8)) {
                return out;
            }
            if (c.base != verifiedBase) {
                if (c.base == rejectedBase) {
                    return out;
                }
                verified = SecretFeaturesProfile::match(c.mission, c.length);
                if (!verified) {
                    rejectedBase = c.base;
                    mismatch = true;
                    return out;
                }
                verifiedBase = c.base;
                mismatch = false;
            }
            if (!verified || c.ip < c.base || c.ip - c.base + 2 > SecretFeaturesProfile::bytes) {
                return out;
            }
            observed = now;
            const size_t at = size_t(c.ip - c.base);
            const auto* b = c.mission;
            const uint16_t op = uint16_t(b[at] | unsigned(b[at + 1]) << 8) & 0x7FFF;
            if (op == 0x0050 && at + 7 <= SecretFeaturesProfile::bytes && b[at + 2] == 1) {
                int32_t target = 0;
                std::memcpy(&target, b + at + 3, 4);
                if (target == -31041) {
                    state.reset();
                    latched = false;
                    owner = c.owner;
                    testArmed = true;
                    recordStopped = false;
                    recording = false;
                    ++recordSerial;
                }
                if (target == -32505) {
                    recordStopped = true;
                    recording = false;
                }
                if (target == -int32_t(SecretFeaturesProfile::resultSub)) {
                    if (c.score < 70 && c.test >= 1 && c.test <= 16 && c.leave == 0) {
                        touched = now;
                        owner = c.owner;
                        if (!latched) {
                            state.begin(c.test);
                            latched = true;
                        }
                    } else {
                        testArmed = recordStopped = recording = false;
                        if (state.active) {
                            state.reset();
                            latched = false;
                            owner = 0;
                        }
                    }
                }
            }
            recording = testArmed && !recordStopped && !state.active && c.testRunning == 1 && c.leave == 0;
            if (!state.active || owner != c.owner) {
                return out;
            }
            if (at >= SecretFeaturesProfile::resultSub && at < SecretFeaturesProfile::resultEnd) {
                int args = -1;
                switch (op) {
                    case 0x0937:
                        args = 6;
                        break;
                    case 0x033E:
                        args = 3;
                        break;
                    case 0x045A:
                        args = 4;
                        break;
                    case 0x045B:
                        args = 5;
                        break;
                    case 0x038D:
                        args = 9;
                        break;
                    default:
                        break;
                }
                if (args >= 0) {
                    const size_t end = secretFeaturesArgumentEnd(b, SecretFeaturesProfile::bytes, at + 2, args);
                    if (end) {
                        out.consume = true;
                        out.next = end;
                    }
                }
            }
            if (at == SecretFeaturesProfile::choiceButton && op == 0x00E1) {
                out.next = secretFeaturesArgumentEnd(b, SecretFeaturesProfile::bytes, at + 2, 2);
                out.consume = out.next != 0;
                out.compareFalse = out.consume;
            }
            return out;
        }

        template <class Write> bool choose(SecretFeaturesChoice choice, Write write)
        {
            if (choice == SecretFeaturesChoice::None || !verified || !state.active || !state.accepted || !owner) {
                return false;
            }
            if (choice == SecretFeaturesChoice::Exit && !write(SecretFeaturesProfile::leaveGlobal, 2)) {
                return false;
            }
            return write(SecretFeaturesProfile::confirmGlobal, 1);
        }
    };
}

#include "RuntimeModels.hpp"
#include <windows.h>

namespace gt4
{
    class SecretFeaturesSchoolNative
    {
        uintptr_t copiedBase = 0;
        bool uiRequested = false;
        std::array<uint8_t, SecretFeaturesProfile::bytes> copy{};
        static constexpr uintptr_t globals = 0xA49960;

        static bool writable(uintptr_t address, size_t size)
        {
            MEMORY_BASIC_INFORMATION i{};
            if (!VirtualQuery(reinterpret_cast<void*>(address), &i, sizeof(i)) || i.State != MEM_COMMIT ||
                (i.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
                return false;
            }
            const DWORD p = i.Protect & 255;
            return (p == PAGE_READWRITE || p == PAGE_WRITECOPY || p == PAGE_EXECUTE_READWRITE ||
                    p == PAGE_EXECUTE_WRITECOPY) &&
                   address >= uintptr_t(i.BaseAddress) && size <= i.RegionSize - (address - uintptr_t(i.BaseAddress));
        }

        bool snapshot(void* self, SecretFeaturesCommand& c)
        {
            const uintptr_t s = uintptr_t(self);
            c.owner = s;
            if (!gta::reader(s + 8, c.name.data(), 8) || std::memcmp(c.name.data(), "DSKOOL\0\0", 8)) {
                return false;
            }
            uint8_t active = 0, external = 0, mission = 0;
            uintptr_t ip = 0;
            if (!gta::read(s + 0x10, c.base) || !gta::read(s + 0x14, ip) || ip < 2 || !gta::read(s + 0xC4, active) ||
                !gta::read(s + 0xC7, external) || !gta::read(s + 0xDC, mission)) {
                return false;
            }
            c.active = active != 0;
            c.external = external != 0;
            c.isMission = mission != 0;
            c.ip = ip - 2;
            if (!c.active || c.external || !c.isMission) {
                return false;
            }
            if (c.base != copiedBase) {
                if (!gta::reader(c.base, copy.data(), copy.size())) {
                    return false;
                }
                copiedBase = c.base;
            }
            c.mission = copy.data();
            c.length = copy.size();
            return gta::read(globals + SecretFeaturesProfile::testGlobal, c.test) &&
                   gta::read(globals + SecretFeaturesProfile::scoreGlobal, c.score) &&
                   gta::read(globals + SecretFeaturesProfile::leaveGlobal, c.leave) &&
                   gta::read(globals + 996, c.testRunning);
        }

    public:
        SecretFeaturesSchoolScript script;

        void reset()
        {
            script.reset();
            copiedBase = 0;
            uiRequested = false;
        }

        // ordinary hud draws shouldn't pay for a font atlas that nobody is using
        bool needsUiAssets() const
        {
            return script.enabled && !script.assetsReady && (uiRequested || script.needsScene());
        }

        bool command(void* self, int command, uint64_t now)
        {
            if (!script.enabled) {
                return false;
            }
            if (command != 0x0050 && command != 0x00E1 && command != 0x0937 && command != 0x033E && command != 0x045A &&
                command != 0x045B && command != 0x038D) {
                return false;
            }
            SecretFeaturesCommand c;
            if (!snapshot(self, c)) {
                return false;
            }
            const size_t at = c.ip >= c.base ? size_t(c.ip - c.base) : copy.size();
            int args = 0;
            switch (command) {
                case 0x0050:
                    args = 1;
                    break;
                case 0x00E1:
                    args = 2;
                    break;
                case 0x0937:
                    args = 6;
                    break;
                case 0x033E:
                    args = 3;
                    break;
                case 0x045A:
                    args = 4;
                    break;
                case 0x045B:
                    args = 5;
                    break;
                case 0x038D:
                    args = 9;
                    break;
                default:
                    return false;
            }
            if (at + 2 > copy.size()) {
                return false;
            }
            const size_t end = secretFeaturesArgumentEnd(copy.data(), copy.size(), at + 2, args);
            std::array<uint8_t, 256> live{};
            if (!end || end - at > live.size() || !gta::reader(c.ip, live.data(), end - at) ||
                std::memcmp(live.data(), copy.data() + at, end - at)) {
                reset();
                script.mismatch = true;
                return false;
            }
            if (!script.assetsReady) {
                uiRequested = SecretFeaturesProfile::match(c.mission, c.length);
                return false;
            }
            const auto skip = script.inspect(c, now);
            if (!skip.consume) {
                return false;
            }
            const uintptr_t next = c.base + skip.next;
            if (!writable(uintptr_t(self) + 0x14, sizeof(uintptr_t))) {
                return false;
            }
            std::memcpy(static_cast<uint8_t*>(self) + 0x14, &next, sizeof(next));
            if (skip.compareFalse) {
                reinterpret_cast<void(__thiscall*)(void*, bool)>(0x4859D0)(self, false);
            }
            return true;
        }

        bool choose(SecretFeaturesChoice choice)
        {
            if (!script.state.active || !writable(globals + SecretFeaturesProfile::leaveGlobal, sizeof(int)) ||
                !writable(globals + SecretFeaturesProfile::confirmGlobal, sizeof(int))) {
                return false;
            }
            SecretFeaturesCommand c;
            if (!snapshot(reinterpret_cast<void*>(script.ownerAddress()), c) || !script.owns(c.owner, c.base) ||
                !gta::reader(c.base, copy.data(), copy.size()) ||
                !SecretFeaturesProfile::match(copy.data(), copy.size()) || c.test != script.state.test ||
                c.score >= 70 || c.leave != 0) {
                return false;
            }
            return script.choose(choice, [&](size_t offset, int value) {
                if (!writable(globals + offset, sizeof(value))) {
                    return false;
                }
                std::memcpy(reinterpret_cast<void*>(globals + offset), &value, sizeof(value));
                return true;
            });
        }
    };
}
#include "Renderer.hpp"

#include "ResultButtonArt.hpp"

namespace gt4
{
    class SecretFeaturesRenderer
    {
        IDirect3DTexture9* scene = nullptr;
        UINT width = 0, height = 0;
        D3DFORMAT sceneFormat = D3DFMT_UNKNOWN;
        uint64_t capturedFrame = UINT64_MAX, drawnFrame = UINT64_MAX;
        bool cleanWorld = false;

        static void button(IDirect3DDevice9* d, Renderer& r, const FailRect& b, bool selected, float alpha)
        {
            const auto& profile = selected ? resultButtonGold : resultButtonGray;
            const float radius = std::min(b.h * .5f, b.w * .5f);
            for (size_t y = 0; y < profile.size(); ++y) {
                const float t = (float(y) + .5f) / float(profile.size()), dy = b.h * (t - .5f);
                const float inset = radius - std::sqrt(std::max(0.f, radius * radius - dy * dy));
                const float top = b.y + b.h * float(y) / float(profile.size()),
                            bottom = b.y + b.h * float(y + 1) / float(profile.size());
                r.rect(d, b.x + inset, top, b.w - 2 * inset, bottom - top, color(profile[y], alpha));
            }
        }

    public:
        ~SecretFeaturesRenderer()
        {
            reset();
        }

        SecretFeaturesRenderer() = default;
        SecretFeaturesRenderer(const SecretFeaturesRenderer&) = delete;
        SecretFeaturesRenderer& operator=(const SecretFeaturesRenderer&) = delete;

        bool ready() const
        {
            return true;
        }

        void reset()
        {
            releaseScene();
        }

        void releaseScene()
        {
            if (scene) {
                scene->Release();
            }
            scene = nullptr;
            width = height = 0;
            sceneFormat = D3DFMT_UNKNOWN;
            capturedFrame = drawnFrame = UINT64_MAX;
            cleanWorld = false;
        }

        bool captureWorld(IDirect3DDevice9* d, uint64_t frame)
        {
            if (!d || capturedFrame == frame) {
                return cleanWorld && capturedFrame == frame;
            }
            cleanWorld = false;
            IDirect3DSurface9* current = nullptr;
            if (FAILED(d->GetRenderTarget(0, &current))) {
                return false;
            }
            D3DSURFACE_DESC desc{};
            if (FAILED(current->GetDesc(&desc)) || !desc.Width || !desc.Height) {
                current->Release();
                return false;
            }
            if (!scene || width != desc.Width || height != desc.Height || sceneFormat != desc.Format) {
                releaseScene();
                if (FAILED(d->CreateTexture(desc.Width,
                                            desc.Height,
                                            1,
                                            D3DUSAGE_RENDERTARGET,
                                            desc.Format,
                                            D3DPOOL_DEFAULT,
                                            &scene,
                                            nullptr))) {
                    current->Release();
                    return false;
                }
                width = desc.Width;
                height = desc.Height;
                sceneFormat = desc.Format;
            }
            IDirect3DSurface9* copy = nullptr;
            if (FAILED(scene->GetSurfaceLevel(0, &copy))) {
                current->Release();
                return false;
            }
            DWORD srgb = FALSE;
            d->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgb);
            d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            const HRESULT copied = d->StretchRect(current, nullptr, copy, nullptr, D3DTEXF_NONE);
            d->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb);
            copy->Release();
            current->Release();
            capturedFrame = frame;
            cleanWorld = SUCCEEDED(copied);
            return cleanWorld;
        }

        bool hasWorld(uint64_t frame) const
        {
            return cleanWorld && scene && capturedFrame == frame;
        }

        bool draw(IDirect3DDevice9* d, Renderer& r, const SecretFeaturesState& s, uint64_t frame)
        {
            if (!d || !s.active || !hasWorld(frame) || drawnFrame == frame) {
                return false;
            }
            RenderTargets targets(d);
            if (!targets.valid()) {
                return false;
            }
            r.ensureUiAssets(d);
            if (!r.uiTextReady()) {
                return false;
            }
            const auto vp = targets.view();
            IDirect3DTexture9* shown = scene;
            IDirect3DStateBlock9* saved = nullptr;
            if (FAILED(d->CreateStateBlock(D3DSBT_ALL, &saved)) || !saved) {
                return false;
            }
            if (FAILED(saved->Capture()) || !targets.isolate()) {
                saved->Release();
                return false;
            }
            d->SetPixelShader(nullptr);
            d->SetVertexShader(nullptr);
            d->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
            d->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            d->SetRenderState(D3DRS_DITHERENABLE, FALSE);
            d->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
            d->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
            d->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
            d->SetRenderState(D3DRS_ZENABLE, FALSE);
            d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            d->SetRenderState(D3DRS_FOGENABLE, FALSE);
            d->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
            d->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
            d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            d->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            d->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT);
            d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            d->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            const float w = float(vp.Width), h = float(vp.Height);
            const SecretFeaturesLayout l(w, h, s.age);
            const auto words = failureText(s.kind);
            r.rect(d, 0, 0, w, h, 0xFF000000);
            r.rect(d, l.scene.x, l.scene.y, l.scene.w, l.scene.h, 0xFFFFFFFF, shown);
            const float maxWidth = std::min(w * .34f, w - l.word.x - w * .035f);
            r.label(d,
                    words.label,
                    l.label.x,
                    l.label.y,
                    r.fitted(words.label, l.label.h, maxWidth, true),
                    l.labelAlpha,
                    false,
                    0,
                    true);
            r.text(d,
                   words.word,
                   l.word.x,
                   l.word.y,
                   r.fitted(words.word, l.word.h, maxWidth, true),
                   color(0xFFFFFF, l.wordAlpha),
                   false,
                   0,
                   true);
            if (s.interactive()) {
                for (int n = 0; n < 2; ++n) {
                    const auto& b = l.buttons[n];
                    button(d, r, b, n == s.selected, l.buttonAlpha);
                    r.text(d,
                           n ? "EXIT" : "RETRY",
                           b.x + b.w * .5f,
                           b.y + b.h * .22f,
                           r.fitted(n ? "EXIT" : "RETRY", l.buttonTextHeight, b.w * .82f, true),
                           color(0x101010, l.buttonAlpha),
                           true,
                           0,
                           true);
                }
                if (l.buttonAlpha > .01f) {
                    r.cursor(d, s.pointerX, s.pointerY, h / 720.f, l.buttonAlpha);
                }
            }
            saved->Apply();
            const bool restored = targets.restore();
            saved->Release();
            if (!restored) {
                return false;
            }
            drawnFrame = frame;
            return true;
        }
    };
}
#include <windows.h>
#include <string>

namespace gt4
{
    struct SecretFeatures
    {
        bool enabled = true;
    };

    inline SecretFeatures readSecretFeatures(const std::wstring& ini)
    {
        return {GetPrivateProfileIntW(L"SecretFeatures", L"Enabled", 1, ini.c_str()) != 0};
    }
}


namespace gt4
{
    struct SecretFeaturesRecoveryState
    {
        SecretFeaturesState view;
        uint32_t event = 0;
        bool music = false, resuming = false, drawn = false, quitPosted = false;
        uint64_t heartbeat = 0, presented = 0, began = 0;
        SecretFeaturesChoice pending = SecretFeaturesChoice::None;
        double closing = 0;

        void reset()
        {
            view.reset();
            event = 0;
            music = resuming = drawn = quitPosted = false;
            heartbeat = presented = began = 0;
            pending = SecretFeaturesChoice::None;
            closing = 0;
        }

        bool matches(uint8_t nativeState, uint32_t stamp) const
        {
            return (nativeState == 1 || nativeState == 2) && event == stamp &&
                   view.kind == (nativeState == 1 ? FailureKind::Wasted : FailureKind::Busted);
        }

        bool observe(uint8_t nativeState, uint32_t stamp, bool enabled, uint64_t now)
        {
            if (!enabled || (nativeState != 1 && nativeState != 2)) {
                reset();
                return false;
            }
            const auto kind = nativeState == 1 ? FailureKind::Wasted : FailureKind::Busted;
            if ((!view.active && !resuming) || !matches(nativeState, stamp)) {
                reset();
                view.begin(1, kind);
                event = stamp;
                began = now;
            }
            heartbeat = now;
            return view.active;
        }

        bool hold(uint64_t now, uint8_t nativeState, uint32_t stamp, bool enabled, bool suspended) const
        {
            const bool startup = !drawn && now >= began && now - began <= 1200 && !suspended;
            const bool live = drawn && (suspended || (now >= presented && now - presented <= 500));
            return enabled && view.active && !resuming && matches(nativeState, stamp) && now >= heartbeat &&
                   now - heartbeat <= 350 && (startup || live);
        }

        void present(uint64_t now, bool ok)
        {
            drawn = ok;
            if (ok) {
                presented = now;
            }
        }

        void resume()
        {
            resuming = true;
            view.active = false;
            drawn = false;
        }
    };
}
