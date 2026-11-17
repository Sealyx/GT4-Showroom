#pragma once
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <cstring>
#include "RenderTargets.hpp"

namespace gt4
{
    inline constexpr char previewOcclusionShader[] = R"hlsl(
row_major float4x4 worldViewProjection : register(c0);
row_major float4x4 worldView : register(c4);
float4 material : register(c8);
sampler2D image : register(s0);
sampler2D geometry : register(s1);
float4 projectionInfo : register(c0);
float4 sampleInfo : register(c1);
float4 pixelInfo : register(c2);
struct Input { float3 position : POSITION0; float3 normal : NORMAL0; float2 uv : TEXCOORD0; float cavity : TEXCOORD1; };
struct Output { float4 position : POSITION0; float3 normal : TEXCOORD0; float depth : TEXCOORD1; float2 uv : TEXCOORD2; float cavity : TEXCOORD3; };
Output geometryVertex(Input input)
{
    Output output;
    output.position = mul(float4(input.position, 1), worldViewProjection);
    output.normal = mul(float4(input.normal, 0), worldView).xyz;
    output.depth = mul(float4(input.position, 1), worldView).z;
    output.uv = input.uv;
    output.cavity = input.cavity;
    return output;
}
float4 geometryPixel(Output input) : COLOR0
{
    if (material.x > 0) { clip(tex2D(image, input.uv).a - 0.98); }
    float cavity = material.y * saturate(input.cavity);
    return float4(normalize(input.normal) * (1 - cavity) * 0.5 + 0.5, input.depth);
}
float3 positionAt(float2 uv, float depth)
{
    return float3((uv.x * 2 - 1 - projectionInfo.z) / projectionInfo.x,
                  (1 - uv.y * 2 - projectionInfo.w) / projectionInfo.y, 1) * depth;
}
float4 occlusionPixel(float2 uv : TEXCOORD0) : COLOR0
{
    float4 center = tex2D(image, uv);
    if (center.a <= 0) { return 1; }
    float3 normal = normalize(center.rgb * 2 - 1);
    float3 position = positionAt(uv, center.a);
    float radius = sampleInfo.x;
    float sum = 0;
    float valid = 0;
    [loop] for (int i = 0; i < 32; ++i) {
        if (i >= sampleInfo.y) { break; }
        float angle = i * 2.39996323;
        float fraction = sqrt((i + 0.5) / sampleInfo.y);
        float2 offset = float2(cos(angle), sin(angle)) * fraction;
        offset *= radius * abs(projectionInfo.xy) / max(center.a, 0.1) * 0.5;
        float2 probe = uv + offset;
        float4 neighbor = tex2Dlod(image, float4(probe, 0, 0));
        float3 delta = positionAt(probe, neighbor.a) - position;
        float distance = length(delta);
        float horizon = max(0, dot(normal, delta / max(distance, 0.001)) - 0.08);
        float weight = saturate(1 - distance / radius);
        float inside = step(0, probe.x) * step(probe.x, 1) * step(0, probe.y) * step(probe.y, 1);
        sum += horizon * weight * step(0.001, neighbor.a) * inside;
        valid += inside;
    }
    float screenAO = min(0.35, sum / max(valid, 1) * sampleInfo.z);
    float cavityAO = min(0.35, max(0, 1 - length(center.rgb * 2 - 1)));
    return 1 - max(screenAO, cavityAO) * sampleInfo.w;
}
float4 compositePixel(float2 uv : TEXCOORD0) : COLOR0
{
    float4 center = tex2D(geometry, uv);
    if (center.a <= 0) { return 1; }
    float value = tex2D(image, uv).r;
    float total = 1;
    [unroll] for (int i = 0; i < 4; ++i) {
        float2 direction = i == 0 ? float2(1, 0) : i == 1 ? float2(-1, 0) :
                           i == 2 ? float2(0, 1) : float2(0, -1);
        float2 probe = uv + direction * pixelInfo.xy;
        float4 neighbor = tex2D(geometry, probe);
        float weight = exp(-abs(center.a - neighbor.a) / max(sampleInfo.x * 0.1, 0.001));
        weight *= saturate(dot(normalize(center.rgb * 2 - 1), normalize(neighbor.rgb * 2 - 1)));
        value += tex2D(image, probe).r * weight;
        total += weight;
    }
    return float4((value / total).xxx, 1);
}
)hlsl";

    using CompilePreviewShader = HRESULT(WINAPI *)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *,
                                                   LPCSTR, LPCSTR, UINT, UINT, ID3DBlob **, ID3DBlob **);

    // only the system compiler is eligible; a missing optional compiler must not break the showroom
    inline HMODULE loadPreviewShaderCompiler()
    {
        wchar_t directory[MAX_PATH]{};
        const UINT length = GetSystemDirectoryW(directory, MAX_PATH);
        if (!length || length >= MAX_PATH - 24)
        {
            return nullptr;
        }
        std::wcscat(directory, L"\\d3dcompiler_47.dll");
        return LoadLibraryW(directory);
    }

    inline HRESULT compilePreviewShader(CompilePreviewShader compile, const char *entry, const char *profile,
                                        ID3DBlob **result, ID3DBlob **errors = nullptr)
    {
        return compile(previewOcclusionShader, sizeof(previewOcclusionShader) - 1, "preview-occlusion", nullptr,
                       nullptr, entry, profile,
                       D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, result, errors);
    }

    struct PreviewOcclusion
    {
        IDirect3DTexture9 *normals = nullptr;
        IDirect3DTexture9 *occlusion = nullptr;
        IDirect3DSurface9 *normalSurface = nullptr;
        IDirect3DSurface9 *occlusionSurface = nullptr;
        IDirect3DSurface9 *depth = nullptr;
        IDirect3DVertexShader9 *vertex = nullptr;
        IDirect3DPixelShader9 *geometry = nullptr;
        IDirect3DPixelShader9 *shading = nullptr;
        IDirect3DPixelShader9 *composite = nullptr;
        unsigned width = 0, height = 0;
        bool unavailable = false;

        PreviewOcclusion() = default;
        PreviewOcclusion(const PreviewOcclusion &) = delete;
        PreviewOcclusion &operator=(const PreviewOcclusion &) = delete;

        ~PreviewOcclusion()
        {
            reset();
        }

        template <class T> static void release(T *&resource)
        {
            if (resource)
            {
                resource->Release();
                resource = nullptr;
            }
        }

        void releaseTargets()
        {
            release(normalSurface);
            release(occlusionSurface);
            release(normals);
            release(occlusion);
            release(depth);
            width = height = 0;
        }

        void reset()
        {
            releaseTargets();
            release(vertex);
            release(geometry);
            release(shading);
            release(composite);
            unavailable = false;
        }

        bool prepare(IDirect3DDevice9 *device, unsigned w, unsigned h)
        {
            if (unavailable)
            {
                return false;
            }
            if (!vertex)
            {
                D3DCAPS9 caps{};
                if (FAILED(device->GetDeviceCaps(&caps)) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0) ||
                    caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
                {
                    unavailable = true;
                    return false;
                }
                HMODULE library = loadPreviewShaderCompiler();
                auto compile =
                    library ? reinterpret_cast<CompilePreviewShader>(GetProcAddress(library, "D3DCompile")) : nullptr;
                bool okay = compile != nullptr;
                constexpr const char *entries[]{"geometryVertex", "geometryPixel", "occlusionPixel", "compositePixel"};
                for (unsigned i = 0; i < 4 && okay; ++i)
                {
                    ID3DBlob *code = nullptr;
                    okay = SUCCEEDED(compilePreviewShader(compile, entries[i], i ? "ps_3_0" : "vs_3_0", &code)) && code;
                    if (okay)
                    {
                        const auto *bytes = static_cast<const DWORD *>(code->GetBufferPointer());
                        okay = SUCCEEDED(i == 0 ? device->CreateVertexShader(bytes, &vertex)
                                                : device->CreatePixelShader(bytes, i == 1   ? &geometry
                                                                                   : i == 2 ? &shading
                                                                                            : &composite));
                    }
                    release(code);
                }
                if (library)
                {
                    FreeLibrary(library);
                }
                if (!okay)
                {
                    reset();
                    unavailable = true;
                    return false;
                }
            }
            if (width == w && height == h && normals && occlusion && depth)
            {
                return true;
            }
            releaseTargets();
            const bool okay = SUCCEEDED(device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F,
                                                              D3DPOOL_DEFAULT, &normals, nullptr)) &&
                              normals && SUCCEEDED(normals->GetSurfaceLevel(0, &normalSurface)) && normalSurface &&
                              SUCCEEDED(device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                                                              D3DPOOL_DEFAULT, &occlusion, nullptr)) &&
                              occlusion && SUCCEEDED(occlusion->GetSurfaceLevel(0, &occlusionSurface)) &&
                              occlusionSurface &&
                              SUCCEEDED(device->CreateDepthStencilSurface(w, h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0,
                                                                          TRUE, &depth, nullptr)) &&
                              depth;
            if (!okay)
            {
                releaseTargets();
                unavailable = true;
                return false;
            }
            width = w;
            height = h;
            return true;
        }

        static D3DMATRIX multiply(const D3DMATRIX &a, const D3DMATRIX &b)
        {
            D3DMATRIX result{};
            for (unsigned row = 0; row < 4; ++row)
            {
                for (unsigned column = 0; column < 4; ++column)
                {
                    for (unsigned k = 0; k < 4; ++k)
                    {
                        result.m[row][column] += a.m[row][k] * b.m[k][column];
                    }
                }
            }
            return result;
        }

        bool bindGeometry(IDirect3DDevice9 *device, bool textured, bool cavity = false)
        {
            D3DMATRIX world{}, view{}, projection{};
            if (FAILED(device->GetTransform(D3DTS_WORLD, &world)) || FAILED(device->GetTransform(D3DTS_VIEW, &view)) ||
                FAILED(device->GetTransform(D3DTS_PROJECTION, &projection)))
            {
                return false;
            }
            const auto worldView = multiply(world, view);
            const auto matrix = multiply(worldView, projection);
            const float material[]{textured ? 1.f : 0.f, cavity ? 1.f : 0.f, 0, 0};
            return SUCCEEDED(device->SetVertexShader(vertex)) && SUCCEEDED(device->SetPixelShader(geometry)) &&
                   SUCCEEDED(device->SetVertexShaderConstantF(0, &matrix.m[0][0], 4)) &&
                   SUCCEEDED(device->SetVertexShaderConstantF(4, &worldView.m[0][0], 4)) &&
                   SUCCEEDED(device->SetPixelShaderConstantF(8, material, 1));
        }

        static void quad(IDirect3DDevice9 *device, const D3DVIEWPORT9 &viewport)
        {
            struct Pixel
            {
                float x, y, z, rhw, u, v;
            };
            const float x = float(viewport.X) - .5f, y = float(viewport.Y) - .5f;
            const float right = x + viewport.Width, bottom = y + viewport.Height;
            const Pixel vertices[]{
                {x, y, 0, 1, 0, 0}, {right, y, 0, 1, 1, 0}, {x, bottom, 0, 1, 0, 1}, {right, bottom, 0, 1, 1, 1}};
            device->SetVertexShader(nullptr);
            device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Pixel));
        }

        // geometry, depth and compositing stay private; no game or graphics-plugin buffers are sampled
        template <class Draw>
        bool apply(IDirect3DDevice9 *device, int quality, float radius, float opacity, Draw &&draw)
        {
            RenderTargets targets(device);
            if (quality < 1 || !targets.valid() || targets.view().Height < 180)
            {
                return false;
            }
            const auto original = targets.view();
            unsigned divisor = 1;
            if (quality == 1)
            {
                divisor = 4;
            }
            else if (quality == 2 || uint64_t(original.Width) * original.Height * 16 > 48ull * 1024 * 1024)
            {
                divisor = 2;
            }
            const unsigned w = std::max(1u, unsigned(original.Width) / divisor);
            const unsigned h = std::max(1u, unsigned(original.Height) / divisor);
            if (uint64_t(w) * h * 16 > 48ull * 1024 * 1024 || !prepare(device, w, h))
            {
                return false;
            }
            IDirect3DStateBlock9 *state = nullptr;
            if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)) || !state)
            {
                release(state);
                return false;
            }
            D3DMATRIX projection{};
            device->GetTransform(D3DTS_PROJECTION, &projection);
            const D3DVIEWPORT9 viewport{0, 0, w, h, 0, 1};
            bool okay = targets.select(normalSurface, depth, viewport);
            if (okay)
            {
                device->SetTexture(1, nullptr);
                device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
                device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
                device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
                device->SetRenderState(D3DRS_ZENABLE, TRUE);
                device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
                device->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
                device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
                okay = SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0)) && draw();
            }
            if (okay)
            {
                okay = targets.select(occlusionSurface, nullptr, viewport);
                device->SetRenderState(D3DRS_ZENABLE, FALSE);
                device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
                device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
                for (unsigned sampler = 0; sampler < 2; ++sampler)
                {
                    device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                    device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                    device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_POINT);
                    device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                    device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
                    device->SetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, FALSE);
                }
                const float projectionValues[]{projection._11, projection._22, projection._31, projection._32};
                float sampleCount = 32.f;
                float strength = 2.f;
                if (quality == 1)
                {
                    sampleCount = 8.f;
                    strength = 1.f;
                }
                else if (quality == 2)
                {
                    sampleCount = 16.f;
                    strength = 1.5f;
                }
                const float samples[]{radius, sampleCount, strength, std::clamp(opacity, 0.f, 1.f)};
                const float pixels[]{1.f / w, 1.f / h, 0, 0};
                device->SetPixelShaderConstantF(0, projectionValues, 1);
                device->SetPixelShaderConstantF(1, samples, 1);
                device->SetPixelShaderConstantF(2, pixels, 1);
                device->SetTexture(0, normals);
                okay = okay && SUCCEEDED(device->SetPixelShader(shading));
                if (okay)
                {
                    quad(device, viewport);
                    device->SetTexture(0, nullptr);
                    okay = targets.primary();
                }
                if (okay)
                {
                    device->SetTexture(0, occlusion);
                    device->SetTexture(1, normals);
                    device->SetPixelShader(composite);
                    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
                    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
                    device->SetRenderState(D3DRS_COLORWRITEENABLE, 7);
                    quad(device, original);
                }
            }
            device->SetTexture(0, nullptr);
            device->SetTexture(1, nullptr);
            targets.restore();
            state->Apply();
            state->Release();
            return okay;
        }
    };
} // namespace gt4
