std::vector<ScreenVertex> viewerSurfaceVertices, viewerPatternVertices;
std::vector<uint16_t> viewerSurfaceIndices;
std::array<ScreenVertex, 65 * 37> viewerSurfaceGrid{};
std::array<float, 18> viewerSurfaceCamera{};
double viewerSurfaceTime = -1;
ViewerScene viewerSurfaceScene = ViewerScene::Softbox;
uint64_t viewerSurfaceStyle = 0;

struct ViewerPatternLine
{
    V3 first, second;
    uint32_t firstColor, secondColor;
    float alpha, width;
};

std::vector<std::array<ViewerWaveVertex, 3>> viewerWaveMesh;
struct ViewerWaveBounds
{
    V3 low, high;
};

static constexpr size_t viewerWaveTrianglesPerSegment = 24;
std::vector<ViewerWaveBounds> viewerWaveBounds;
std::vector<ViewerPatternLine> viewerPatternMesh;
double viewerPatternTime = -1;
ViewerScene viewerPatternScene = ViewerScene::Softbox;
int viewerPatternPalette = -1;

void updateViewerPatternMesh(const VehicleViewer& viewer, double now)
{
    const double time = viewer.backgroundElapsed(now);
    const auto scene = viewer.settings.scene;
    const int palette = viewer.settings.palette;
    if (time == viewerPatternTime && scene == viewerPatternScene && palette == viewerPatternPalette) {
        return;
    }

    viewerWaveMesh.clear();
    viewerWaveBounds.clear();
    viewerPatternMesh.clear();
    if (viewerWaveScene(scene)) {
        viewerWaveMesh.reserve(720 * viewerWaveTrianglesPerSegment);
        viewerWaveBounds.reserve(720);
    }
    viewerPatternMesh.reserve(scene == ViewerScene::Ribbons ? 20 * viewerRibbonSegmentCount : 720);
    viewerWaveTriangles(viewer, now, [&](ViewerWaveVertex a, ViewerWaveVertex b, ViewerWaveVertex c) {
        if (viewerWaveMesh.size() % viewerWaveTrianglesPerSegment == 0) {
            viewerWaveBounds.push_back({a.point, a.point});
        }
        auto& bounds = viewerWaveBounds.back();
        for (V3 point : {a.point, b.point, c.point}) {
            bounds.low = {std::min(bounds.low.x, point.x),
                          std::min(bounds.low.y, point.y),
                          std::min(bounds.low.z, point.z)};
            bounds.high = {std::max(bounds.high.x, point.x),
                           std::max(bounds.high.y, point.y),
                           std::max(bounds.high.z, point.z)};
        }
        viewerWaveMesh.push_back({a, b, c});
    });
    viewerPatternLines(viewer, now, [&](V3 a, V3 b, uint32_t first, uint32_t second, float alpha, float width) {
        viewerPatternMesh.push_back({a, b, first, second, alpha, width});
    });
    viewerPatternTime = time;
    viewerPatternScene = scene;
    viewerPatternPalette = palette;
}

uint64_t viewerStudioStyle(const ViewerSettings& settings)
{
    constexpr uint64_t surfaceChoices = viewerWhite + 2;
    const uint64_t lightAndScene = uint64_t(settings.scene) * viewerPalettes.size() + settings.palette;
    const uint64_t wallAndFloor = uint64_t(settings.wallColor + 1) * surfaceChoices + settings.floorColor + 1;
    return 1 + lightAndScene * surfaceChoices * surfaceChoices + wallAndFloor;
}

void releaseCustomStudio()
{
    if (customStudioTexture) {
        customStudioTexture->Release();
        customStudioTexture = nullptr;
    }
    customStudioLoadAttempted = false;
}

