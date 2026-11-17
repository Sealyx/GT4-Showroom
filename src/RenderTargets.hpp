#pragma once
#include <array>
#include <d3d9.h>

namespace gt4
{
    struct AuxiliaryGraphicsPass
    {
        using Query = int(__cdecl*)();
        std::array<Query, 5> captures{};

        bool active() const
        {
            for (auto query : captures) {
                if (query && query()) {
                    return true;
                }
            }
            return false;
        }
    };

    // a shadow/reflection redraw is not a safe moment to snapshot the visible showroom car.
    inline bool auxiliaryGraphicsPass()
    {
        static HMODULE bound = nullptr;
        static AuxiliaryGraphicsPass queries;
        const HMODULE module = GetModuleHandleW(L"ProperShaders.asi");
        if (bound != module) {
            bound = module;
            queries = {};
            if (module) {
                constexpr const char* names[] = {"PS_IsCapturingGlobalShadows", "PS_IsCapturingLocalShadows",
                                                 "PS_IsCapturingSkylight", "PS_IsCapturingReflections",
                                                 "PS_IsCapturingDepth"};
                for (size_t i = 0; i < queries.captures.size(); ++i) {
                    queries.captures[i] = reinterpret_cast<AuxiliaryGraphicsPass::Query>(
                        GetProcAddress(module, names[i]));
                }
            }
        }
        return queries.active();
    }

    template <class Device = IDirect3DDevice9, class Surface = IDirect3DSurface9> class RenderTargets
    {
        Device* device = nullptr;
        std::array<Surface*, 4> targets{};
        Surface* depth = nullptr;
        D3DVIEWPORT9 viewport{};
        RECT scissor{};
        DWORD count = 0;
        bool captured = false, changed = false;

    public:
        explicit RenderTargets(Device* d)
            : device(d)
        {
            if (!device) {
                return;
            }
            D3DCAPS9 caps{};
            if (FAILED(device->GetDeviceCaps(&caps)) || !caps.NumSimultaneousRTs ||
                caps.NumSimultaneousRTs > targets.size()) {
                return;
            }
            count = caps.NumSimultaneousRTs;
            for (DWORD i = 0; i < count; ++i) {
                const HRESULT hr = device->GetRenderTarget(i, &targets[i]);
                if ((i == 0 && (!targets[i] || FAILED(hr))) || (i > 0 && FAILED(hr) && hr != D3DERR_NOTFOUND)) {
                    return;
                }
            }
            const HRESULT hr = device->GetDepthStencilSurface(&depth);
            if ((FAILED(hr) && hr != D3DERR_NOTFOUND) || FAILED(device->GetViewport(&viewport)) ||
                FAILED(device->GetScissorRect(&scissor))) {
                return;
            }
            captured = true;
        }

        ~RenderTargets()
        {
            restore();
            for (auto* target : targets) {
                if (target) {
                    target->Release();
                }
            }
            if (depth) {
                depth->Release();
            }
        }

        RenderTargets(const RenderTargets&) = delete;
        RenderTargets& operator=(const RenderTargets&) = delete;

        bool valid() const
        {
            return captured;
        }

        const D3DVIEWPORT9& view() const
        {
            return viewport;
        }

        void track()
        {
            if (captured) {
                changed = true;
            }
        }

        bool isolate()
        {
            if (!captured) {
                return false;
            }
            changed = true;
            for (DWORD i = 1; i < count; ++i) {
                if (FAILED(device->SetRenderTarget(i, nullptr))) {
                    return false;
                }
            }
            return true;
        }

        bool select(Surface* target, Surface* stencil, const D3DVIEWPORT9& view)
        {
            if (!target || !isolate()) {
                return false;
            }
            return SUCCEEDED(device->SetDepthStencilSurface(nullptr)) &&
                   SUCCEEDED(device->SetRenderTarget(0, target)) &&
                   SUCCEEDED(device->SetDepthStencilSurface(stencil)) && SUCCEEDED(device->SetViewport(&view));
        }

        bool primary()
        {
            return captured && select(targets[0], depth, viewport);
        }

        bool restore()
        {
            if (!captured) {
                return false;
            }
            if (!changed) {
                return true;
            }
            bool ok = SUCCEEDED(device->SetDepthStencilSurface(nullptr));
            for (DWORD i = 1; i < count; ++i) {
                ok = SUCCEEDED(device->SetRenderTarget(i, nullptr)) && ok;
            }
            ok = SUCCEEDED(device->SetRenderTarget(0, targets[0])) && ok;
            for (DWORD i = 1; i < count; ++i) {
                ok = SUCCEEDED(device->SetRenderTarget(i, targets[i])) && ok;
            }
            ok = SUCCEEDED(device->SetDepthStencilSurface(depth)) && ok;
            ok = SUCCEEDED(device->SetScissorRect(&scissor)) && ok;
            ok = SUCCEEDED(device->SetViewport(&viewport)) && ok;
            if (ok) {
                changed = false;
            }
            return ok;
        }
    };

    inline bool readBackBufferSize(IDirect3DDevice9* device, IDirect3DSwapChain9* swapChain, D3DSURFACE_DESC& output)
    {
        IDirect3DSurface9* buffer = nullptr;
        const HRESULT acquired = swapChain ? swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &buffer)
                                           : device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer);
        if (FAILED(acquired) || !buffer) {
            return false;
        }
        const bool valid = SUCCEEDED(buffer->GetDesc(&output)) && output.Width && output.Height;
        buffer->Release();
        return valid;
    }

    template <class Draw>
    bool drawToBackBuffer(IDirect3DDevice9* device, IDirect3DSwapChain9* swapChain, bool sceneActive, Draw&& draw)
    {
        RenderTargets targets(device);
        if (!targets.valid()) {
            return false;
        }

        IDirect3DSurface9* backBuffer = nullptr;
        const HRESULT acquired = swapChain ? swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)
                                           : device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
        if (FAILED(acquired) || !backBuffer) {
            return false;
        }
        D3DSURFACE_DESC format{};
        bool selected = false;
        if (SUCCEEDED(backBuffer->GetDesc(&format)) && format.Width && format.Height) {
            const D3DVIEWPORT9 viewport{0, 0, format.Width, format.Height, 0, 1};
            selected = targets.select(backBuffer, nullptr, viewport);
        }
        backBuffer->Release();
        if (!selected || (!sceneActive && FAILED(device->BeginScene()))) {
            return false;
        }

        struct Scene
        {
            IDirect3DDevice9* device;

            ~Scene()
            {
                if (device) {
                    device->EndScene();
                }
            }

            bool finish()
            {
                const bool ended = !device || SUCCEEDED(device->EndScene());
                device = nullptr;
                return ended;
            }
        } scene{sceneActive ? nullptr : device};

        const bool drawn = draw(format.Width, format.Height);
        const bool ended = scene.finish();
        return targets.restore() && ended && drawn;
    }
}
