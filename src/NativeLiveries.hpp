#pragma once
#include "NativePrize.hpp"
#include "LiveryFocus.hpp"

namespace gt4::liveryNative
{
    inline std::set<int> held;
    inline std::shared_ptr<Mesh> preview;
    inline int capturedModel = -1, capturedLivery = -2, capturedPaint = -1;
    inline int displayedLivery = -1;

    inline void releaseGraphics(Model& s)
    {
        preview.reset();
        for (auto& livery : s.liveries) {
            livery.texture.reset();
            livery.appearanceTexture.reset();
            livery.appearanceMaterials.reset();
            livery.appearanceSource = 0;
        }
        capturedModel = displayedLivery = -1;
        capturedLivery = -2;
        capturedPaint = -1;
        for (int id : held) {
            if (prizeNative::validTxdSlot(id)) {
                reinterpret_cast<void(__cdecl*)(int)>(0x731A70)(id);
            }
        }
        held.clear();
    }

    inline void clear(Model& s)
    {
        releaseGraphics(s);
        s.liveries.clear();
        s.liveriesModel = -1;
        s.livery = 0;
    }

    inline void scan(Model& s, int model)
    {
        if (s.liveriesModel == model) {
            return;
        }
        clear(s);
        void* mi = gta::modelInfo(model);
        if (!game::vehicleInfo(mi)) {
            return;
        }
        s.liveriesModel = model;
        s.liveries.push_back({-1, -1, {}});
        for (int i = 0; i < 4; ++i) {
            int txd = prizeNative::extended16(static_cast<uint8_t*>(mi) + 0x2FA + i * 2);
            if (txd >= 0 && prizeNative::validTxdSlot(txd)) {
                s.liveries.push_back({i, txd, {}});
            }
        }
    }

    inline void queue(int txd)
    {
        if (!prizeNative::validTxdSlot(txd)) {
            return;
        }
        if (held.insert(txd).second) {
            reinterpret_cast<void(__cdecl*)(int)>(0x731A00)(txd);
        }
        reinterpret_cast<void(__cdecl*)(int, int)>(0x407100)(txd, prizeStreamingFlags);
    }

    inline std::shared_ptr<void> thumbnail(int txd)
    {
        if (!prizeNative::txdReady(txd)) {
            return {};
        }
        void* dict = reinterpret_cast<void*(__cdecl*)(int)>(0x408340)(txd);
        if (!dict) {
            return {};
        }
        void* tex = reinterpret_cast<void*(__cdecl*)(void*)>(0x734940)(dict);
        void* raster = nullptr;
        int ext = game::at<int>(0xB4E9E0);
        if (!tex || !gta::read(uintptr_t(tex), raster) || !raster || ext < int(sizeof(void*)) || ext >= 4096) {
            return {};
        }
        IDirect3DTexture9* d3d = nullptr;
        if (!gta::read(uintptr_t(raster) + size_t(ext), d3d) || !d3d) {
            return {};
        }
        d3d->AddRef();
        return {d3d, [](void* p) {
                    static_cast<IDirect3DTexture9*>(p)->Release();
                }};
    }

    // A native capture can identify remap materials on add-ons without a remap name
    // Transfer only textures, and only after confirming the batch layout and UVs match
    inline std::shared_ptr<const std::vector<std::shared_ptr<void>>> matchMaterials(const Mesh& stock, const Mesh& painted)
    {
        if (stock.batches.size() != painted.batches.size()) { return {}; }
        auto materials = std::make_shared<std::vector<std::shared_ptr<void>>>(stock.batches.size());
        bool changed = false;
        for (size_t i = 0; i < stock.batches.size(); ++i) {
            const auto& from = stock.batches[i];
            const auto& to = painted.batches[i];
            if (from.vertices.size() != to.vertices.size() ||
                std::memcmp(from.vertices.data(), to.vertices.data(), from.vertices.size() * sizeof(Vertex)) != 0) {
                return {};
            }
            if (from.texture && to.texture && (from.textureName != to.textureName || from.remap)) {
                to.texture->AddRef();
                (*materials)[i] = {to.texture, [](void* texture) { static_cast<IDirect3DTexture9*>(texture)->Release(); }};
                changed = true;
            }
        }
        return changed ? materials : nullptr;
    }