bool drawCustomStudio(IDirect3DDevice9* device, const Camera& camera)
{
    if (!customStudioLoadAttempted) {
        customStudioLoadAttempted = true;
        if (!customStudioPath.empty()) {
            customStudioTexture = VehicleLogos::loadPanorama(device, customStudioPath);
        }
    }
    if (!customStudioTexture) {
        return false;
    }

    constexpr int columns = 64;
    constexpr int rows = 36;
    std::array<ScreenVertex, (columns + 1) * (rows + 1)> vertices{};
    const float center = std::atan2(camera.forward.y, camera.forward.x);
    for (int row = 0; row <= rows; ++row) {
        for (int column = 0; column <= columns; ++column) {
            const float x = camera.w * column / columns;
            const float y = camera.h * row / rows;
            const V3 ray = unit(camera.forward +
                                camera.right * ((x - camera.w * .5f) / camera.focal) +
                                camera.up * ((camera.cy - y) / camera.focal));
            const float longitude = std::atan2(ray.y, ray.x);
            const float u = .5f + (center + std::remainder(longitude - center, VehicleViewer::fullTurn)) /
                                      VehicleViewer::fullTurn;
            const float v = .5f - std::asin(std::clamp(ray.z, -1.f, 1.f)) / pi;
            vertices[size_t(row * (columns + 1) + column)] = {x, y, .999f, 1, 0xFFFFFFFF, u, v};
        }
    }
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetTexture(0, customStudioTexture);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    const bool drawn = SUCCEEDED(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
                                                               0,
                                                               UINT(vertices.size()),
                                                               UINT(viewerSurfaceIndices.size() / 3),
                                                               viewerSurfaceIndices.data(),
                                                               D3DFMT_INDEX16,
                                                               vertices.data(),
                                                               sizeof(ScreenVertex)));
    device->SetTexture(0, nullptr);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    return drawn;
}

bool viewerStudio(IDirect3DDevice9* d,
                  const Camera& camera,
                  const VehicleViewer& viewer,
                  double now,
                  bool glaze = false,
                  float outputScaleX = 1,
                  float outputScaleY = 1)
{
    screen(d);
    viewerSurfaceVertices.clear();
    constexpr size_t gridIndexCount = 64 * 36 * 6;
    if (viewerSurfaceIndices.empty()) {
        viewerSurfaceVertices.reserve(viewerSurfaceGrid.size() + 624);
        viewerSurfaceIndices.reserve(gridIndexCount + 624);
        for (uint16_t row = 0; row < 36; ++row) {
            for (uint16_t column = 0; column < 64; ++column) {
                const auto a = uint16_t(row * 65 + column);
                const auto b = uint16_t(a + 1);
                const auto c = uint16_t(a + 66);
                const auto e = uint16_t(a + 65);
                viewerSurfaceIndices.insert(viewerSurfaceIndices.end(), {a, b, c, a, c, e});
            }
        }
    }
    viewerSurfaceIndices.resize(gridIndexCount);
    const std::array<float, 18> view{camera.eye.x,
                                     camera.eye.y,
                                     camera.eye.z,
                                     camera.right.x,
                                     camera.right.y,
                                     camera.right.z,
                                     camera.up.x,
                                     camera.up.y,
                                     camera.up.z,
                                     camera.forward.x,
                                     camera.forward.y,
                                     camera.forward.z,
                                     camera.focal,
                                     camera.w,
                                     camera.h,
                                     camera.cy,
                                     camera.offsetX,
                                     camera.offsetY};
    const auto scene = viewer.settings.scene;
    const uint64_t style = viewerStudioStyle(viewer.settings);
    const double time = viewerWaveScene(scene) ? viewer.backgroundElapsed(now) : 0;
    if (view != viewerSurfaceCamera || time != viewerSurfaceTime || scene != viewerSurfaceScene ||
        style != viewerSurfaceStyle) {
        for (int row = 0; row <= 36; ++row) {
            for (int column = 0; column <= 64; ++column) {
                const float x = camera.w * column / 64, y = camera.h * row / 36;
                const auto sample = viewerSurface(camera, x, y, viewer, now);
                auto& vertex = viewerSurfaceGrid[size_t(row * 65 + column)];
                vertex = {};
                camera.project(sample.point, vertex.x, vertex.y, vertex.z, vertex.rhw);
                vertex.color = color(sample.rgb, sample.floorCoverage * .42f);
            }
        }
        viewerSurfaceCamera = view;
        viewerSurfaceTime = time;
        viewerSurfaceScene = scene;
        viewerSurfaceStyle = style;
    }
    if (!glaze && scene == ViewerScene::Custom && drawCustomStudio(d, camera)) {
        return true;
    }
    viewerSurfaceVertices.assign(viewerSurfaceGrid.begin(), viewerSurfaceGrid.end());
    if (glaze) {
        if (outputScaleX != 1 || outputScaleY != 1) {
            for (auto& vertex : viewerSurfaceVertices) {
                vertex.x *= outputScaleX;
                vertex.y *= outputScaleY;
            }
        }
    } else {
        for (auto& vertex : viewerSurfaceVertices) {
            vertex.color |= 0xFF000000;
        }
    }
    d->SetTexture(0, nullptr);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, glaze);
    d->SetRenderState(D3DRS_ZENABLE, FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    const bool drawn = SUCCEEDED(d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
                                                           0,
                                                           UINT(viewerSurfaceVertices.size()),
                                                           UINT(viewerSurfaceIndices.size() / 3),
                                                           viewerSurfaceIndices.data(),
                                                           D3DFMT_INDEX16,
                                                           viewerSurfaceVertices.data(),
                                                           sizeof(ScreenVertex)));
    triangles += viewerSurfaceIndices.size() / 3;
    ++drawCalls;
    return drawn;
}

