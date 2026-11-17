#pragma once
#include "Showroom.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <map>
#include <vector>

namespace gt4
{
    // sample the actual enclosure, including surfaces hidden from the camera
    class CavityOcclusion
    {
        struct Triangle { V3 a, b, c; };
        struct Node
        {
            V3 low{}, high{};
            size_t begin = 0, end = 0;
            int left = -1, right = -1;
        };
        struct Sample { V3 position, normal; bool opaque; };
        std::vector<Triangle> triangles;
        std::vector<Node> nodes;
        std::vector<int> pending;
        std::vector<Sample> samples;
        std::map<std::array<int, 6>, float> cache;
        size_t cursor = 0;
        float radius = 1;
        static float axis(V3 p, int a) { return a == 0 ? p.x : a == 1 ? p.y : p.z; }
        static V3 minimum(V3 a, V3 b)
        { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
        static V3 maximum(V3 a, V3 b)
        { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
        int node(size_t begin, size_t end)
        {
            Node n;
            n.begin = begin;
            n.end = end;
            n.low = {1.e30f, 1.e30f, 1.e30f};
            n.high = {-1.e30f, -1.e30f, -1.e30f};
            for (size_t i = begin; i < end; ++i) {
                const auto& t = triangles[i];
                n.low = minimum(n.low, minimum(t.a, minimum(t.b, t.c)));
                n.high = maximum(n.high, maximum(t.a, maximum(t.b, t.c)));
            }
            nodes.push_back(n);
            pending.push_back(int(nodes.size() - 1));
            return int(nodes.size() - 1);
        }
        bool intersects(const Node& n, V3 p, V3 direction, float distance) const
        {
            float entry = 0, exit = distance;
            for (int a = 0; a < 3; ++a) {
                const float d = axis(direction, a), origin = axis(p, a);
                if (std::abs(d) < 1.e-8f) {
                    if (origin < axis(n.low, a) || origin > axis(n.high, a)) { return false; }
                } else {
                    float x = (axis(n.low, a) - origin) / d, y = (axis(n.high, a) - origin) / d;
                    if (x > y) { std::swap(x, y); }
                    entry = std::max(entry, x);
                    exit = std::min(exit, y);
                    if (exit < entry) { return false; }
                }
            }
            return true;
        }
        float trace(V3 p, V3 direction, float distance) const
        {
            std::array<int, 64> stack{};
            unsigned count = 1;
            while (count) {
                const auto& n = nodes[size_t(stack[--count])];
                if (!intersects(n, p, direction, distance)) { continue; }
                if (n.left >= 0) {
                    stack[count++] = n.left;
                    stack[count++] = n.right;
                    continue;
                }
                for (size_t i = n.begin; i < n.end; ++i) {
                    const auto& t = triangles[i];
                    const V3 edge = t.b - t.a, other = t.c - t.a;
                    const V3 h = cross(direction, other);
                    const float determinant = dot(edge, h);
                    if (std::abs(determinant) < 1.e-10f) { continue; }
                    const float inverse = 1.f / determinant;
                    const V3 s = p - t.a;
                    const float u = inverse * dot(s, h);
                    if (u < 0 || u > 1) { continue; }
                    const V3 q = cross(s, edge);
                    const float v = inverse * dot(direction, q);
                    if (v < 0 || u + v > 1) { continue; }
                    const float hit = inverse * dot(other, q);
                    if (hit > radius * .002f && hit < distance) { distance = hit; }
                }
            }
            return distance;
        }
        float sample(const Sample& s) const
        {
            if (!s.opaque || nodes.empty()) { return 0; }
            const V3 normal = unit(s.normal);
            const V3 origin = s.position + normal * (radius * .003f);
            const float reach = radius * 14.f;
            float enclosed = 0, blocked = 0, total = 0;
            for (int i = 0; i < 32; ++i) {
                const float z = 1.f - 2.f * (float(i) + .5f) / 32.f;
                const float r = std::sqrt(std::max(0.f, 1 - z * z));
                const float angle = float(i) * 2.39996323f;
                const V3 direction{r * std::cos(angle), r * std::sin(angle), z};
                const float distance = trace(origin, direction, reach);
                enclosed += distance < reach ? 1.f : 0.f;
                const float weight = std::max(0.f, dot(normal, direction));
                total += weight;
                blocked += weight * std::max(0.f, 1.f - distance / (radius * 2.5f));
            }
            // open surfaces keep the existing screen-space AO exactly as before
            const float enclosure = std::clamp((enclosed / 32.f - .65f) / .2f, 0.f, 1.f);
            return std::clamp(blocked / std::max(total, .001f) * enclosure * .7f, 0.f, .35f);
        }

    public:
        std::vector<float> values;

        template<class Mesh, class Transform>
        CavityOcclusion(const Mesh& mesh, Transform transform)
        {
            radius = std::max(.001f, mesh.span() * .13f);
            size_t count = 0;
            for (const auto& b : mesh.batches) { count += b.vertices.size(); }
            samples.reserve(count);
            values.resize(count);
            triangles.reserve(count / 3);
            for (const auto& b : mesh.batches) {
                const bool opaque = b.alpha >= .98f;
                const size_t begin = samples.size();
                const auto part = transform(b.component);
                for (const auto& v : b.vertices) {
                    const auto p = part.point({v.p.x, v.p.y, v.p.z});
                    const auto n = part.vector({v.n.x, v.n.y, v.n.z});
                    samples.push_back({{p.x, p.y, p.z}, {n.x, n.y, n.z}, opaque});
                }
                if (opaque) {
                    for (size_t i = begin; i + 2 < samples.size(); i += 3) {
                        triangles.push_back({samples[i].position, samples[i + 1].position, samples[i + 2].position});
                    }
                }
            }
            if (!triangles.empty()) { node(0, triangles.size()); }
        }

        bool ready() const { return pending.empty() && cursor == samples.size(); }

        // no worker touches game or D3D objects; queries advance on a small frame budget
        void step(double milliseconds = 1.)
        {
            const auto end = std::chrono::steady_clock::now() + std::chrono::microseconds(int(milliseconds * 1000));
            do {
                if (!pending.empty()) {
                    const int at = pending.back();
                    pending.pop_back();
                    const auto n = nodes[size_t(at)];
                    if (n.end - n.begin > 8) {
                        const V3 extent = n.high - n.low;
                        const int a = extent.x > extent.y && extent.x > extent.z ? 0 : extent.y > extent.z ? 1 : 2;
                        const size_t middle = (n.begin + n.end) / 2;
                        std::nth_element(triangles.begin() + n.begin, triangles.begin() + middle,
                                         triangles.begin() + n.end, [a](const auto& x, const auto& y) {
                            return axis(x.a + x.b + x.c, a) < axis(y.a + y.b + y.c, a);
                        });
                        const int left = node(n.begin, middle), right = node(middle, n.end);
                        nodes[size_t(at)].left = left;
                        nodes[size_t(at)].right = right;
                    }
                } else if (cursor < samples.size()) {
                    const auto& s = samples[cursor];
                    const float cell = radius * .015f;
                    const std::array<int, 6> key{{int(std::round(s.position.x / cell)),
                        int(std::round(s.position.y / cell)), int(std::round(s.position.z / cell)),
                        int(std::round(s.normal.x * 16)), int(std::round(s.normal.y * 16)),
                        int(std::round(s.normal.z * 16))}};
                    const auto found = cache.find(key);
                    values[cursor] = !s.opaque ? 0 : found != cache.end() ? found->second : sample(s);
                    if (s.opaque && found == cache.end() && cache.size() < 65536) {
                        cache.emplace(key, values[cursor]);
                    }
                    ++cursor;
                } else { break; }
            } while (std::chrono::steady_clock::now() < end);
        }
    };
}
