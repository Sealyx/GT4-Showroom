#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace gt4
{
    struct CinematicPoint
    {
        float x = 0, y = 0, z = 0;
    };

    inline CinematicPoint operator+(CinematicPoint a, CinematicPoint b)
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    inline CinematicPoint operator-(CinematicPoint a, CinematicPoint b)
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    inline CinematicPoint operator*(CinematicPoint a, float k)
    {
        return {a.x * k, a.y * k, a.z * k};
    }

    inline float cinematicDot(CinematicPoint a, CinematicPoint b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline CinematicPoint cinematicCross(CinematicPoint a, CinematicPoint b)
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    inline CinematicPoint cinematicUnit(CinematicPoint p)
    {
        const float n = std::sqrt(cinematicDot(p, p));
        return n > 1.e-6f ? p * (1.f / n) : CinematicPoint{0, 0, 1};
    }

    inline bool cinematicFinite(CinematicPoint p)
    {
        return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
    }

    inline CinematicPoint cinematicClosestPoint(CinematicPoint p, CinematicPoint a, CinematicPoint b, CinematicPoint c)
    {
        const auto ab = b - a, ac = c - a, ap = p - a;
        const float d1 = cinematicDot(ab, ap), d2 = cinematicDot(ac, ap);
        if (d1 <= 0 && d2 <= 0) {
            return a;
        }
        const auto bp = p - b;
        const float d3 = cinematicDot(ab, bp), d4 = cinematicDot(ac, bp);
        if (d3 >= 0 && d4 <= d3) {
            return b;
        }
        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0 && d1 >= 0 && d3 <= 0) {
            return a + ab * (d1 / (d1 - d3));
        }
        const auto cp = p - c;
        const float d5 = cinematicDot(ab, cp), d6 = cinematicDot(ac, cp);
        if (d6 >= 0 && d5 <= d6) {
            return c;
        }
        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0 && d2 >= 0 && d6 <= 0) {
            return a + ac * (d2 / (d2 - d6));
        }
        const float va = d3 * d6 - d5 * d4;
        if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
            return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
        }
        const float sum = va + vb + vc;
        if (std::abs(sum) < 1.e-12f) {
            return a;
        }
        return a + ab * (vb / sum) + ac * (vc / sum);
    }
    enum class CinematicComponent : uint8_t
    {
        Bonnet,
        Boot,
        FrontLeftDoor,
        FrontRightDoor,
        RearLeftDoor,
        RearRightDoor,
        FrontLeftWheel,
        FrontRightWheel,
        RearLeftWheel,
        RearRightWheel
    };
    inline constexpr size_t cinematicComponentCount = 10;
    inline constexpr std::array<int, cinematicComponentCount> cinematicNativeNodes{{16, 17, 10, 8, 11, 9, 5, 2, 7, 4}};
    inline constexpr std::array<const char*, cinematicComponentCount> cinematicComponentNames{{"Hood",
                                                                                               "Trunk",
                                                                                               "Left front door",
                                                                                               "Right front door",
                                                                                               "Left rear door",
                                                                                               "Right rear door",
                                                                                               "Left front wheel",
                                                                                               "Right front wheel",
                                                                                               "Left rear wheel",
                                                                                               "Right rear wheel"}};

    struct CinematicHotspot
    {
        int component = -1;
        float x = 0, y = 0, radius = 0;
    };

    struct CinematicHotspotList
    {
        std::array<CinematicHotspot, cinematicComponentCount> items{};
        size_t count = 0;

        int cycle(int current, int direction) const
        {
            if (!count) {
                return -1;
            }
            size_t at = count;
            for (size_t i = 0; i < count; ++i) {
                if (items[i].component == current) {
                    at = i;
                    break;
                }
            }
            if (at == count) {
                return items[direction < 0 ? count - 1 : 0].component;
            }
            return items[(at + count + (direction < 0 ? count - 1 : 1)) % count].component;
        }
    };

    struct CinematicPart
    {
        bool present = false;
        CinematicPoint pivot{}, anchor{}, axis{0, 0, 1}, outward{0, 0, 1};
        float openAngle = 0;
    };

    struct CinematicCapabilities
    {
        std::array<CinematicPart, cinematicComponentCount> parts{};
        bool allWheelSteering = false;

        bool supported(int component) const
        {
            return component >= 0 && component < int(parts.size()) && parts[size_t(component)].present &&
                   (component < 8 || allWheelSteering);
        }

        bool any() const
        {
            for (size_t i = 0; i < parts.size(); ++i) {
                if (supported(int(i))) {
                    return true;
                }
            }
            return false;
        }
    };

    struct CinematicVehicleState
    {
        bool hotspots = true;
        std::array<float, 6> doors{}, doorTargets{};
        float steering = 0, steeringTarget = 0;
        float rearSteering = 0, rearSteeringTarget = 0;
        int focus = -1, boundModel = -1, wheelDrag = -1;
        float wheelGrabX = 0, wheelGrabSteering = 0;

        void reset()
        {
            *this = CinematicVehicleState{};
        }

        void bind(int id)
        {
            if (boundModel != id) {
                const bool show = hotspots;
                reset();
                hotspots = show;
                boundModel = id;
            }
        }

        void step(double dt)
        {
            if (!std::isfinite(dt) || dt <= 0) {
                return;
            }
            const float k = float(1. - std::exp(-std::clamp(dt, 0., .25) * 9.));
            for (size_t i = 0; i < doors.size(); ++i) {
                doors[i] += (doorTargets[i] - doors[i]) * k;
                if (std::abs(doors[i] - doorTargets[i]) < .0005f) {
                    doors[i] = doorTargets[i];
                }
            }
            updateSteering(steering, steeringTarget, k);
            updateSteering(rearSteering, rearSteeringTarget, k);
        }

        static void updateSteering(float& angle, float target, float amount)
        {
            angle += (target - angle) * amount;
            if (std::abs(target - angle) < .0005f) {
                angle = target;
            }
        }

        float& steeringGoal(int component)
        {
            if (component >= int(CinematicComponent::RearLeftWheel)) {
                return rearSteeringTarget;
            }
            return steeringTarget;
        }

        float steeringAngle(int component) const
        {
            if (component >= int(CinematicComponent::RearLeftWheel)) {
                return rearSteering;
            }
            return steering;
        }

        void centerWheels()
        {
            steering = steeringTarget = 0;
            rearSteering = rearSteeringTarget = 0;
            endWheelDrag();
        }

        bool activate(int component, const CinematicCapabilities& caps)
        {
            if (!caps.supported(component)) {
                return false;
            }
            focus = component;
            if (component >= 6) {
                return false;
            }
            auto& t = doorTargets[size_t(component)];
            t = t > .5f ? 0.f : 1.f;
            return true;
        }

        void togglePanels(const CinematicCapabilities& caps)
        {
            bool allOpen = true;
            for (size_t i = 0; i < doorTargets.size(); ++i) {
                if (caps.supported(int(i)) && doorTargets[i] < .5f) {
                    allOpen = false;
                }
            }
            for (size_t i = 0; i < doorTargets.size(); ++i) {
                if (caps.supported(int(i))) {
                    doorTargets[i] = allOpen ? 0.f : 1.f;
                }
            }
        }

        bool beginWheelDrag(int component, const CinematicCapabilities& caps, float x)
        {
            if (component < 6 || !caps.supported(component) || !std::isfinite(x)) {
                return false;
            }
            wheelDrag = focus = component;
            wheelGrabX = x;
            wheelGrabSteering = steeringGoal(component);
            return true;
        }

        void dragWheel(float x, float width)
        {
            if (wheelDrag < 0 || !std::isfinite(x) || !std::isfinite(width) || width <= 0) {
                return;
            }
            auto& target = steeringGoal(wheelDrag);
            target = std::clamp(wheelGrabSteering + (x - wheelGrabX) / width * 2.4f, -.52f, .52f);
        }

        void endWheelDrag()
        {
            wheelDrag = -1;
        }

        void steer(float direction, double dt)
        {
            if (std::isfinite(direction) && std::isfinite(dt) && dt > 0) {
                auto& target = steeringGoal(focus);
                target = std::clamp(target + direction * float(std::min(dt, .1)) * 1.2f, -.52f, .52f);
            }
        }

        bool animating() const
        {
            for (size_t i = 0; i < doors.size(); ++i) {
                if (doors[i] != doorTargets[i]) {
                    return true;
                }
            }
            return steering != steeringTarget || rearSteering != rearSteeringTarget;
        }
    };

    struct CinematicTransform
    {
        CinematicPoint right{1, 0, 0}, forward{0, 1, 0}, up{0, 0, 1}, position{};

        CinematicPoint vector(CinematicPoint p) const
        {
            return right * p.x + forward * p.y + up * p.z;
        }

        CinematicPoint point(CinematicPoint p) const
        {
            return vector(p) + position;
        }
    };

    inline CinematicTransform cinematicRotation(CinematicPoint pivot, CinematicPoint axis, float angle)
    {
        CinematicTransform out;
        if (!cinematicFinite(pivot) || !cinematicFinite(axis) || !std::isfinite(angle) || std::abs(angle) < 1.e-7f) {
            return out;
        }
        axis = cinematicUnit(axis);
        const float c = std::cos(angle), s = std::sin(angle);
        auto rotate = [&](CinematicPoint p) {
            return p * c + cinematicCross(axis, p) * s + axis * (cinematicDot(axis, p) * (1 - c));
        };
        out.right = rotate({1, 0, 0});
        out.forward = rotate({0, 1, 0});
        out.up = rotate({0, 0, 1});
        out.position = pivot - out.vector(pivot);
        return out;
    }

    inline void cinematicHoodDirection(CinematicPart& hood)
    {
        if (!hood.present) {
            return;
        }
        const float rise =
            cinematicRotation(hood.pivot, hood.axis, hood.openAngle).point(hood.anchor).z - hood.anchor.z;
        if (rise < -.01f &&
            cinematicRotation(hood.pivot, hood.axis, -hood.openAngle).point(hood.anchor).z - hood.anchor.z > .01f) {
            hood.openAngle = -hood.openAngle;
        }
    }

    inline CinematicTransform
    cinematicPartTransform(const CinematicCapabilities& caps, const CinematicVehicleState* state, int component)
    {
        if (!state || !caps.supported(component)) {
            return {};
        }
        const auto& p = caps.parts[size_t(component)];
        const float angle = component < 6 ? p.openAngle * state->doors[size_t(component)]
                                          : state->steeringAngle(component);
        return cinematicRotation(p.pivot, p.axis, angle);
    }
}