bool viewerPatterns(IDirect3DDevice9* d, const Camera& camera, const VehicleViewer& viewer, double now, bool reflected)
{
    const auto scene = viewer.settings.scene;
    if (scene != ViewerScene::Ribbons && !viewerWaveScene(scene)) {
        return true;
    }
    updateViewerPatternMesh(viewer, now);
    screen(d);
    viewerPatternVertices.clear();
    constexpr size_t capacity = 720 * 12 * 2 * 6;
    if (viewerPatternVertices.capacity() < capacity) {
        viewerPatternVertices.reserve(capacity);
    }
    const float scale = camera.focal / 1008.f;
    const float margin = 2 * std::max(1.f, scale);
    const auto outside = [&](V3 point) {
        if (reflected) {
            point.z = -point.z;
        }
        const V3 offset = point - camera.eye;
        const float depth = dot(offset, camera.forward);
        const float x = dot(offset, camera.right) * camera.focal;
        const float y = dot(offset, camera.up) * camera.focal;
        unsigned mask = 0;
        if (x < -(camera.w * .5f + margin) * depth) {
            mask |= 1;
        }
        if (x > (camera.w * .5f + margin) * depth) {
            mask |= 2;
        }
        if (y < -(camera.h - camera.cy + margin) * depth) {
            mask |= 4;
        }
        if (y > (camera.cy + margin) * depth) {
            mask |= 8;
        }
        return mask;
    };
    for (size_t segment = 0; segment < viewerWaveBounds.size(); ++segment) {
        const auto& bounds = viewerWaveBounds[segment];
        if (bounds.high.z < 0) {
            continue;
        }
        unsigned mask = 15;
        for (float x : {bounds.low.x, bounds.high.x}) {
            for (float y : {bounds.low.y, bounds.high.y}) {
                for (float z : {std::max(0.f, bounds.low.z), bounds.high.z}) {
                    mask &= outside({x, y, z});
                }
            }
        }
        if (mask) {
            continue;
        }
        const size_t start = segment * viewerWaveTrianglesPerSegment;
        const size_t end = std::min(start + viewerWaveTrianglesPerSegment, viewerWaveMesh.size());
        for (size_t index = start; index < end; ++index) {
            const auto& triangle = viewerWaveMesh[index];
            viewerClipWaveTriangle(
                camera,
                triangle[0],
                triangle[1],
                triangle[2],
                reflected,
                [&](const ViewerWaveVertex& first, const ViewerWaveVertex& second, const ViewerWaveVertex& third) {
                    for (const auto* source : {&first, &second, &third}) {
                        ScreenVertex vertex{};
                        camera.project(source->point, vertex.x, vertex.y, vertex.z, vertex.rhw);
                        vertex.color = color(source->rgb, source->alpha);
                        viewerPatternVertices.push_back(vertex);
                    }
                });
        }
    }
    if (scene == ViewerScene::Ribbons) {
        constexpr size_t segments = viewerRibbonSegmentCount;
        constexpr std::array<float, 5> offsets{-1.f, -.45f, 0.f, .45f, 1.f};
        constexpr std::array<float, 5> falloff{0.f, .48f, 1.f, .48f, 0.f};
        for (size_t band = 0; band + segments <= viewerPatternMesh.size(); band += segments) {
            std::array<ScreenVertex, segments> projected{};
            std::array<bool, segments> visible{};
            std::array<std::array<ScreenVertex, offsets.size()>, segments> edges{};
            for (size_t i = 0; i < segments; ++i) {
                V3 point = viewerPatternMesh[band + i].first;
                if (point.z < 0) {
                    continue;
                }
                if (reflected) {
                    point.z = -point.z;
                }
                auto& vertex = projected[i];
                visible[i] = camera.project(point, vertex.x, vertex.y, vertex.z, vertex.rhw);
            }
            for (size_t i = 0; i < segments; ++i) {
                if (!visible[i]) {
                    continue;
                }
                const auto& source = viewerPatternMesh[band + i];
                const auto& center = projected[i];
                const size_t before = (i + segments - 1) % segments;
                const size_t after = (i + 1) % segments;
                const auto& previous = visible[before] ? projected[before] : center;
                const auto& next = visible[after] ? projected[after] : center;
                const float dx = next.x - previous.x;
                const float dy = next.y - previous.y;
                const float length = std::sqrt(dx * dx + dy * dy);
                if (length < .001f) {
                    visible[i] = false;
                    continue;
                }
                const float width = source.width * scale;
                const float radius = std::max(1.3f, width * 1.65f);
                const float coverage = source.alpha * .78f * std::clamp(width, .45f, 1.f);
                const float alpha = reflected ? coverage * .18f : coverage;
                for (size_t edge = 0; edge < offsets.size(); ++edge) {
                    auto vertex = center;
                    const float distance = offsets[edge] * radius / length;
                    vertex.x -= dy * distance;
                    vertex.y += dx * distance;
                    vertex.color = color(source.firstColor, alpha * falloff[edge]);
                    edges[i][edge] = vertex;
                }
            }
            for (size_t i = 0; i < segments; ++i) {
                const size_t next = (i + 1) % segments;
                if (!visible[i] || !visible[next] ||
                    (outside(viewerPatternMesh[band + i].first) & outside(viewerPatternMesh[band + next].first))) {
                    continue;
                }
                for (size_t edge = 0; edge + 1 < offsets.size(); ++edge) {
                    const auto& a = edges[i];
                    const auto& b = edges[next];
                    viewerPatternVertices.insert(viewerPatternVertices.end(),
                                                 {a[edge], b[edge], b[edge + 1], a[edge], b[edge + 1], a[edge + 1]});
                }
            }
        }
    } else {
        for (const auto& source : viewerPatternMesh) {
            V3 first = source.first;
            V3 second = source.second;
            if ((outside(first) & outside(second)) || !viewerClipFloor(first, second)) {
                continue;
            }
            float alpha = source.alpha;
            if (reflected) {
                first.z = -first.z;
                second.z = -second.z;
                alpha *= .18f;
            }
            ScreenVertex p{}, q{};
            if (!camera.project(first, p.x, p.y, p.z, p.rhw) || !camera.project(second, q.x, q.y, q.z, q.rhw)) {
                continue;
            }
            const float dx = q.x - p.x;
            const float dy = q.y - p.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            if (length < .001f) {
                continue;
            }
            const float pixels = source.width * scale;
            alpha *= std::min(1.f, pixels);
            const float radius = std::max(1.f, pixels) * .5f;
            const float nx = -dy / length * radius;
            const float ny = dx / length * radius;
            p.color = color(source.firstColor, alpha);
            q.color = color(source.secondColor, alpha);
            ScreenVertex a0 = p, a1 = p, b0 = q, b1 = q;
            a0.x -= nx;
            a0.y -= ny;
            a1.x += nx;
            a1.y += ny;
            b0.x -= nx;
            b0.y -= ny;
            b1.x += nx;
            b1.y += ny;
            viewerPatternVertices.insert(viewerPatternVertices.end(), {a0, b0, b1, a0, b1, a1});
        }
    }
    d->SetTexture(0, nullptr);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d->SetRenderState(D3DRS_ZENABLE, FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    if (!viewerPatternVertices.empty()) {
        const bool drawn = SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                        UINT(viewerPatternVertices.size() / 3),
                                                        viewerPatternVertices.data(),
                                                        sizeof(ScreenVertex)));
        triangles += viewerPatternVertices.size() / 3;
        ++drawCalls;
        return drawn;
    }
    return true;
}

