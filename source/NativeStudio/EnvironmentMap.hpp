#pragma once
#include "../RenderTargets.hpp"
#include "../Showroom.hpp"
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace gt4::nativeStudio
{
    inline V3 environmentCubeDirection(unsigned face, float u, float v)
    {
        const float x = 2 * u - 1;
        const float y = 2 * v - 1;
        switch (face) {
            case 0:
                return {1, -x, -y};
            case 1:
                return {-1, x, -y};
            case 2:
                return {x, y, 1};
            case 3:
                return {x, -y, -1};
            case 4:
                return {x, 1, -y};
            default:
                return {-x, -1, -y};
        }
    }

    inline Camera environmentCubeCamera(unsigned face, unsigned size, V3 probe)
    {
        Camera camera(float(size), float(size), 1, .5f);
        camera.eye = probe;
        camera.forward = environmentCubeDirection(face, .5f, .5f);
        camera.right = environmentCubeDirection(face, 1, .5f) - camera.forward;
        camera.up = camera.forward - environmentCubeDirection(face, .5f, 1);
        camera.focal = float(size) * .5f;
        return camera;
    }

    inline V3 environmentSphereDirection(float u, float v)
    {
        const float x = 2 * u - 1;
        const float y = 1 - 2 * v;
        const float radius = std::sqrt(x * x + y * y);
        const float z = 2 * std::min(1.f, radius) - 1;
        const float radial = std::sqrt(std::max(0.f, 1 - z * z));
        if (radius < .000001f) {
            return {0, 0, -1};
        }
        return {x * radial / radius, y * radial / radius, z};
    }

    struct EnvironmentVertex
    {
        float x = 0, y = 0, z = 0, rhw = 1;
        float u = 0, v = 0, w = 0;
    };

    template <class Device = IDirect3DDevice9,
              class Cube = IDirect3DCubeTexture9,
              class Texture = IDirect3DTexture9,
              class Surface = IDirect3DSurface9,
              class StateBlock = IDirect3DStateBlock9,
              bool CaptureSphere = true>
    class EnvironmentMap
    {
        Device* owner = nullptr;
        Cube* cube = nullptr;
        Texture* sphere = nullptr;
        std::array<Surface*, 6> faces{};
        Surface* sphereSurface = nullptr;
        StateBlock* captureState = nullptr;
        std::vector<EnvironmentVertex> sphereVertices;
        std::vector<uint16_t> sphereIndices;
        double lastUpdate = -1;
        uint64_t lastRevision = 0;
        bool complete = false;
        bool cleanupOnDestruction = true;

        static constexpr unsigned cubeSize = CaptureSphere ? 256 : 128;
        static constexpr unsigned sphereSize = 512;
        static constexpr unsigned sphereSegments = 96;
        static constexpr DWORD textureUsage = D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP;

        template <class Resource> static void release(Resource*& resource)
        {
            if (resource) {
                resource->Release();
                resource = nullptr;
            }
        }

        static bool supportsMipmaps(Device* device, const D3DCAPS9& caps)
        {
            if (!(caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP)) {
                return false;
            }
            IDirect3D9* api = nullptr;
            const HRESULT result = device->GetDirect3D(&api);
            if (FAILED(result) || !api) {
                release(api);
                return false;
            }
            D3DDISPLAYMODE mode{};
            const bool supported = SUCCEEDED(api->GetAdapterDisplayMode(caps.AdapterOrdinal, &mode)) &&
                                   api->CheckDeviceFormat(caps.AdapterOrdinal,
                                                          caps.DeviceType,
                                                          mode.Format,
                                                          textureUsage,
                                                          D3DRTYPE_CUBETEXTURE,
                                                          D3DFMT_A8R8G8B8) == D3D_OK &&
                                   (!CaptureSphere ||
                                    api->CheckDeviceFormat(caps.AdapterOrdinal,
                                                           caps.DeviceType,
                                                           mode.Format,
                                                           textureUsage,
                                                           D3DRTYPE_TEXTURE,
                                                           D3DFMT_A8R8G8B8) == D3D_OK);
            release(api);
            return supported;
        }

        bool ensure(Device* device)
        {
            if (owner == device && cube && captureState && (!CaptureSphere || (sphere && sphereSurface))) {
                return true;
            }
            reset();
            D3DCAPS9 caps{};
            if (!device || FAILED(device->GetDeviceCaps(&caps)) || !(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) ||
                caps.MaxTextureWidth < cubeSize || caps.MaxTextureHeight < cubeSize ||
                (CaptureSphere && (caps.MaxTextureWidth < sphereSize || caps.MaxTextureHeight < sphereSize)) ||
                !supportsMipmaps(device, caps) ||
                FAILED(device->CreateCubeTexture(
                    cubeSize, 0, textureUsage, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cube, nullptr)) ||
                !cube || FAILED(cube->SetAutoGenFilterType(D3DTEXF_LINEAR))) {
                reset();
                return false;
            }
            owner = device;
            for (unsigned face = 0; face < faces.size(); ++face) {
                if (FAILED(cube->GetCubeMapSurface(D3DCUBEMAP_FACES(face), 0, &faces[face])) || !faces[face]) {
                    reset();
                    return false;
                }
            }
            if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &captureState)) || !captureState) {
                reset();
                return false;
            }
            if constexpr (CaptureSphere) {
                if (FAILED(device->CreateTexture(
                        sphereSize, sphereSize, 0, textureUsage, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &sphere, nullptr)) ||
                    !sphere || FAILED(sphere->SetAutoGenFilterType(D3DTEXF_LINEAR)) ||
                    FAILED(sphere->GetSurfaceLevel(0, &sphereSurface)) || !sphereSurface) {
                    reset();
                    return false;
                }
                if (sphereVertices.empty()) {
                    constexpr unsigned rowLength = sphereSegments + 1;
                    static_assert(rowLength * rowLength <= 65536);
                    sphereVertices.reserve(rowLength * rowLength);
                    sphereIndices.reserve(sphereSegments * sphereSegments * 6);
                    auto vertex = [](unsigned column, unsigned row) {
                        const float u = float(column) / sphereSegments;
                        const float v = float(row) / sphereSegments;
                        const V3 direction = environmentSphereDirection(u, v);
                        return EnvironmentVertex{
                            u * sphereSize - .5f, v * sphereSize - .5f, 0, 1, direction.x, direction.z, direction.y};
                    };
                    for (unsigned row = 0; row < rowLength; ++row) {
                        for (unsigned column = 0; column < rowLength; ++column) {
                            sphereVertices.push_back(vertex(column, row));
                        }
                    }
                    for (unsigned row = 0; row < sphereSegments; ++row) {
                        for (unsigned column = 0; column < sphereSegments; ++column) {
                            const auto a = uint16_t(row * rowLength + column);
                            const auto b = uint16_t(a + 1);
                            const auto c = uint16_t(a + rowLength + 1);
                            const auto d = uint16_t(a + rowLength);
                            sphereIndices.insert(sphereIndices.end(), {a, b, c, a, c, d});
                        }
                    }
                }
            }
            return true;
        }

        bool projectSphere(Device* device)
        {
            bool ready = SUCCEEDED(device->SetVertexShader(nullptr));
            ready = SUCCEEDED(device->SetPixelShader(nullptr)) && ready;
            ready = SUCCEEDED(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0))) && ready;
            ready = SUCCEEDED(device->SetTexture(0, cube)) && ready;
            for (const auto state : {std::pair<D3DTEXTURESTAGESTATETYPE, DWORD>{D3DTSS_COLOROP, D3DTOP_SELECTARG1},
                                     {D3DTSS_COLORARG1, D3DTA_TEXTURE},
                                     {D3DTSS_ALPHAOP, D3DTOP_SELECTARG1},
                                     {D3DTSS_ALPHAARG1, D3DTA_TEXTURE},
                                     {D3DTSS_RESULTARG, D3DTA_CURRENT},
                                     {D3DTSS_TEXCOORDINDEX, 0},
                                     {D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE}}) {
                ready = SUCCEEDED(device->SetTextureStageState(0, state.first, state.second)) && ready;
            }
            ready = SUCCEEDED(device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE)) && ready;
            ready = SUCCEEDED(device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE)) && ready;
            for (const auto state : {std::pair<D3DSAMPLERSTATETYPE, DWORD>{D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                                     {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                                     {D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP},
                                     {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                                     {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                                     {D3DSAMP_MIPFILTER, D3DTEXF_NONE},
                                     {D3DSAMP_SRGBTEXTURE, FALSE}}) {
                ready = SUCCEEDED(device->SetSamplerState(0, state.first, state.second)) && ready;
            }
            for (const auto state : {std::pair<D3DRENDERSTATETYPE, DWORD>{D3DRS_ZENABLE, FALSE},
                                     {D3DRS_ZWRITEENABLE, FALSE},
                                     {D3DRS_ALPHABLENDENABLE, FALSE},
                                     {D3DRS_ALPHATESTENABLE, FALSE},
                                     {D3DRS_FOGENABLE, FALSE},
                                     {D3DRS_STENCILENABLE, FALSE},
                                     {D3DRS_SCISSORTESTENABLE, FALSE},
                                     {D3DRS_CULLMODE, D3DCULL_NONE},
                                     {D3DRS_FILLMODE, D3DFILL_SOLID},
                                     {D3DRS_COLORWRITEENABLE, 15},
                                     {D3DRS_SRGBWRITEENABLE, FALSE}}) {
                ready = SUCCEEDED(device->SetRenderState(state.first, state.second)) && ready;
            }
            if (!ready) {
                return false;
            }
            const bool drawn = SUCCEEDED(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
                                                                        0,
                                                                        UINT(sphereVertices.size()),
                                                                        UINT(sphereIndices.size() / 3),
                                                                        sphereIndices.data(),
                                                                        D3DFMT_INDEX16,
                                                                        sphereVertices.data(),
                                                                        sizeof(EnvironmentVertex)));
            return SUCCEEDED(device->SetTexture(0, nullptr)) && drawn;
        }

    public:
        explicit EnvironmentMap(bool cleanup = true)
            : cleanupOnDestruction(cleanup)
        {
        }

        ~EnvironmentMap()
        {
            if (cleanupOnDestruction) {
                reset();
            }
        }

        EnvironmentMap(const EnvironmentMap&) = delete;
        EnvironmentMap& operator=(const EnvironmentMap&) = delete;

        void reset()
        {
            complete = false;
            lastUpdate = -1;
            owner = nullptr;
            release(captureState);
            release(sphereSurface);
            for (auto*& face : faces) {
                release(face);
            }
            release(sphere);
            release(cube);
        }

        Cube* cubeTexture() const
        {
            return complete ? cube : nullptr;
        }

        Texture* sphereTexture() const
        {
            return complete ? sphere : nullptr;
        }

        template <class DrawScene>
        bool update(Device* device,
                    uint64_t revision,
                    double now,
                    V3 probe,
                    DrawScene drawScene,
                    double minimumInterval = 1. / 30)
        {
            if (!std::isfinite(now) || !std::isfinite(probe.x) || !std::isfinite(probe.y) || !std::isfinite(probe.z)) {
                complete = false;
                return false;
            }
            if (complete && owner == device && revision == lastRevision && now >= lastUpdate &&
                now - lastUpdate < minimumInterval) {
                return true;
            }
            complete = false;
            if (!ensure(device)) {
                return false;
            }

            struct CaptureScope
            {
                Device* device;
                RenderTargets<Device, Surface> targets;
                StateBlock* state = nullptr;
                bool restored = false;

                explicit CaptureScope(Device* value, StateBlock* block)
                    : device(value),
                      targets(value)
                {
                    if (targets.valid() && block && SUCCEEDED(block->Capture())) {
                        state = block;
                    }
                }

                ~CaptureScope()
                {
                    restore();
                }

                bool restore()
                {
                    if (restored) {
                        return true;
                    }
                    const bool stateRestored = state && SUCCEEDED(state->Apply());
                    restored = targets.restore() && stateRestored;
                    return restored;
                }
            } capture(device, captureState);

            if (!capture.state || !capture.targets.valid()) {
                return false;
            }
            const D3DVIEWPORT9 cubeView{0, 0, cubeSize, cubeSize, 0, 1};
            for (unsigned face = 0; face < faces.size(); ++face) {
                if (FAILED(device->SetTexture(0, nullptr)) || FAILED(device->SetTexture(1, nullptr)) ||
                    !capture.targets.select(faces[face], nullptr, cubeView) ||
                    FAILED(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1, 0)) ||
                    !drawScene(environmentCubeCamera(face, cubeSize, probe))) {
                    return false;
                }
            }
            if constexpr (CaptureSphere) {
                const D3DVIEWPORT9 sphereView{0, 0, sphereSize, sphereSize, 0, 1};
                if (!capture.targets.select(sphereSurface, nullptr, sphereView)) {
                    return false;
                }
                cube->GenerateMipSubLevels();
                if (!projectSphere(device)) {
                    return false;
                }
            }
            if (!capture.restore()) {
                return false;
            }
            if constexpr (CaptureSphere) {
                sphere->GenerateMipSubLevels();
            } else {
                cube->GenerateMipSubLevels();
            }
            lastUpdate = now;
            lastRevision = revision;
            complete = true;
            return true;
        }
    };
}
