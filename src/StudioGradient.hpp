#pragma once

#include "PreviewOcclusion.hpp"
#include "StudioLighting.hpp"

namespace gt4
{
    class StudioGradient
    {
        IDirect3DPixelShader9* shader = nullptr;
        bool attempted = false, lookupAttempted = false;
        static constexpr UINT mapWidth = 256, mapHeight = 256;
        std::array<IDirect3DTexture9*, 4> maps{};

        bool prepareMaps(IDirect3DDevice9* device)
        {
            if (lookupAttempted) { return maps.back() != nullptr; }
            lookupAttempted = true;
            for (unsigned setup = 0; setup < maps.size(); ++setup) {
                IDirect3DTexture9* texture = nullptr;
                if (FAILED(device->CreateTexture(mapWidth, mapHeight, 1, 0, D3DFMT_A8R8G8B8,
                                                 D3DPOOL_MANAGED, &texture, nullptr))) {
                    return false;
                }
                D3DLOCKED_RECT lock{};
                if (FAILED(texture->LockRect(0, &lock, nullptr, 0))) {
                    texture->Release();
                    return false;
                }
                studioLighting::LightFrame light{};
                light.from = light.to = setup;
                for (UINT y = 0; y < mapHeight; ++y) {
                    auto* pixels = reinterpret_cast<DWORD*>(static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch);
                    for (UINT x = 0; x < mapWidth; ++x) {
                        const float value = studioLighting::backdrop(0, float(x) / (mapWidth - 1),
                                                                     float(y) / (mapHeight - 1), light)[1];
                        const auto packed = DWORD(std::lround(value * 257.f));
                        pixels[x] = 0xFF000000 | ((packed >> 8) << 16) | ((packed & 255) << 8);
                    }
                }
                if (FAILED(texture->UnlockRect(0))) {
                    texture->Release();
                    return false;
                }
                maps[setup] = texture;
            }
            return true;
        }

    public:
        StudioGradient() = default;
        StudioGradient(const StudioGradient&) = delete;
        StudioGradient& operator=(const StudioGradient&) = delete;
        ~StudioGradient() { reset(); }

        void reset()
        {
            if (shader) { shader->Release(); }
            shader = nullptr;
            attempted = lookupAttempted = false;
            for (auto*& map : maps) {
                if (map) { map->Release(); }
                map = nullptr;
            }
        }

        bool bind(IDirect3DDevice9* device, bool lookup = false, float mix = 0.f, float opacity = 1.f)
        {
            if (!attempted) {
                attempted = true;
                D3DCAPS9 caps{};
                if (FAILED(device->GetDeviceCaps(&caps)) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
                    return false;
                }
                HMODULE library = loadPreviewShaderCompiler();
                if (!library) { return false; }
                const auto compile = reinterpret_cast<CompilePreviewShader>(GetProcAddress(library, "D3DCompile"));
                if (compile) {
                    // Keep sub-byte vertex colors until the final 8-bit write.
                    // A fixed sub-LSB pattern prevents large flat bands without temporal shimmer.
                    static constexpr char source[] = R"hlsl(
sampler2D fromMap : register(s6);
sampler2D toMap : register(s7);
float4 settings : register(c0);
float4 main(float3 rgb : TEXCOORD0, float2 pixel : VPOS) : COLOR0
{
    if (settings.x > 0.5) {
        const float2 weights = float2(65280.0, 255.0) / 65535.0;
        float fromValue = dot(tex2D(fromMap, rgb.xy).rg, weights);
        float toValue = dot(tex2D(toMap, rgb.xy).rg, weights);
        float value = lerp(fromValue, toValue, settings.y);
        float cool = min(2.5 / 255.0, value * .027);
        rgb = float3(value - cool, value, value + cool * .6);
    }
    float2 xy = floor(pixel);
    float2 lo = frac(xy * .5) * 2;
    float2 hi = frac(floor(xy * .5) * .5) * 2;
    float index = ((1 - abs(lo.x - lo.y)) * 8 + lo.y * 4 +
                   (1 - abs(hi.x - hi.y)) * 2 + hi.y);
    float3 quantized = floor(saturate(rgb) * 255.0 + (index + .5) / 16.0);
    return float4(quantized / 255.0, settings.z);
}
)hlsl";
                    ID3DBlob* bytecode = nullptr;
                    if (SUCCEEDED(compile(source, sizeof(source) - 1, "StudioGradient", nullptr, nullptr, "main",
                                          "ps_3_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &bytecode, nullptr)) &&
                        bytecode) {
                        device->CreatePixelShader(static_cast<const DWORD*>(bytecode->GetBufferPointer()), &shader);
                    }
                    if (bytecode) { bytecode->Release(); }
                }
                FreeLibrary(library);
            }
            const float settings[]{lookup ? 1.f : 0.f, mix, opacity, 0.f};
            return shader && SUCCEEDED(device->SetPixelShaderConstantF(0, settings, 1)) &&
                   SUCCEEDED(device->SetPixelShader(shader));
        }

        bool drawLookup(IDirect3DDevice9* device, float width, float height, float floorShift,
                        const studioLighting::LightFrame& light, float opacity)
        {
            if (!bind(device, true, light.mix, opacity)) { return false; }
            if (!prepareMaps(device)) {
                device->SetPixelShader(nullptr);
                return false;
            }

            // Filter the packed high and low bytes together so the reconstructed luminance stays linear.
            device->SetTexture(6, maps[light.from]);
            device->SetTexture(7, maps[light.to]);
            for (DWORD slot : {6u, 7u}) {
                device->SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                device->SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                device->SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
                device->SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                device->SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                device->SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, FALSE);
            }
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, opacity < 1.f);
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

            struct Vertex { float x, y, z, rhw, u, v, unused; };
            constexpr float u0 = .5f / mapWidth, u1 = 1.f - u0;
            constexpr float step = 1.f - 1.f / mapHeight;
            const float v0 = .5f / mapHeight - floorShift * step, v1 = v0 + step;
            const Vertex vertices[]{
                {-.5f, -.5f, 1.f, 1.f, u0, v0, 0.f}, {width - .5f, -.5f, 1.f, 1.f, u1, v0, 0.f},
                {-.5f, height - .5f, 1.f, 1.f, u0, v1, 0.f},
                {width - .5f, height - .5f, 1.f, 1.f, u1, v1, 0.f}
            };
            const bool drawn = SUCCEEDED(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0))) &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex)));
            device->SetTexture(6, nullptr);
            device->SetTexture(7, nullptr);
            device->SetPixelShader(nullptr);
            return drawn;
        }
    };
}
