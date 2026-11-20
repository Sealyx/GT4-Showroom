#pragma once

#include <cmath>
#include "PreviewOcclusion.hpp"

namespace gt4
{
    class PrologueSky
    {
        static constexpr UINT cloudWidth = 768, cloudHeight = 288;
        IDirect3DVolumeTexture9* volume = nullptr;
        IDirect3DTexture9* cloudLayer = nullptr;
        IDirect3DSurface9* cloudSurface = nullptr;
        IDirect3DPixelShader9* clouds = nullptr;
        IDirect3DPixelShader9* studio = nullptr;
        bool attempted = false;
        float renderedPhase = -1.f, renderedDrift = -1.f, renderedEvolution = -1.f;

        static float noise(float x, float y, float z, int repeat)
        {
            const auto hash = [repeat](int x, int y, int z) {
                const uint32_t mask = uint32_t(repeat - 1);
                uint32_t hash = (uint32_t(x) & mask) * 0x8DA6B343u ^
                    (uint32_t(y) & mask) * 0xD8163841u ^ (uint32_t(z) & mask) * 0xCB1AB31Fu;
                hash ^= hash >> 15;
                hash *= 0x85EBCA6Bu;
                return float((hash ^ (hash >> 13)) & 65535u) / 65535.f;
            };
            const auto weight = [](float t) { return t*t*t * (t * (t * 6.f - 15.f) + 10.f); };
            const auto blend = [](float a, float b, float t) { return a + (b-a) * t; };
            x *= repeat; y *= repeat; z *= repeat;
            const int ix = int(std::floor(x)), iy = int(std::floor(y)), iz = int(std::floor(z));
            const float dx = weight(x - ix), dy = weight(y - iy), dz = weight(z - iz);
            const auto slice = [&](int layer) {
                const float top = blend(hash(ix, iy, layer), hash(ix+1, iy, layer), dx);
                const float bottom = blend(hash(ix, iy+1, layer), hash(ix+1, iy+1, layer), dx);
                return blend(top, bottom, dy);
            };
            return blend(slice(iz), slice(iz+1), dz);
        }

        bool prepareVolume(IDirect3DDevice9* device)
        {
            constexpr UINT width = 128, height = 64, depth = 32;
            if (FAILED(device->CreateVolumeTexture(width, height, depth, 1, 0, D3DFMT_A8R8G8B8,
                                                   D3DPOOL_MANAGED, &volume, nullptr))) {
                return false;
            }
            D3DLOCKED_BOX lock{};
            if (FAILED(volume->LockBox(0, &lock, nullptr, 0))) { return false; }
            for (UINT z = 0; z < depth; ++z) {
                const float w = (z + .5f) / depth;
                for (UINT y = 0; y < height; ++y) {
                    auto* voxels = reinterpret_cast<DWORD*>(static_cast<uint8_t*>(lock.pBits) +
                                                            z * lock.SlicePitch + y * lock.RowPitch);
                    const float v = (y + .5f) / height;
                    for (UINT x = 0; x < width; ++x) {
                        const float u = (x + .5f) / width;
                        const float coarse = noise(u, v, w, 8);
                        const float detail = noise(u + .23f, v + .47f, w + .17f, 16);
                        const float fine = noise(u + .49f, v + .12f, w + .37f, 32);
                        const float density = .51f * coarse + .32f * detail + .17f * fine;
                        const auto packed = DWORD(std::lround(density * 65535.f));
                        voxels[x] = 0xFF000000 | ((packed >> 8) << 16) | ((packed & 255) << 8);
                    }
                }
            }
            return SUCCEEDED(volume->UnlockBox(0));
        }

        static IDirect3DPixelShader9* compileShader(IDirect3DDevice9* device, CompilePreviewShader compile,
                                                  const char* source, const char* entry)
        {
            ID3DBlob* bytecode = nullptr;
            IDirect3DPixelShader9* output = nullptr;
            if (SUCCEEDED(compile(source, std::strlen(source), "PrologueSky", nullptr, nullptr, entry, "ps_3_0",
                                  D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &bytecode, nullptr)) && bytecode) {
                device->CreatePixelShader(static_cast<const DWORD*>(bytecode->GetBufferPointer()), &output);
            }
            if (bytecode) { bytecode->Release(); }
            return output;
        }

        void release()
        {
            if (clouds) { clouds->Release(); }
            if (studio) { studio->Release(); }
            if (volume) { volume->Release(); }
            if (cloudSurface) { cloudSurface->Release(); }
            if (cloudLayer) { cloudLayer->Release(); }
            volume = nullptr;
            cloudLayer = nullptr;
            cloudSurface = nullptr;
            clouds = studio = nullptr;
        }

