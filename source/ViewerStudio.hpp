#pragma once
#include "Showroom.hpp"
#include "ViewerPatternData.hpp"

namespace gt4
// vehicle's viewer main studio controller
// reflections, shadows etc
{
    inline constexpr float viewerReflectionOpacity = .23f;

    inline Camera viewerCamera(float width, float height, const VehicleViewer& viewer, double now)
    {
        Camera camera(width, height, 5, .5f);
        const float aspect = width / std::max(1.f, height);
        const float framing = std::max(1.f, (16.f / 9) / std::max(.1f, aspect));
        const float distance = 7.5f * framing / std::max(.1f, viewer.zoom);
        const float orbit = viewer.yaw(now);
        const V3 focus{0, 0, .65f};
        const float horizontal = std::cos(viewer.pitch) * distance;
        camera.eye =
            focus + V3{std::sin(orbit) * horizontal, -std::cos(orbit) * horizontal, std::sin(viewer.pitch) * distance};
        camera.forward = unit(focus - camera.eye);
        camera.right = unit(cross(camera.forward, {0, 0, 1}));
        camera.up = cross(camera.right, camera.forward);
        return camera;
    }

    // keep the viewer timeline alive while camera adjustments remain local to photo mode
    inline Camera
    viewerPhotoCamera(float width, float height, const ViewerPhoto& photo, const VehicleViewer& viewer, double now)
    {
        Camera camera(width, height, 5, .5f);
        const float aspect = width / std::max(1.f, height);
        const float framing = std::max(1.f, (16.f / 9) / std::max(.1f, aspect));
        const float distance = 7.5f * framing / photo.zoom;
        const V3 focus{photo.panX, photo.panY, .65f + photo.panZ};
        const float horizontal = std::cos(photo.pitch) * distance;
        const float orbit = photo.yaw + viewer.yaw(now) - photo.entryYaw;
        camera.eye =
            focus + V3{std::sin(orbit) * horizontal, -std::cos(orbit) * horizontal, std::sin(photo.pitch) * distance};
        camera.forward = unit(focus - camera.eye);
        const auto right = unit(cross(camera.forward, {0, 0, 1}));
        const auto up = cross(right, camera.forward);
        const float angle = photo.roll * pi / 180;
        camera.right = right * std::cos(angle) + up * std::sin(angle);
        camera.up = up * std::cos(angle) - right * std::sin(angle);
        camera.focal = height / (2 * std::tan(photo.fov * pi / 360));
        return camera;
    }

    // cinematic keeps its existing camera and car motion; photo adjustments only change our private view
    inline Camera cinematicPhotoCamera(Camera camera, const ViewerPhoto& photo)
    {
        const V3 focus{photo.panX, photo.panY, .55f + photo.panZ};
        const V3 offset = camera.eye - V3{0, 0, .55f};
        const float distance = std::sqrt(dot(offset, offset)) / photo.zoom;
        const float pitch = std::atan2(offset.z, std::sqrt(offset.x * offset.x + offset.y * offset.y)) + photo.pitch;
        const float orbit = std::atan2(offset.x, -offset.y) + photo.yaw;
        const float horizontal = std::cos(pitch) * distance;
        camera.eye = focus + V3{
            std::sin(orbit) * horizontal, -std::cos(orbit) * horizontal, std::sin(pitch) * distance};
        camera.forward = unit(focus - camera.eye);
        const auto right = unit(cross(camera.forward, {0, 0, 1}));
        const auto up = cross(right, camera.forward);
        const float roll = photo.roll * pi / 180;
        camera.right = right * std::cos(roll) + up * std::sin(roll);
        camera.up = up * std::cos(roll) - right * std::sin(roll);
        camera.focal = camera.h / (2 * std::tan(photo.fov * pi / 360));
        return camera;
    }

    struct ViewerSurface
    {
        V3 point;
        uint32_t rgb = 0;
        float floorCoverage = 0;
    };

    inline uint32_t viewerBlend(uint32_t first, uint32_t second, float mix)
    {
        mix = std::clamp(mix, 0.f, 1.f);
        uint32_t result = 0;
        for (int shift : {0, 8, 16}) {
            const float a = float((first >> shift) & 255), b = float((second >> shift) & 255);
            result |= uint32_t(std::lround(a + (b - a) * mix)) << shift;
        }
        return result;
    }

    inline float viewerFloorHeight(float radius)
    {
        const float offset = std::clamp(radius - 20, 0.f, 4.f);
        return 4 - std::sqrt(std::max(0.f, 16 - offset * offset));
    }

    inline bool viewerClipFloor(V3& first, V3& second)
    {
        if (first.z < 0 && second.z < 0) {
            return false;
        }
        if (first.z >= 0 && second.z >= 0) {
            return true;
        }
        const float amount = first.z / (first.z - second.z);
        V3 floorPoint = first + (second - first) * amount;
        floorPoint.z = 0;
        if (first.z < 0) {
            first = floorPoint;
        } else {
            second = floorPoint;
        }
        return true;
    }

    inline ViewerSurface viewerSurface(const Camera& camera, float x, float y, const VehicleViewer& viewer, double now)
    {
        const V3 ray = camera.forward + camera.right * ((x - camera.offsetX - camera.w * .5f) / camera.focal) +
                       camera.up * ((camera.offsetY + camera.cy - y) / camera.focal);
        const float a = ray.x * ray.x + ray.y * ray.y;
        const float b = 2 * (camera.eye.x * ray.x + camera.eye.y * ray.y);
        const float c = camera.eye.x * camera.eye.x + camera.eye.y * camera.eye.y - 24 * 24;
        const float discriminant = std::max(0.f, b * b - 4 * a * c);
        float distance = a > .00001f ? (-b + std::sqrt(discriminant)) / (2 * a) : 200.f;
        V3 normal{-1, 0, 0};
        bool flat = false;
        bool ceilingHit = false;
        if (ray.z < -.00001f) {
            const float ground = -camera.eye.z / ray.z;
            if (ground > 0 && ground < distance) {
                const V3 point = camera.eye + ray * ground;
                const float radius = std::sqrt(point.x * point.x + point.y * point.y);
                const float eyeRadius = std::sqrt(camera.eye.x * camera.eye.x + camera.eye.y * camera.eye.y);
                if (radius <= 20 || eyeRadius >= 24) {
                    distance = ground;
                    flat = true;
                }
            }
        } else if (ray.z > .00001f) {
            const float ceiling = (16 - camera.eye.z) / ray.z;
            if (ceiling > 0 && ceiling < distance) {
                distance = ceiling;
                ceilingHit = true;
            }
        }
        if (!flat && !ceilingHit && camera.eye.z > 0 &&
            camera.eye.x * camera.eye.x + camera.eye.y * camera.eye.y < 400) {
            const V3 wall = camera.eye + ray * distance;
            if (wall.z < 4) {
                float low = 0;
                float high = distance;
                for (int iteration = 0; iteration < 18; ++iteration) {
                    const float middle = (low + high) * .5f;
                    const V3 point = camera.eye + ray * middle;
                    const float radius = std::sqrt(point.x * point.x + point.y * point.y);
                    if (point.z >= viewerFloorHeight(radius)) {
                        low = middle;
                    } else {
                        high = middle;
                    }
                }
                distance = (low + high) * .5f;
            }
        }
        const V3 point = camera.eye + ray * std::clamp(distance, .11f, 200.f);
        const float radius = std::sqrt(point.x * point.x + point.y * point.y);
        if (flat || point.z < .0005f) {
            normal = {0, 0, 1};
        } else if (ceilingHit) {
            normal = {0, 0, -1};
        } else if (point.z < 4 && radius > 20) {
            const float radial = std::clamp((radius - 20) / 4, 0.f, 1.f);
            normal = {-point.x / radius * radial, -point.y / radius * radial, (4 - point.z) / 4};
            normal = unit(normal);
        } else if (radius > .001f) {
            normal = {-point.x / radius, -point.y / radius, 0};
        }
        const float floor = std::pow(std::max(0.f, normal.z), 4);
        const auto scene = viewer.settings.scene;
        const bool customizable = scene != ViewerScene::Custom;
        const uint32_t tint = customizable ? viewer.settings.lighting().color : 0xFFFFFF;
        const float angle = std::atan2(point.y, point.x);
        const float pool = std::exp(-std::pow((point.x + 3) / 12, 2) - std::pow((point.y - 2) / 11, 2));
        const float fill = std::exp(-std::pow((point.x - 5) / 16, 2) - std::pow((point.y + 4) / 14, 2));
        const float softbox = std::pow(std::max(0.f, std::cos(angle + .4f)), 4);
        uint32_t wall = viewerBlend(0x282D30, 0x4A535A, .28f + .58f * softbox);
        uint32_t ground = viewerBlend(0x707A80, 0xA2AAAE, .49f * pool + .21f * fill);
        wall = viewerBlend(wall, tint, .035f * softbox);
        ground = viewerBlend(ground, tint, .04f + .1f * pool);
        if (viewerWaveScene(scene)) {
            const float time = float(viewer.backgroundElapsed(now));
            const float movingPool = std::exp(-std::pow((point.x + std::sin(time * .22f) * 5) / 5, 2));
            ground = viewerBlend(ground, tint, .04f * movingPool);
        }
        if (customizable && viewer.settings.wallColor >= 0) {
            const uint32_t surfaceColor = viewerSurfaceColor(viewer.settings.wallColor);
            wall = viewerBlend(0x0B0B0D, surfaceColor, .82f + .14f * fill);
        }
        if (customizable && viewer.settings.floorColor >= 0) {
            const uint32_t surfaceColor = viewerSurfaceColor(viewer.settings.floorColor);
            ground = viewerBlend(0x101116, surfaceColor, .79f + .17f * pool);
        }
        const float floorMix = ease(std::sqrt(floor));
        return {point, viewerBlend(wall, ground, floorMix), floor};
    }

    template <class Emit> inline void viewerClipTriangle(const Camera& camera, V3 a, V3 b, V3 c, Emit emit)
    {
        std::array<V3, 4> clipped{};
        unsigned count = 0;
        const std::array<V3, 3> input{a, b, c};
        for (unsigned index = 0; index < input.size(); ++index) {
            const V3 first = input[index];
            const V3 second = input[(index + 1) % input.size()];
            const float start = dot(first - camera.eye, camera.forward) - .11f;
            const float finish = dot(second - camera.eye, camera.forward) - .11f;
            if (start >= 0) {
                clipped[count++] = first;
            }
            if ((start >= 0) != (finish >= 0)) {
                clipped[count++] = first + (second - first) * (start / (start - finish));
            }
        }
        for (unsigned index = 1; index + 1 < count; ++index) {
            emit(clipped[0], clipped[index], clipped[index + 1]);
        }
    }

    template <size_t Fields, size_t Count>
    inline std::array<std::array<float, viewerPatternColumns>, Fields>
    viewerPatternFields(const std::array<ViewerPatternKey<Fields>, Count>& keys, double elapsed, double duration)
    {
        const float time = float(std::fmod(std::max(0., elapsed), duration));
        size_t first = 0;
        while (first + 1 < Count && keys[first + 1].time <= time) {
            ++first;
        }
        const size_t second = (first + 1) % Count;
        const size_t previous = (first + Count - 1) % Count;
        const size_t following = (first + 2) % Count;
        const float start = keys[first].time;
        const float end = keys[second].time + (second == 0 ? float(duration) : 0);
        const float before = keys[previous].time - (first == 0 ? float(duration) : 0);
        const float finish = keys[following].time + (following <= second ? float(duration) : 0);
        const float interval = end - start;
        const float u = (time - start) / interval;
        const float u2 = u * u;
        const float u3 = u2 * u;
        std::array<std::array<float, viewerPatternColumns>, Fields> result{};
        for (size_t field = 0; field < Fields; ++field) {
            for (size_t column = 0; column < viewerPatternColumns; ++column) {
                const float a = keys[first].fields[field][column];
                const float b = keys[second].fields[field][column];
                const float tangentA = (b - keys[previous].fields[field][column]) / (end - before) * interval;
                const float tangentB = (keys[following].fields[field][column] - a) / (finish - start) * interval;
                result[field][column] = (2 * u3 - 3 * u2 + 1) * a + (u3 - 2 * u2 + u) * tangentA +
                                        (-2 * u3 + 3 * u2) * b + (u3 - u2) * tangentB;
            }
        }
        return result;
    }

    inline constexpr double viewerWavePatternDuration =
        double(viewerWaveKeys.back().time) + viewerWaveKeys[1].time;

    inline auto viewerWaveFields(const VehicleViewer& viewer, double elapsed)
    {
        const double patternTime = elapsed * viewerWavePatternDuration / viewer.orbitProfile().duration;
        return viewerPatternFields(viewerWaveKeys, patternTime, viewerWavePatternDuration);
    }

    struct ViewerWorldCurve
    {
        static constexpr float radius = 9;
        std::array<float, viewerPatternColumns + 2> angles{}, heights{};

        ViewerWorldCurve(const Camera& camera, const std::array<float, viewerPatternColumns>& rows)
        {
            for (size_t column = 0; column < rows.size(); ++column) {
                const float x = camera.w * column / (viewerPatternColumns - 1);
                const float y = camera.h * rows[column];
                const V3 ray = camera.forward + camera.right * ((x - camera.w * .5f) / camera.focal) +
                               camera.up * ((camera.cy - y) / camera.focal);
                const float a = ray.x * ray.x + ray.y * ray.y;
                const float b = 2 * (camera.eye.x * ray.x + camera.eye.y * ray.y);
                const float c = camera.eye.x * camera.eye.x + camera.eye.y * camera.eye.y - radius * radius;
                const float distance = (-b + std::sqrt(std::max(0.f, b * b - 4 * a * c))) / (2 * a);
                const V3 point = camera.eye + ray * distance;
                float angle = std::atan2(point.y, point.x);
                if (column != 0) {
                    angle = angles[column - 1] + std::remainder(angle - angles[column - 1], VehicleViewer::fullTurn);
                }
                angles[column] = angle;
                heights[column] = std::clamp(point.z, -2.f, 12.f);
            }
            constexpr size_t last = viewerPatternColumns - 1;
            if (angles[last] < angles[0]) {
                std::reverse(angles.begin(), angles.begin() + viewerPatternColumns);
                std::reverse(heights.begin(), heights.begin() + viewerPatternColumns);
            }
            angles[viewerPatternColumns] = (angles[last] + angles[0] + VehicleViewer::fullTurn) * .5f;
            heights[viewerPatternColumns] = (heights[last] + heights[0]) * .5f;
            angles[viewerPatternColumns + 1] = angles[0] + VehicleViewer::fullTurn;
            heights[viewerPatternColumns + 1] = heights[0];
        }

        float at(float angle) const
        {
            angle = angles[0] + std::fmod(std::fmod(angle - angles[0], VehicleViewer::fullTurn) + VehicleViewer::fullTurn,
                                          VehicleViewer::fullTurn);
            size_t first = 0;
            constexpr size_t last = viewerPatternColumns + 1;
            while (first + 1 < last && angles[first + 1] < angle) {
                ++first;
            }
            const size_t second = first + 1;
            const size_t previous = first == 0 ? viewerPatternColumns : first - 1;
            const size_t following = second == last ? 1 : second + 1;
            const float start = angles[first];
            const float end = angles[second];
            const float before = angles[previous] - (first == 0 ? VehicleViewer::fullTurn : 0);
            const float finish = angles[following] + (second == last ? VehicleViewer::fullTurn : 0);
            const float interval = end - start;
            const float u = std::clamp((angle - start) / interval, 0.f, 1.f);
            const float a = heights[first];
            const float b = heights[second];
            const float tangentA = (b - heights[previous]) / (end - before) * interval;
            const float tangentB = (heights[following] - a) / (finish - start) * interval;
            const float u2 = u * u;
            const float u3 = u2 * u;
            return std::clamp((2 * u3 - 3 * u2 + 1) * a + (u3 - 2 * u2 + u) * tangentA + (-2 * u3 + 3 * u2) * b +
                                  (u3 - u2) * tangentB,
                              -2.f,
                              12.f);
        }
    };

    struct ViewerWaveVertex
    {
        V3 point;
        uint32_t rgb = 0;
        float alpha = 0;
    };

    inline ViewerWaveVertex
    viewerWaveBetween(const ViewerWaveVertex& first, const ViewerWaveVertex& second, float amount)
    {
        return {first.point + (second.point - first.point) * amount,
                viewerBlend(first.rgb, second.rgb, amount),
                first.alpha + (second.alpha - first.alpha) * amount};
    }

    template <class Emit>
    inline void viewerClipWaveTriangle(
        const Camera& camera, ViewerWaveVertex a, ViewerWaveVertex b, ViewerWaveVertex c, bool reflected, Emit emit)
    {
        std::array<ViewerWaveVertex, 6> input{{a, b, c}}, clipped{};
        unsigned count = 3;
        for (unsigned plane = 0; plane < 2; ++plane) {
            unsigned output = 0;
            for (unsigned index = 0; index < count; ++index) {
                const auto& first = input[index];
                const auto& second = input[(index + 1) % count];
                const float start = plane == 0 ? first.point.z : dot(first.point - camera.eye, camera.forward) - .11f;
                const float finish =
                    plane == 0 ? second.point.z : dot(second.point - camera.eye, camera.forward) - .11f;
                if (start >= 0) {
                    clipped[output++] = first;
                }
                if ((start >= 0) != (finish >= 0)) {
                    auto intersection = viewerWaveBetween(first, second, start / (start - finish));
                    if (plane == 0) {
                        intersection.point.z = 0;
                    }
                    clipped[output++] = intersection;
                }
            }
            input = clipped;
            count = output;
            if (plane == 0 && reflected) {
                for (unsigned index = 0; index < count; ++index) {
                    input[index].point.z = -input[index].point.z;
                    input[index].alpha *= .18f;
                }
            }
        }
        for (unsigned index = 1; index + 1 < count; ++index) {
            emit(input[0], input[index], input[index + 1]);
        }
    }

    template <class Emit> inline void viewerWaveTriangles(const VehicleViewer& viewer, double now, Emit emit)
    {
        if (!viewerWaveScene(viewer.settings.scene)) {
            return;
        }
        const auto& lighting = viewer.settings.lighting();
        const uint32_t core = viewerBlend(lighting.color, 0xFFFFFF, .2f);
        const uint32_t halo = lighting.halo;
        VehicleViewer reference = viewer;
        reference.yawOffset = 0;
        reference.pitch = .18f;
        reference.zoom = 1;
        const Camera camera = viewerCamera(1280, 720, reference, now);
        const auto fields = viewerWaveFields(viewer, viewer.backgroundElapsed(now));
        std::array<float, viewerPatternColumns> upper{}, lower{};
        for (size_t column = 0; column < viewerPatternColumns; ++column) {
            const float halfHeight = std::clamp(fields[1][column], .015f, .2f) * 1.5f;
            upper[column] = fields[0][column] - halfHeight;
            lower[column] = fields[0][column] + halfHeight;
        }
        const ViewerWorldCurve top(camera, upper), bottom(camera, lower), middle(camera, fields[0]);
        const float edge = std::exp(-3.2f);
        std::array<float, 13> levels{}, alphas{};
        std::array<uint32_t, 13> colors{};
        for (unsigned band = 0; band < levels.size(); ++band) {
            levels[band] = float(band) / 6 - 1;
            const float intensity = std::exp(-levels[band] * levels[band] * 3.2f);
            colors[band] = viewerBlend(halo, core, std::pow(intensity, 1.8f));
            alphas[band] = (intensity - edge) / (1 - edge);
        }
        auto column = [&](unsigned index) {
            const float angle = float(index) * VehicleViewer::fullTurn / 720;
            const float high = top.at(angle), low = bottom.at(angle), center = middle.at(angle);
            const float halfHeight = std::max(.1f, (high - low) * .5f);
            std::array<ViewerWaveVertex, 13> result{};
            for (unsigned band = 0; band < result.size(); ++band) {
                result[band] = {{std::cos(angle) * ViewerWorldCurve::radius,
                                 std::sin(angle) * ViewerWorldCurve::radius,
                                 std::clamp(center + levels[band] * halfHeight, -2.f, 12.f)},
                                colors[band],
                                alphas[band]};
            }
            return result;
        };
        const auto first = column(0);
        auto previous = first;
        for (unsigned segment = 0; segment < 720; ++segment) {
            const auto next = segment == 719 ? first : column(segment + 1);
            for (unsigned band = 0; band < 12; ++band) {
                emit(previous[band], next[band], next[band + 1]);
                emit(previous[band], next[band + 1], previous[band + 1]);
            }
            previous = next;
        }
    }

    inline constexpr int viewerRibbonSegmentCount = 128;

    template <class Emit> inline void viewerPatternLines(const VehicleViewer& viewer, double now, Emit emit)
    {
        const auto scene = viewer.settings.scene;
        if (scene != ViewerScene::Ribbons && !viewerWaveScene(scene)) {
            return;
        }
        const auto& lighting = viewer.settings.lighting();
        VehicleViewer reference = viewer;
        reference.yawOffset = 0;
        reference.pitch = .18f;
        reference.zoom = 1;
        const Camera camera = viewerCamera(1280, 720, reference, now);
        const double time = viewer.backgroundElapsed(now);
        if (scene == ViewerScene::Ribbons) {
            const auto fields = viewerPatternFields(viewerRibbonKeys, time, viewer.orbitProfile().duration);
            const ViewerWorldCurve top(camera, fields[0]);
            const ViewerWorldCurve bottom(camera, fields[1]);
            const ViewerWorldCurve middle(camera, fields[2]);
            for (int band = 0; band < 20; ++band) {
                const float level = float(band) / 19;
                const uint32_t rgb = viewerBlend(lighting.color, lighting.halo, level * .82f);
                auto point = [&](int index) {
                    const float angle = index * VehicleViewer::fullTurn / viewerRibbonSegmentCount;
                    const float high = top.at(angle);
                    const float low = bottom.at(angle);
                    const float bend = middle.at(angle) - (high + low) * .5f;
                    const float height = low * (1 - level) + high * level + 4 * level * (1 - level) * bend;
                    return V3{
                        std::cos(angle) * ViewerWorldCurve::radius, std::sin(angle) * ViewerWorldCurve::radius, height};
                };
                for (int segment = 0; segment < viewerRibbonSegmentCount; ++segment) {
                    emit(point(segment), point(segment + 1), rgb, rgb, 1.f, 1.2f);
                }
            }
            return;
        }
        const auto fields = viewerWaveFields(viewer, time);
        std::array<float, viewerPatternColumns> upper{}, lower{};
        for (size_t column = 0; column < viewerPatternColumns; ++column) {
            const float halfHeight = std::clamp(fields[1][column], .015f, .2f) * 1.5f;
            upper[column] = fields[0][column] - halfHeight;
            lower[column] = fields[0][column] + halfHeight;
        }
        const ViewerWorldCurve top(camera, upper);
        const ViewerWorldCurve bottom(camera, lower);
        const ViewerWorldCurve middle(camera, fields[0]);
        for (int bar = 0; bar < 720; ++bar) {
            const float angle = bar * VehicleViewer::fullTurn / 720;
            const float high = top.at(angle);
            const float low = bottom.at(angle);
            const float center = middle.at(angle);
            const float halfHeight = std::max(.1f, (high - low) * .5f);
            const V3 a{std::cos(angle) * ViewerWorldCurve::radius,
                       std::sin(angle) * ViewerWorldCurve::radius,
                       std::clamp(center - halfHeight, -2.f, 12.f)};
            const V3 b{a.x, a.y, std::clamp(center + halfHeight, -2.f, 12.f)};
            emit(a, b, lighting.halo, lighting.halo, .025f, .35f);
        }
    }
}