void updateStudioEnvironment(IDirect3DDevice9* d, const VehicleViewer& viewer, double clock)
{
    if (!studioEnvironmentAvailable) {
        return;
    }
    const auto scene = viewer.settings.scene;
    const bool animated = scene == ViewerScene::Ribbons || viewerWaveScene(scene);
    const double time = animated ? viewer.backgroundElapsed(clock) : 0.;
    const uint64_t revision = viewerStudioStyle(viewer.settings);
    constexpr double captureInterval = 1. / 8.;
    studioEnvironmentAvailable = studioEnvironment.update(
        d,
        revision,
        time,
        {0, 0, .8f},
        [&](const Camera& view) {
            ownedState(d);
            return viewerStudio(d, view, viewer, clock) && viewerPatterns(d, view, viewer, clock, false);
        },
        captureInterval);
}

void releaseViewerTarget()
{
    if (viewerDepth) {
        viewerDepth->Release();
        viewerDepth = nullptr;
    }
    if (viewerTargetSurface) {
        viewerTargetSurface->Release();
        viewerTargetSurface = nullptr;
    }
    if (viewerTarget) {
        viewerTarget->Release();
        viewerTarget = nullptr;
    }
    viewerTargetWidth = 0;
    viewerTargetHeight = 0;
}

bool allocateViewerTarget(IDirect3DDevice9* d, UINT width, UINT height)
{
    if (viewerTargetWidth == width && viewerTargetHeight == height) {
        return viewerTarget && viewerTargetSurface && viewerDepth;
    }
    releaseViewerTarget();
    if (FAILED(d->CreateTexture(
            width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &viewerTarget, nullptr)) ||
        !viewerTarget || FAILED(viewerTarget->GetSurfaceLevel(0, &viewerTargetSurface)) || !viewerTargetSurface ||
        FAILED(d->CreateDepthStencilSurface(
            width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &viewerDepth, nullptr)) ||
        !viewerDepth) {
        releaseViewerTarget();
    }
    viewerTargetWidth = width;
    viewerTargetHeight = height;
    return viewerTarget && viewerTargetSurface && viewerDepth;
}

