#pragma once
#include <d3d9.h>
#include <algorithm>
#include <cstdint>

namespace gt4
{
    inline unsigned previewSampleCount(int quality, unsigned width, unsigned height)
    {
        if (quality <= 0 || !width || !height)
        {
            return 0;
        }
        unsigned samples = 1u << std::clamp(quality, 1, 3);
        constexpr uint64_t budget = 64ull * 1024 * 1024;
        while (samples > 1 && uint64_t(width) * height * 8 * samples > budget)
        {
            samples >>= 1;
        }
        return samples > 1 ? samples : 0;
    }

    struct PreviewAntialiasing
    {
        IDirect3DSurface9 *surface = nullptr;
        unsigned samples = 0;
        int rejectedQuality = -1;

        ~PreviewAntialiasing()
        {
            reset();
        }

        void reset()
        {
            if (surface)
            {
                surface->Release();
                surface = nullptr;
            }
            samples = 0;
        }

        // budget and test both formats before allocating; unsupported drivers keep the original target path
        bool create(IDirect3DDevice9 *device, unsigned width, unsigned height, int quality, IDirect3DSurface9 *&depth)
        {
            reset();
            const unsigned desired = quality == rejectedQuality ? 0 : previewSampleCount(quality, width, height);
            D3DCAPS9 caps{};
            if (!desired || FAILED(device->GetDeviceCaps(&caps)) || !(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
            {
                return false;
            }
            IDirect3D9 *api = nullptr;
            IDirect3DSwapChain9 *chain = nullptr;
            D3DPRESENT_PARAMETERS parameters{};
            if (FAILED(device->GetDirect3D(&api)) || !api)
            {
                if (api)
                {
                    api->Release();
                }
                return false;
            }
            const bool queried = SUCCEEDED(device->GetSwapChain(0, &chain)) && chain &&
                                 SUCCEEDED(chain->GetPresentParameters(&parameters));
            if (chain)
            {
                chain->Release();
            }
            if (!queried)
            {
                api->Release();
                return false;
            }
            for (unsigned count = desired; count >= 2; count >>= 1)
            {
                const auto type = D3DMULTISAMPLE_TYPE(count);
                if (FAILED(api->CheckDeviceMultiSampleType(caps.AdapterOrdinal, caps.DeviceType, D3DFMT_A8R8G8B8,
                                                           parameters.Windowed, type, nullptr)) ||
                    FAILED(api->CheckDeviceMultiSampleType(caps.AdapterOrdinal, caps.DeviceType, D3DFMT_D24S8,
                                                           parameters.Windowed, type, nullptr)))
                {
                    continue;
                }
                IDirect3DSurface9 *stencil = nullptr;
                if (SUCCEEDED(device->CreateRenderTarget(width, height, D3DFMT_A8R8G8B8, type, 0, FALSE, &surface,
                                                         nullptr)) &&
                    surface &&
                    SUCCEEDED(device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, type, 0, TRUE, &stencil,
                                                                nullptr)) &&
                    stencil)
                {
                    depth = stencil;
                    samples = count;
                    api->Release();
                    return true;
                }
                if (stencil)
                {
                    stencil->Release();
                }
                reset();
            }
            api->Release();
            return false;
        }

        bool resolve(IDirect3DDevice9 *device, IDirect3DSurface9 *output, int quality)
        {
            if (!surface)
            {
                return true;
            }
            if (SUCCEEDED(device->StretchRect(surface, nullptr, output, nullptr, D3DTEXF_NONE)))
            {
                return true;
            }
            rejectedQuality = quality;
            return false;
        }
    };
} // namespace gt4