        static void sampleLinear(IDirect3DDevice9* device, DWORD slot, DWORD address)
        {
            device->SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            device->SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            device->SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            device->SetSamplerState(slot, D3DSAMP_ADDRESSU, address);
            device->SetSamplerState(slot, D3DSAMP_ADDRESSV, address);
            device->SetSamplerState(slot, D3DSAMP_ADDRESSW, address);
            device->SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, FALSE);
        }

        static bool quad(IDirect3DDevice9* device, float width, float height, float opacity)
        {
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, opacity < 1.f);
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            struct Vertex { float x, y, z, rhw, u, v; };
            const Vertex vertices[]{
                {-.5f, -.5f, 1.f, 1.f, 0.f, 0.f}, {width - .5f, -.5f, 1.f, 1.f, 1.f, 0.f},
                {-.5f, height - .5f, 1.f, 1.f, 0.f, 1.f},
                {width - .5f, height - .5f, 1.f, 1.f, 1.f, 1.f}
            };
            return SUCCEEDED(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1)) &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex)));
        }

        bool renderLayer(IDirect3DDevice9* device, float phase, float drift, float evolution)
        {
            if (renderedPhase == phase && renderedDrift == drift && renderedEvolution == evolution) { return true; }
            RenderTargets<> targets(device);
            if (!targets.select(cloudSurface, nullptr, {0, 0, cloudWidth, cloudHeight, 0, 1})) {
                return false;
            }
            DWORD scissor = FALSE;
            device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissor);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            const float settings[]{phase, drift, evolution, 0.f};
            device->SetTexture(5, volume);
            sampleLinear(device, 5, D3DTADDRESS_WRAP);
            const bool drawn = SUCCEEDED(device->SetPixelShaderConstantF(0, settings, 1)) &&
                SUCCEEDED(device->SetPixelShader(clouds)) && quad(device, cloudWidth, cloudHeight, 1.f);
            device->SetTexture(5, nullptr);
            device->SetPixelShader(nullptr);
            const bool restored = targets.restore();
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissor);
            if (drawn && restored) {
                renderedPhase = phase;
                renderedDrift = drift;
                renderedEvolution = evolution;
            }
            return drawn && restored;
        }

    public:
        PrologueSky() = default;
        PrologueSky(const PrologueSky&) = delete;
        PrologueSky& operator=(const PrologueSky&) = delete;
        ~PrologueSky() { release(); }

        void reset()
        {
            release();
            attempted = false;
            renderedPhase = renderedDrift = renderedEvolution = -1.f;
        }

        bool available(IDirect3DDevice9* device, float phase, float drift, float evolution)
        {
            if (!attempted) {
                attempted = true;
                HMODULE library = loadPreviewShaderCompiler();
                if (!library) { return false; }
                const auto compile = reinterpret_cast<CompilePreviewShader>(GetProcAddress(library, "D3DCompile"));
                D3DCAPS9 caps{};
                if (compile && SUCCEEDED(device->GetDeviceCaps(&caps)) && caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)
                    && (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
                    && (caps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)) {
                    static constexpr char source[] = R"hlsl(
float4 options : register(c0);
sampler3D densityMap : register(s5);
sampler2D litClouds : register(s6);
float densityAt(float3 samplePosition)
{
    return dot(tex3Dlod(densityMap, float4(samplePosition, 0)).rg, float2(65280, 255) / 65535.0);
}

// Integrate illuminated density front to back so clear gaps transmit the sky.
float4 cloudPass(float2 uv : TEXCOORD0) : COLOR0
{
    float3 rgb = 0;
    float transmittance = 1;
    float mask = 1 - smoothstep(.62, 1, uv.y);
    float2 view = uv - float2(.5, .20);
    const float nearDrift = options.x;
    const float farDrift = options.y;
    [loop] for (int i = 0; i < 9; ++i) {
        float z = (i + .5) / 9.0;
        const float drift = z < .56 ? nearDrift : farDrift;
        float2 plane = float2(.40 + drift, .17) + view * float2(.90, .52) * (1 + .23 * z);
        float3 samplePosition = float3(plane + float2(.040, -.078) * z, .31 + z * .245 + options.z);
        float density = densityAt(samplePosition);
        const float formation = densityAt(float3(.24 + drift + uv.x * .56, .32 + uv.y * .29, options.z - .17));
        float shape = max(0, density - lerp(.357, .422, formation));
        float mass = saturate(shape * 4.3) * mask * (1 - abs(2 * z - 1)) * .84;
        float above = densityAt(samplePosition + float3(-.014, -.039, .021));
        float sunlight = saturate(.74 + (density - above) * 3.05 - shape * .36);
        float3 shade = lerp(float3(.40, .45, .54), float3(.99, .98, .98), sunlight);
        rgb += shade * mass * transmittance;
        transmittance *= (1 - mass);
    }
    // Stretch the distant layer along the horizon and composite it behind the nearer clouds.
    const float3 distant = float3(.18 + farDrift + uv.x * .58, uv.y * 1.94 + .12, .65 - options.z);
    const float4 bands = float4(densityAt(distant), densityAt(distant + float3(.084, .016, .094)),
        densityAt(distant + float3(-.041, .053, -.016)),
        densityAt(distant + float3(.11, -.028, .08)));
    const float wisps = smoothstep(.44, .71, dot(bands, float4(.37, .25, .23, .15)));
    const float bandAlpha = wisps * .60 * (1 - smoothstep(.58, 1, uv.y));
    const float3 bandLight = lerp(float3(.49, .59, .73), float3(.85, .89, .95), wisps);
    rgb += bandLight * bandAlpha * transmittance;
    transmittance *= 1 - bandAlpha;
    return float4(rgb, 1 - transmittance);
}

float bright(float2 uv)
{
    float3 color = tex2D(litClouds, uv).rgb;
    return max(0, dot(color, float3(.22, .70, .08)) - .45);
}

float4 studioPass(float2 uv : TEXCOORD0, float2 pixel : VPOS) : COLOR0
{
    float2 view = (uv - float2(.52, .11)) * float2(1.72, 2.3);
    float sun = exp(-dot(view, view));
    float3 blueSky = lerp(float3(.34, .47, .64), float3(.59, .72, .86), sun);
    float2 skyUV = float2(uv.x, uv.y / .69);
    float4 clouds = tex2D(litClouds, skyUV);
    // Filter the bright cloud radiance separately from the cloud silhouette.
    float bloom = bright(skyUV + float2(-.026, 0)) + bright(skyUV + float2(.026, 0)) +
        bright(skyUV + float2(0, -.052)) + bright(skyUV + float2(0, .052)) +
        (bright(skyUV + float2(-.021, -.041)) + bright(skyUV + float2(.021, .041)) +
         bright(skyUV + float2(-.021, .041)) + bright(skyUV + float2(.021, -.041))) * .78;
    float3 sky = blueSky * (1 - clouds.a) + clouds.rgb +
        float3(.64, .71, .82) * bloom * .205 * (.38 + .62 * sun);
    sky = min(sky, .83) + .17 * (1 - exp(-max(0, sky - .83) * 5.9));
    float horizon = .555 + options.w * .42;
    float mist = 1 - smoothstep(.23, horizon + .068, uv.y);
    float3 wall = lerp(float3(.107, .123, .148), sky, mist);
    float floorBlend = smoothstep(horizon - .035, horizon + .085, uv.y);
    float falloff = 1 - smoothstep(.70, .97, uv.y);
    float2 floorOffset = (uv - float2(.51, horizon + .108)) * float2(2.0, 5.4);
    float pool = exp(-dot(floorOffset, floorOffset) * 1.3);
    float3 floorColor = float3(.035, .042, .054) + falloff * float3(.09, .105, .126) +
        pool * float3(.29, .317, .350);
    float3 value = lerp(wall, floorColor, floorBlend);
    float2 xy = floor(pixel);
    float2 lo = frac(xy * .5) * 2;
    float2 hi = frac(floor(xy * .5) * .5) * 2;
    float index = ((1 - abs(lo.x - lo.y)) * 8 + lo.y * 4 +
                   (1 - abs(hi.x - hi.y)) * 2 + hi.y);
    return float4(floor(saturate(value) * 255 + (index + .5) / 16) / 255, options.z);
}
)hlsl";
                    clouds = compileShader(device, compile, source, "cloudPass");
                    studio = compileShader(device, compile, source, "studioPass");
                }
                FreeLibrary(library);
                if (!clouds || !studio || !prepareVolume(device) ||
                    FAILED(device->CreateTexture(cloudWidth, cloudHeight, 1, D3DUSAGE_RENDERTARGET,
                                                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cloudLayer, nullptr)) ||
                    FAILED(cloudLayer->GetSurfaceLevel(0, &cloudSurface))) {
                    release();
                }
            }
            return cloudSurface && renderLayer(device, phase, drift, evolution);
        }

        bool draw(IDirect3DDevice9* device, float width, float height, float floorShift, float opacity)
        {
            const float settings[]{0.f, 0.f, opacity, floorShift};
            device->SetTexture(6, cloudLayer);
            sampleLinear(device, 6, D3DTADDRESS_CLAMP);
            const bool drawn = SUCCEEDED(device->SetPixelShaderConstantF(0, settings, 1)) &&
                SUCCEEDED(device->SetPixelShader(studio)) && quad(device, width, height, opacity);
            device->SetTexture(6, nullptr);
            device->SetPixelShader(nullptr);
            return drawn;
        }
    };
}