bool viewerRoom(IDirect3DDevice9* d,
                const Camera& camera,
                const VehicleViewer& viewer,
                double now,
                const Mesh& car,
                float angle,
                const Paint& paint,
                float opacity,
                float carSize,
                const CinematicVehicleState* vehicle,
                float rideHeight = 0,
                bool clean = false)
{
    if (viewer.settings.scene == ViewerScene::Custom) {
        return viewerStudio(d, camera, viewer, now);
    }
    const bool fullResolutionRibbons = viewer.settings.scene == ViewerScene::Ribbons && camera.h > 720.f;
    const auto backdrop = [&](const Camera& view, bool includeRibbons) {
        return viewerStudio(d, view, viewer, now) &&
               ((fullResolutionRibbons && !includeRibbons) || viewerPatterns(d, view, viewer, now, false)) &&
               viewerPatterns(d, view, viewer, now, true);
    };
    const auto direct = [&] {
        return backdrop(camera, true) && viewerStudio(d, camera, viewer, now, true);
    };
    // keep the animated bands above a subpixel grid on smaller displays. the foreground car stays native-sized.
    const float scale = camera.h <= 720.f
        ? std::min(2.f, 720.f / std::max(1.f, camera.h))
        : std::min(.5f, 720.f / std::max(camera.w, camera.h));
    const UINT width = UINT(std::max(1.f, std::round(camera.w * scale)));
    const UINT height = UINT(std::max(1.f, std::round(camera.h * scale)));
    if (!allocateViewerTarget(d, width, height)) {
        return direct();
    }

    RenderTargets layers(d);
    if (!layers.valid()) {
        return direct();
    }
    const D3DVIEWPORT9 viewport{0, 0, width, height, 0, 1};
    Camera reduced = camera;
    reduced.w = float(width);
    reduced.h = float(height);
    reduced.focal *= reduced.h / camera.h;
    reduced.cy *= reduced.h / camera.h;
    reduced.offsetX *= reduced.w / camera.w;
    reduced.offsetY *= reduced.h / camera.h;
    d->SetTexture(0, nullptr);
    const bool roomDrawn = layers.select(viewerTargetSurface, nullptr, viewport) &&
                           SUCCEEDED(d->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF0F1012, 1, 0)) && backdrop(reduced, false);
    if (!layers.restore()) {
        return false;
    }
    if (!roomDrawn) {
        return direct();
    }

    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    d->SetRenderState(D3DRS_ZENABLE, FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    if (camera.h <= 720.f) {
        // integrate neighbouring samples instead of sharpening subpixel bands when the layer shrinks.
        constexpr std::array<std::array<float, 3>, 5> taps{{
            {{0.f, 0.f, .5f}}, {{-1.25f, 0.f, .125f}}, {{1.25f, 0.f, .125f}},
            {{0.f, -1.25f, .125f}}, {{0.f, 1.25f, .125f}}
        }};
        for (size_t i = 0; i < taps.size(); ++i) {
            d->SetRenderState(D3DRS_ALPHABLENDENABLE, i != 0);
            d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            const float u = taps[i][0] / camera.w, v = taps[i][1] / camera.h;
            rect(d, 0, 0, camera.w, camera.h, color(0xFFFFFF, 1.f, taps[i][2]), viewerTarget,
                 u, v, 1.f + u, 1.f + v);
        }
        d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    } else {
        rect(d, 0, 0, camera.w, camera.h, 0xFFFFFFFF, viewerTarget);
    }
    d->SetTexture(0, nullptr);
    if (fullResolutionRibbons && !viewerPatterns(d, camera, viewer, now, false)) {
        return direct();
    }

    if (viewer.settings.reflections && opacity > 0 && !car.empty()) {
        const bool reflectionReady = layers.select(viewerTargetSurface, viewerDepth, viewport) &&
                                     SUCCEEDED(d->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0)) &&
                                     SUCCEEDED(d->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE)) &&
                                     SUCCEEDED(d->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA)) &&
                                     SUCCEEDED(d->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD)) &&
                                     SUCCEEDED(d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE));
        if (reflectionReady) {
            mesh(d,
                 car,
                 reduced,
                 angle,
                 paint,
                 1,
                 carSize,
                 false,
                 true,
                 false,
                 vehicle,
                 false,
                 rideHeight,
                 false,
                 clean);
        }
        d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        if (!layers.restore()) {
            return false;
        }
        if (reflectionReady) {
            const float amount = opacity * viewerReflectionOpacity;
            d->SetRenderState(D3DRS_ZENABLE, FALSE);
            d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            rect(d, 0, 0, camera.w, camera.h, color(0xFFFFFF, amount, amount), viewerTarget);
            d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        }
    }
    return viewerStudio(d, reduced, viewer, now, true, camera.w / reduced.w, camera.h / reduced.h);
}