    inline void update(Model& s, const Entry& e, const Mesh* stock)
    {
        scan(s, e.id);
        if (s.page != Page::Paint && !s.usesLivery()) {
            return;
        }
        for (auto& option : s.liveries) {
            if (option.txd >= 0 && !option.texture) {
                queue(option.txd);
                option.texture = thumbnail(option.txd);
                if (option.texture) {
                    const auto focus = liveryFocus(static_cast<IDirect3DTexture9*>(option.texture.get()));
                    option.focusU = focus.first;
                    option.focusV = focus.second;
                }
            }
        }
        if (!s.usesLivery() || s.livery == 0 || s.livery >= s.liveries.size()) {
            preview.reset();
            capturedLivery = -2;
            return;
        }
        if (!stock) { return; }
        auto& option = s.liveries[s.livery];
        const bool materialRemap = std::any_of(stock->batches.begin(), stock->batches.end(),
            [](const Batch& batch) { return batch.remap && batch.texture; });
        if (option.appearanceSource != stock->serial) {
            option.appearanceMaterials.reset();
            option.appearanceSource = stock->serial;
            capturedLivery = -2;
        }
        if (option.appearanceMaterials) { return; }
        if (materialRemap && option.texture && !option.appearanceTexture) {
            TextureSnapshots copies;
            if (auto* privateTexture = copies.get(game::device(), static_cast<IDirect3DTexture9*>(option.texture.get()))) {
                option.appearanceTexture = {privateTexture, [](void* pointer) {
                    static_cast<IDirect3DTexture9*>(pointer)->Release();
                }};
            }
        }
        if (materialRemap && option.appearanceTexture) {
            return;
        }
        if (!prizeNative::txdReady(option.txd)) {
            preview.reset();
            return;
        }
        if (capturedModel == e.id && capturedLivery == option.index && capturedPaint == int(s.paint)) {
            return;
        }
        if (capturedModel != e.id || capturedPaint != int(s.paint) ||
            displayedLivery != option.index) { preview.reset(); }
        PrizePlan plan;
        plan.id = e.id;
        plan.modkit = false;
        plan.enablePaintjob = true;
        plan.paintjob = option.index;
        plan.paintResolved = plan.kitResolved = true;
        plan.remapTxd = option.txd;
        prizeNative::preparing = &plan;
        const Paint activePaint = s.previewPaint(e);
        prizeNative::preparingPaint = &activePaint;
        prizeNative::prepared = false;
        auto mesh = nativePreview::snapshot(e.id, prizeNative::preparePreview);
        prizeNative::preparing = nullptr;
        prizeNative::preparingPaint = nullptr;
        if (prizeNative::prepared && mesh) {
            option.appearanceMaterials = matchMaterials(*stock, *mesh);
            preview = std::move(mesh);
            capturedModel = e.id;
            capturedLivery = displayedLivery = option.index;
            capturedPaint = int(s.paint);
        }
    }

    inline bool ready(const Model& s)
    {
        if (!s.usesLivery() || s.livery == 0) {
            return true;
        }
        if (s.livery >= s.liveries.size()) {
            return false;
        }
        queue(s.liveries[s.livery].txd);
        return prizeNative::txdReady(s.liveries[s.livery].txd);
    }

    inline void apply(void* vehicle, const Model& s)
    {
        if (!vehicle) {
            return;
        }
        const int index = s.usesLivery() && s.livery < s.liveries.size() ? s.liveries[s.livery].index : -1;
        if (index >= 0) {
            game::preserveRemapColors(vehicle);
        }
        reinterpret_cast<void(__thiscall*)(void*, int)>(0x6D0C00)(vehicle, index);
    }
}
