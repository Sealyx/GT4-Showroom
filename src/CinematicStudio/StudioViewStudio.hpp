#pragma once
#include "Showroom.hpp"
#include "StudioViewPatternData.hpp"

namespace gt4
// vehicle's studioView main studio controller
// reflections, shadows etc
{
    inline constexpr float studioViewReflectionOpacity = .23f;

    inline Camera studioViewCamera(float width, float height, const CinematicStudio& studioView, double now)
    {
        if (studioView.showroomCamera) {
            Camera camera(width,height,5);
            const auto& a=studioView.cameraSeed;
            const float yaw=studioView.yaw(now)-studioView.seedYaw;
            const auto spin=[&](V3 p){return V3{p.x*std::cos(yaw)-p.y*std::sin(yaw),p.x*std::sin(yaw)+p.y*std::cos(yaw),p.z};};
            const V3 focus{0,0,.55f};
            V3 offset=V3{a[0],a[1],a[2]}-focus;
            const float pitch=studioView.pitch-studioView.seedPitch;
            const float horizontal=std::sqrt(offset.x*offset.x+offset.y*offset.y);
            const float distance=std::sqrt(dot(offset,offset))/std::max(.1f,studioView.zoom/studioView.seedZoom);
            const float basePitch=std::atan2(offset.z,horizontal);
            const float orbit=std::atan2(offset.x,-offset.y)+yaw;
            camera.eye=focus+V3{std::sin(orbit)*std::cos(basePitch+pitch)*distance,-std::cos(orbit)*std::cos(basePitch+pitch)*distance,std::sin(basePitch+pitch)*distance};
            // At entry preserve the complete original projection, including off-center framing
            camera.forward=spin({a[3],a[4],a[5]});camera.right=spin({a[6],a[7],a[8]});camera.up=spin({a[9],a[10],a[11]});
            if(std::abs(pitch)>.00001f) {camera.forward=unit(focus-camera.eye);camera.right=unit(cross(camera.forward,{0,0,1}));camera.up=cross(camera.right,camera.forward);}
            camera.focal=a[12]*height;camera.cy=a[13]*height;camera.offsetX=a[14]*width;camera.offsetY=a[15]*height;
            return camera;
        }
        Camera camera(width, height, 5, .5f);
        const float aspect = width / std::max(1.f, height);
        const float framing = std::max(1.f, (16.f / 9) / std::max(.1f, aspect));
        const float distance = 7.5f * framing / std::max(.1f, studioView.zoom);
        const float orbit = studioView.yaw(now);
        const V3 focus{0, 0, .65f};
        const float horizontal = std::cos(studioView.pitch) * distance;
        camera.eye =
            focus + V3{std::sin(orbit) * horizontal, -std::cos(orbit) * horizontal, std::sin(studioView.pitch) * distance};
        camera.forward = unit(focus - camera.eye);
        camera.right = unit(cross(camera.forward, {0, 0, 1}));
        camera.up = cross(camera.right, camera.forward);
        return camera;
    }

    // keep the studioView timeline alive while camera adjustments remain local to photo mode
    inline Camera
    studioViewPhotoCamera(float width, float height, const StudioViewPhoto& photo, const CinematicStudio& studioView, double now)
    {
        if(studioView.showroomCamera) {
            auto posed=studioView;
            posed.yawOffset += photo.yaw-photo.entryYaw;
            posed.pitch=photo.pitch;posed.zoom=photo.zoom;
            auto camera=studioViewCamera(width,height,posed,now);
            const V3 pan{photo.panX,photo.panY,photo.panZ};camera.eye=camera.eye+pan;
            const auto right=camera.right,up=camera.up;
            const float roll=photo.roll*pi/180;
            camera.right=right*std::cos(roll)+up*std::sin(roll);
            camera.up=up*std::cos(roll)-right*std::sin(roll);
            camera.focal*=std::tan(39.30765f*pi/360)/std::tan(photo.fov*pi/360);
            return camera;
        }
        Camera camera(width, height, 5, .5f);
        const float aspect = width / std::max(1.f, height);
        const float framing = std::max(1.f, (16.f / 9) / std::max(.1f, aspect));
        const float distance = 7.5f * framing / photo.zoom;
        const V3 focus{photo.panX, photo.panY, .65f + photo.panZ};
        const float horizontal = std::cos(photo.pitch) * distance;
        const float orbit = photo.yaw + studioView.yaw(now) - photo.entryYaw;
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

    inline StudioViewPhoto returningPhotoPose(const StudioViewPhoto& photo,const CinematicStudio& studioView,double now) {
        auto pose=photo;
        const float t=photo.returnAlpha(now),keep=1.f-t;
        if(photo.studioViewContext) {
            const float target=studioView.yaw(now);
            const float from=photo.yaw+target-photo.entryYaw;
            pose.yaw=target+std::remainder(from-target,CinematicStudio::fullTurn)*keep;
            pose.entryYaw=target;
            pose.pitch=studioView.pitch+(photo.pitch-studioView.pitch)*keep;
            pose.zoom=studioView.zoom+(photo.zoom-studioView.zoom)*keep;
        } else {pose.yaw*=keep;pose.pitch*=keep;pose.zoom=1+(photo.zoom-1)*keep;}
        pose.panX*=keep;pose.panY*=keep;pose.panZ*=keep;pose.roll*=keep;
        pose.fov=39.30765f+(photo.fov-39.30765f)*keep;
        return pose;
    }

    // cinematic keeps its existing camera and car motion; photo adjustments only change our private view
    inline Camera cinematicPhotoCamera(Camera camera, const StudioViewPhoto& photo)
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

    struct StudioViewSurface
    {
        V3 point;
        uint32_t rgb = 0;
        float floorCoverage = 0;
    };

    inline uint32_t studioViewBlend(uint32_t first, uint32_t second, float mix)
    {
        mix = std::clamp(mix, 0.f, 1.f);
        uint32_t result = 0;
        for (int shift : {0, 8, 16}) {
            const float a = float((first >> shift) & 255), b = float((second >> shift) & 255);
            result |= uint32_t(std::lround(a + (b - a) * mix)) << shift;
        }
        return result;
    }

    inline float studioViewFloorHeight(float radius)
    {
        const float offset = std::clamp(radius - 20, 0.f, 4.f);
        return 4 - std::sqrt(std::max(0.f, 16 - offset * offset));
    }

    inline bool studioViewClipFloor(V3& first, V3& second)
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

    inline StudioViewSurface studioViewSurface(const Camera& camera, float x, float y, const CinematicStudio& studioView, double now)
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
                    if (point.z >= studioViewFloorHeight(radius)) {
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
        const auto scene = studioView.settings.scene;
        const bool customizable = scene != StudioViewScene::Custom;
        const uint32_t tint = customizable ? studioView.settings.lighting().color : 0xFFFFFF;
        const float angle = std::atan2(point.y, point.x);
        const float pool = std::exp(-std::pow((point.x + 3) / 12, 2) - std::pow((point.y - 2) / 11, 2));
        const float fill = std::exp(-std::pow((point.x - 5) / 16, 2) - std::pow((point.y + 4) / 14, 2));
        const float softbox = std::pow(std::max(0.f, std::cos(angle + .4f)), 4);
        uint32_t wall = studioViewBlend(0x282D30, 0x4A535A, .28f + .58f * softbox);
        uint32_t ground = studioViewBlend(0x707A80, 0xA2AAAE, .49f * pool + .21f * fill);
        if (scene == StudioViewScene::Softbox) {
            wall = studioViewBlend(0x323B40, 0x808A8E, .10f + .55f * softbox);
            ground = studioViewBlend(0x87969C, 0xE2ECEF, std::clamp(.70f * pool + .18f * fill, 0.f, 1.f));
        }
        wall = studioViewBlend(wall, tint, .035f * softbox);
        ground = studioViewBlend(ground, tint, .04f + .1f * pool);
        if (studioViewWaveScene(scene)) {
            const float time = float(studioView.backgroundElapsed(now));
            const float movingPool = std::exp(-std::pow((point.x + std::sin(time * .22f) * 5) / 5, 2));
            ground = studioViewBlend(ground, tint, .04f * movingPool);
        }
        if (customizable && studioView.settings.wallColor >= 0) {
            const uint32_t surfaceColor = studioViewSurfaceColor(studioView.settings.wallColor);
            wall = studioViewBlend(0x0B0B0D, surfaceColor, .82f + .14f * fill);
        }
        if (customizable && studioView.settings.floorColor >= 0) {
            const uint32_t surfaceColor = studioViewSurfaceColor(studioView.settings.floorColor);
            ground = studioViewBlend(0x101116, surfaceColor, .79f + .17f * pool);
        }
        const float floorMix = ease(std::sqrt(floor));
        return {point, studioViewBlend(wall, ground, floorMix), floor};
    }

    template <class Emit> inline void studioViewClipTriangle(const Camera& camera, V3 a, V3 b, V3 c, Emit emit)
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
    inline std::array<std::array<float, studioViewPatternColumns>, Fields>
    studioViewPatternFields(const std::array<StudioViewPatternKey<Fields>, Count>& keys, double elapsed, double duration)
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
        std::array<std::array<float, studioViewPatternColumns>, Fields> result{};
        for (size_t field = 0; field < Fields; ++field) {
            for (size_t column = 0; column < studioViewPatternColumns; ++column) {
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

    inline constexpr double studioViewWavePatternDuration =
        double(studioViewWaveKeys.back().time) + studioViewWaveKeys[1].time;

    inline auto studioViewWaveFields(const CinematicStudio& studioView, double elapsed)
    {
        const double patternTime = elapsed * studioViewWavePatternDuration / studioView.orbitProfile().duration;
        return studioViewPatternFields(studioViewWaveKeys, patternTime, studioViewWavePatternDuration);
    }

    inline V3 studioViewPatternPoint(float angle, float height);

    struct StudioViewWorldCurve
    {
        static constexpr float radius = 24;
        std::array<float, studioViewPatternColumns + 2> angles{}, heights{};
        std::array<float, studioViewPatternColumns + 2> directionOffsets{};
        float rearVariation = 0;

        StudioViewWorldCurve(const Camera& camera, const std::array<float, studioViewPatternColumns>& rows)
        {
            for (size_t column = 0; column < rows.size(); ++column) {
                const float x = camera.w * column / (studioViewPatternColumns - 1);
                const float y = camera.h * rows[column];
                const V3 ray = camera.forward + camera.right * ((x - camera.w * .5f) / camera.focal) +
                               camera.up * ((camera.cy - y) / camera.focal);
                const float a = ray.x * ray.x + ray.y * ray.y;
                const float b = 2 * (camera.eye.x * ray.x + camera.eye.y * ray.y);
                const float c = camera.eye.x * camera.eye.x + camera.eye.y * camera.eye.y - radius * radius;
                const float distance = (-b + std::sqrt(std::max(0.f, b * b - 4 * a * c))) / (2 * a);
                const V3 cylinderPoint = camera.eye + ray * distance;
                const V3 point = studioViewSurface(camera, x, y, CinematicStudio{}, 0).point;
                const float radial = std::sqrt(point.x * point.x + point.y * point.y);
                float height = point.z;
                if (point.z < 4.f) {
                    height = radial > 20
                        ? 4 - 4 * std::atan2(4 - point.z, radial - 20)
                        : 4 - 2 * pi + 20 * std::log(std::max(radial, .01f) / 20);
                } else if (point.z >= 15.999f && radial < radius) {
                    height = 16 + radius * std::log(radius / std::max(radial, .01f));
                }
                float angle = std::atan2(cylinderPoint.y, cylinderPoint.x);
                if (column != 0) {
                    angle = angles[column - 1] + std::remainder(angle - angles[column - 1], CinematicStudio::fullTurn);
                }
                angles[column] = angle;
                heights[column] = std::clamp(height, -64.f, 64.f);
                directionOffsets[column] = std::remainder(std::atan2(point.y, point.x) - angle,
                                                          CinematicStudio::fullTurn);
            }
            constexpr size_t last = studioViewPatternColumns - 1;
            if (angles[last] < angles[0]) {
                std::reverse(angles.begin(), angles.begin() + studioViewPatternColumns);
                std::reverse(heights.begin(), heights.begin() + studioViewPatternColumns);
                std::reverse(directionOffsets.begin(), directionOffsets.begin() + studioViewPatternColumns);
            }
            angles[studioViewPatternColumns] = (angles[last] + angles[0] + CinematicStudio::fullTurn) * .5f;
            heights[studioViewPatternColumns] = (heights[last] + heights[0]) * .5f;
            angles[studioViewPatternColumns + 1] = angles[0] + CinematicStudio::fullTurn;
            heights[studioViewPatternColumns + 1] = heights[0];
            directionOffsets[studioViewPatternColumns] = (directionOffsets[last] + directionOffsets[0]) * .5f;
            directionOffsets[studioViewPatternColumns + 1] = directionOffsets[0];
            const auto range = std::minmax_element(heights.begin(), heights.begin() + studioViewPatternColumns);
            rearVariation = std::clamp((*range.second - *range.first) * .3f, .2f, 2.f);
        }

        float at(float angle, bool direction = false) const
        {
            angle = angles[0] + std::fmod(std::fmod(angle - angles[0], CinematicStudio::fullTurn) + CinematicStudio::fullTurn,
                                          CinematicStudio::fullTurn);
            size_t first = 0;
            constexpr size_t last = studioViewPatternColumns + 1;
            while (first + 1 < last && angles[first + 1] < angle) {
                ++first;
            }
            const size_t second = first + 1;
            const size_t previous = first == 0 ? studioViewPatternColumns : first - 1;
            const size_t following = second == last ? 1 : second + 1;
            const float start = angles[first];
            const float end = angles[second];
            const float before = angles[previous] - (first == 0 ? CinematicStudio::fullTurn : 0);
            const float finish = angles[following] + (second == last ? CinematicStudio::fullTurn : 0);
            const float interval = end - start;
            const float u = std::clamp((angle - start) / interval, 0.f, 1.f);
            const auto& values = direction ? directionOffsets : heights;
            const float a = values[first];
            const float b = values[second];
            const float tangentA = (b - values[previous]) / (end - before) * interval;
            const float tangentB = (values[following] - a) / (finish - start) * interval;
            const float u2 = u * u;
            const float u3 = u2 * u;
            float height = (2 * u3 - 3 * u2 + 1) * a + (u3 - 2 * u2 + u) * tangentA +
                           (-2 * u3 + 3 * u2) * b + (u3 - u2) * tangentB;
            if (!direction && first >= studioViewPatternColumns - 1) {
                // keep the unseen side wavy too, with zero extra slope at either join
                const float envelope = std::sin(pi * u);
                height += rearVariation * envelope * envelope * std::sin(2 * pi * u);
            }
            return direction ? height : std::clamp(height, -64.f, 64.f);
        }

        V3 pointAt(float angle) const
        {
            return studioViewPatternPoint(angle + at(angle, true), at(angle));
        }
    };

    struct StudioViewWaveVertex
    {
        V3 point;
        uint32_t rgb = 0;
        float alpha = 0;
    };

    // continue the wall artwork around the room's curved floor instead of chopping it at ground level
    inline V3 studioViewPatternPoint(float angle, float height)
    {
        float radius = StudioViewWorldCurve::radius;
        if (height > 16.f) {
            radius *= std::exp(-(height - 16) / radius);
            height = 16;
        } else if (height < 4.f) {
            const float distance = 4.f - height;
            const float arc = 2 * pi;
            if (distance <= arc) {
                const float bend = distance / 4;
                radius = 20 + 4 * std::cos(bend);
                height = 4 - 4 * std::sin(bend);
            } else {
                radius = 20 * std::exp(-(distance - arc) / 20);
                height = 0;
            }
        }
        return {std::cos(angle) * radius, std::sin(angle) * radius, height};
    }

    inline StudioViewWaveVertex
    studioViewWaveBetween(const StudioViewWaveVertex& first, const StudioViewWaveVertex& second, float amount)
    {
        return {first.point + (second.point - first.point) * amount,
                studioViewBlend(first.rgb, second.rgb, amount),
                first.alpha + (second.alpha - first.alpha) * amount};
    }

    template <class Emit>
    inline void studioViewClipWaveTriangle(
        const Camera& camera, StudioViewWaveVertex a, StudioViewWaveVertex b, StudioViewWaveVertex c, bool reflected, Emit emit)
    {
        std::array<StudioViewWaveVertex, 6> input{{a, b, c}}, clipped{};
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
                    auto intersection = studioViewWaveBetween(first, second, start / (start - finish));
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

    template <class Emit> inline void studioViewWaveTriangles(const CinematicStudio& studioView, double now, Emit emit)
    {
        if (!studioViewWaveScene(studioView.settings.scene)) {
            return;
        }
        const auto& lighting = studioView.settings.lighting();
        const uint32_t core = studioViewBlend(lighting.color, 0xFFFFFF, .2f);
        const uint32_t halo = lighting.halo;
        CinematicStudio reference = studioView;
        reference.yawOffset = 0;
        reference.pitch = .18f;
        reference.zoom = 1;
        const Camera camera = studioViewCamera(1280, 720, reference, now);
        const auto fields = studioViewWaveFields(studioView, studioView.backgroundElapsed(now));
        std::array<float, studioViewPatternColumns> upper{}, lower{};
        for (size_t column = 0; column < studioViewPatternColumns; ++column) {
            const float halfHeight = std::clamp(fields[1][column], .015f, .2f) * 1.5f;
            upper[column] = fields[0][column] - halfHeight;
            lower[column] = fields[0][column] + halfHeight;
        }
        const StudioViewWorldCurve top(camera, upper), bottom(camera, lower), middle(camera, fields[0]);
        const float edge = std::exp(-3.2f);
        std::array<float, 13> levels{}, alphas{};
        std::array<uint32_t, 13> colors{};
        for (unsigned band = 0; band < levels.size(); ++band) {
            levels[band] = float(band) / 6 - 1;
            const float intensity = std::exp(-levels[band] * levels[band] * 3.2f);
            colors[band] = studioViewBlend(halo, core, std::pow(intensity, 1.8f));
            alphas[band] = (intensity - edge) / (1 - edge);
        }
        auto column = [&](unsigned index) {
            const float angle = float(index) * CinematicStudio::fullTurn / 720;
            const float high = top.at(angle), low = bottom.at(angle), center = middle.at(angle);
            const float halfHeight = std::max(.1f, (high - low) * .5f);
            std::array<StudioViewWaveVertex, 13> result{};
            for (unsigned band = 0; band < result.size(); ++band) {
                result[band] = {studioViewPatternPoint(angle + middle.at(angle, true), center + levels[band] * halfHeight),
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

    inline constexpr int studioViewRibbonSegmentCount = 256;

    template <class Emit> inline void studioViewPatternLines(const CinematicStudio& studioView, double now, Emit emit)
    {
        const auto scene = studioView.settings.scene;
        if (scene != StudioViewScene::Ribbons && !studioViewWaveScene(scene)) {
            return;
        }
        const auto& lighting = studioView.settings.lighting();
        CinematicStudio reference = studioView;
        reference.yawOffset = 0;
        reference.pitch = .18f;
        reference.zoom = 1;
        const Camera camera = studioViewCamera(1280, 720, reference, now);
        const double time = studioView.backgroundElapsed(now);
        if (scene == StudioViewScene::Ribbons) {
            const auto fields = studioViewPatternFields(studioViewRibbonKeys, time, studioView.orbitProfile().duration);
            const StudioViewWorldCurve top(camera, fields[0]);
            const StudioViewWorldCurve bottom(camera, fields[1]);
            const StudioViewWorldCurve middle(camera, fields[2]);
            for (int band = 0; band < 20; ++band) {
                const float level = float(band) / 19;
                const uint32_t rgb = studioViewBlend(lighting.color, lighting.halo, level * .82f);
                auto point = [&](int index) {
                    const float angle = index * CinematicStudio::fullTurn / studioViewRibbonSegmentCount;
                    const float high = top.at(angle);
                    const float low = bottom.at(angle);
                    const float bend = middle.at(angle) - (high + low) * .5f;
                    const float height = low * (1 - level) + high * level + 4 * level * (1 - level) * bend;
                    return studioViewPatternPoint(angle + middle.at(angle, true), height);
                };
                for (int segment = 0; segment < studioViewRibbonSegmentCount; ++segment) {
                    emit(point(segment), point(segment + 1), rgb, rgb, 1.f, 1.2f);
                }
            }
            return;
        }
        const auto fields = studioViewWaveFields(studioView, time);
        std::array<float, studioViewPatternColumns> upper{}, lower{};
        for (size_t column = 0; column < studioViewPatternColumns; ++column) {
            const float halfHeight = std::clamp(fields[1][column], .015f, .2f) * 1.5f;
            upper[column] = fields[0][column] - halfHeight;
            lower[column] = fields[0][column] + halfHeight;
        }
        const StudioViewWorldCurve top(camera, upper);
        const StudioViewWorldCurve bottom(camera, lower);
        const StudioViewWorldCurve middle(camera, fields[0]);
        for (int bar = 0; bar < 720; ++bar) {
            const float angle = bar * CinematicStudio::fullTurn / 720;
            const float high = top.at(angle);
            const float low = bottom.at(angle);
            const float center = middle.at(angle);
            const float halfHeight = std::max(.1f, (high - low) * .5f);
            const float direction = angle + middle.at(angle, true);
            const V3 a = studioViewPatternPoint(direction, center - halfHeight);
            const V3 b = studioViewPatternPoint(direction, center + halfHeight);
            emit(a, b, lighting.halo, lighting.halo, .025f, .35f);
        }
    }
}