void drawReviewPanel(IDirect3DDevice9* d,
                     float width,
                     float height,
                     float reveal,
                     float offset,
                     const std::string& title,
                     const std::string& first,
                     const std::string& second,
                     const char* const* buttons,
                     int count,
                     int selected,
                     const char* third = nullptr)
{
    const float scale = height / 720.f;
    auto panel = spawnReviewPanel(width, height);
    panel.y += offset;
    const float edge = std::max(.6f, scale);
    rect(d, 0, 0, width, height, color(0, .2f * reveal));
    rect(d, panel.x + 4 * scale, panel.y + 5 * scale, panel.w, panel.h, color(0, .28f * reveal));
    rect(d, panel.x - edge, panel.y - edge, panel.w + 2 * edge, panel.h + 2 * edge, color(0x747579, reveal));
    rect(d, panel.x, panel.y, panel.w, panel.h, color(0xB3B3B8, reveal));
    rect(d, panel.x, panel.y, panel.w, 29 * scale, color(0x17130B, reveal));
    text(d,
         title,
         width * .5f,
         panel.y + 7 * scale,
         fitted(title, 13 * scale, panel.w - 20 * scale, true),
         color(0xD0D0D2, reveal),
         true,
         0,
         true);
    text(d,
         first,
         width * .5f,
         panel.y + 48 * scale,
         fitted(first, 14.5f * scale, panel.w - 32 * scale, true),
         color(0x111214, reveal),
         true,
         0,
         true);
    text(d,
         second,
         width * .5f,
         panel.y + 66 * scale,
         fitted(second, 12.5f * scale, panel.w - 32 * scale, true),
         color(0x111214, reveal),
         true,
         0,
         true);
    if (third) {
        text(d,
             third,
             width * .5f,
             panel.y + 84 * scale,
             fitted(third, 12.5f * scale, panel.w - 32 * scale, true),
             color(0x111214, reveal),
             true,
             0,
             true);
    }
    for (int i = 0; i < count; ++i) {
        auto button = spawnReviewButton(width, height, i, count);
        button.y += offset;
        monitorButton(d, button, buttons[size_t(i)], reveal, scale, selected == i, 4, 12.5f);
    }
}

void spawnReviewDialog(IDirect3DDevice9* d, float width, float height, const Model& state)
{
    if (state.spawnReview == SpawnReview::None) {
        return;
    }
    if (state.spawnReview == SpawnReview::LiveryWarning) {
        constexpr std::array<const char*, 2> buttons{"Keep this Color", "Switch to White"};
        drawReviewPanel(d,
                        width,
                        height,
                        state.reviewReveal(),
                        state.reviewOffset(height),
                        "Livery Warning",
                        "This vehicle has custom liveries and it's recommended for you",
                        "to switch its color to white. Would you like the switch or would you",
                        buttons.data(),
                        int(buttons.size()),
                        state.reviewChoice,
                        "rather keep whichever color you chose?");
        return;
    }
    if (state.spawnReview == SpawnReview::ShopReset) {
        constexpr std::array<const char*, 3> buttons{"Reset Wheels", "Cancel", "Reset Components"};
        drawReviewPanel(d,
                        width,
                        height,
                        state.reviewReveal(),
                        state.reviewOffset(height),
                        "Customization",
                        "Return the wheels or body components to stock?",
                        "Choose the parts you would like to reset",
                        buttons.data(),
                        int(buttons.size()),
                        state.reviewChoice);
        return;
    }
    if (state.spawnReview == SpawnReview::ShopExit) {
        const char* back = state.shop.fromViewer ? "Return to Viewer" : "Return to Cinematic";
        const std::array<const char*, 3> buttons{back, "Exit", "Spawn Now"};
        drawReviewPanel(d,
                        width,
                        height,
                        state.reviewReveal(),
                        state.reviewOffset(height),
                        "Vehicle Viewer",
                        "Are you sure you would like to spawn the vehicle now?",
                        "Return to your previous view or exit to the vehicle selection",
                        buttons.data(),
                        int(buttons.size()),
                        state.reviewChoice);
        return;
    }
    const bool canSpawn = state.reviewButtons() == 3;
    const std::string first = canSpawn ? "Are you sure you would like to spawn the vehicle now?"
                                       : "Return to Cinematic or exit to the vehicle selection?";
    const std::string second = state.spawnReview == SpawnReview::Initial
                                   ? "You can enter Cinematic Mode to inspect your vehicle"
                                   : "Use Back to return to the previous view";
    const char* primary = state.spawnReview == SpawnReview::Initial ? "Enter Cinematic" : "Back to Cinematic";
    const std::array<const char*, 3> buttons{primary, "Exit", "Spawn Now"};
    const int count = state.reviewButtons();
    drawReviewPanel(d, width, height, state.reviewReveal(), state.reviewOffset(height), "Vehicle Viewer",
                    first, second, buttons.data(), count, state.reviewChoice);
}
