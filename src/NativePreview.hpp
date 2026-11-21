#pragma once
#include "Game.hpp"
#include <unordered_set>
#include "VehFuncsBinding.hpp"
#include "WheelPose.hpp"
#include "PreviewMaterialCycle.hpp"
#include "SpecialComponentPose.hpp"
#include "CinematicNative.hpp"

namespace gt4::nativePreview
{
    inline void* target = nullptr;
    inline bool failedPath = false;
    inline VehFuncsFrames visibility;
    inline uint64_t prepared = 0;
    inline int lastModel = -1, lastSources = 0, lastRemaining = 0, lastWheels = 0;
    inline bool wheelMatrices = false;

    inline int sourceGroups(void* frame, int depth = 0)
    {
        if (!frame || depth > 80) {
            return 0;
        }
        int count = 0, budget = 0;
        while (frame && budget++ < 4096) {
            const char* name = reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(frame);
            if (name && std::strncmp(name, "f_wheel", 7) == 0) {
                ++count;
            }
            count += sourceGroups(game::field<void*>(frame, 0x98), depth + 1);
            frame = game::field<void*>(frame, 0x9C);
        }
        return count;
    }

    inline bool frameVisible(void* frame)
    {
        int depth = 0;
        for (void* f = frame; f && depth < 96; f = game::field<void*>(f, 4), ++depth) {
            if (!visibility.visible(f)) {
                return false;
            }
            const char* name = reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(f);
            if (!previewHighDetail(name)) {
                return false;
            }
        }
        return depth < 96;
    }

    inline void restWheelMatrices(void* vehicle)
    {
        void* mi = gta::modelInfo(game::field<int16_t>(vehicle, 0x22));
        if (!mi) {
            return;
        }
        const int kind = game::field<int>(mi, 0x3C);
        wheelMatrices = prepareWheelPose(
            vehicle, kind, game::field<int16_t>(vehicle, 0x22),
            [&](int node) {
                return game::field<void*>(vehicle, 0x648 + size_t(node) * 4) != nullptr;
            },
            reinterpret_cast<WheelMatrixUpdate>(0x6AA290));
    }

    inline std::unique_ptr<Mesh> result;
    inline void (*observeVehicle)(void*) = nullptr;
    inline bool copyGeometry = true, reached = false;
    inline std::string captureFailure;
    inline void (*diagnostic)(const char*) = nullptr;
    inline std::unordered_set<IDirect3DTexture9*> inspectedDirtTextures;

    inline void inspectDirtTexture(const char* name, void* native, IDirect3DTexture9* texture, bool replacement)
    {
        if (!diagnostic || PreviewDirtScope::level != 0 ||
            (!std::strstr(name, "grunge") && !std::strstr(name, "dirt")) ||
            !inspectedDirtTextures.insert(texture).second) {
            return;
        }
        std::array<void*, 16> dirtTextures{};
        gta::read(0xC02BD0, dirtTextures);
        const auto found = std::find(dirtTextures.begin(), dirtTextures.end(), native);
        const int slot = found == dirtTextures.end() ? -1 : int(found - dirtTextures.begin());
        for (unsigned level = 0; level < texture->GetLevelCount(); ++level) {
            D3DSURFACE_DESC desc{};
            D3DLOCKED_RECT locked{};
            if (FAILED(texture->GetLevelDesc(level, &desc))) {
                continue;
            }
            const auto rows = textureRows(desc.Format, desc.Width, desc.Height);
            uint32_t hash = 2166136261u;
            unsigned minimum = 255, maximum = 0;
            const bool readable = rows.bytes && rows.count &&
                SUCCEEDED(texture->LockRect(level, &locked, nullptr, D3DLOCK_READONLY));
            if (readable) {
                if (locked.pBits && locked.Pitch >= int(rows.bytes)) {
                    for (size_t row = 0; row < rows.count; ++row) {
                        const auto* bytes = static_cast<const uint8_t*>(locked.pBits) + row * size_t(locked.Pitch);
                        for (size_t x = 0; x < rows.bytes; ++x) {
                            hash = (hash ^ bytes[x]) * 16777619u;
                            if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8) {
                                if (x % 4 != 3) {
                                    minimum = std::min(minimum, unsigned(bytes[x]));
                                    maximum = std::max(maximum, unsigned(bytes[x]));
                                }
                            }
                        }
                    }
                }
                texture->UnlockRect(level);
            }
            char detail[256]{};
            std::snprintf(detail, sizeof(detail),
                          "Dirt texture: name=%s slot=%d cleanReplacement=%d level=%u size=%ux%u format=%u "
                          "readable=%d rgbRange=%u-%u hash=%08X",
                          name, slot, replacement, level, desc.Width, desc.Height, unsigned(desc.Format),
                          readable, minimum, maximum, hash);
            diagnostic(detail);
        }
    }
    inline std::unordered_map<void*, int> markerSlots;

    struct MaterialPin
    {
        void* material = nullptr;
        uint32_t color = 0;
        void* texture = nullptr;
        std::array<uint8_t, 12> surface{};
    };

    inline std::vector<MaterialPin> retired;

    // materials might still be on a to-do list. might retire them idk
    inline void releaseRetired()
    {
        for (auto& p : retired) {
            reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);
            if (p.texture) {
                reinterpret_cast<bool(__cdecl*)(void*)>(0x7F3820)(p.texture);
            }
        }
        retired.clear();
    }

    struct Pins
    {
        std::vector<MaterialPin> values;
        std::unordered_set<void*> seen;
        bool valid = true;
    };

    inline void* __cdecl pinAtomic(void* atomic, void* data)
    {
        auto& pins = *static_cast<Pins*>(data);
        void* geometry = game::field<void*>(atomic, 0x18);
        if (!geometry) {
            return atomic;
        }
        const int n = game::field<int>(geometry, 0x24);
        auto** materials = game::field<void**>(geometry, 0x20);
        if (n < 0 || n > 1024 || (!materials && n)) {
            pins.valid = false;
            return atomic;
        }
        for (int i = 0; i < n; ++i) {
            void* m = materials[i];
            if (!m || !pins.seen.insert(m).second) {
                continue;
            }
            if (pins.values.size() >= 4096) {
                pins.valid = false;
                break;
            }
            auto& refs = game::field<int16_t>(m, 0x18);
            if (refs < 1 || refs >= 32766) {
                pins.valid = false;
                break;
            }
            MaterialPin p;
            p.material = m;
            p.color = game::field<uint32_t>(m, 4);
            p.texture = game::field<void*>(m, 0);
            if (p.texture) {
                auto& tr = game::field<int32_t>(p.texture, 0x54);
                if (tr < 1 || tr >= 0x3FFFFFFF) {
                    pins.valid = false;
                    break;
                }
                ++tr;
            }
            const uint32_t rgb = ((p.color & 255) << 16) | (p.color & 0xFF00) | ((p.color >> 16) & 255);
            constexpr uint32_t markers[] = {0x3CFF00, 0xFF00AF, 0x00FFFF, 0xFF00FF};
            for (int k = 0; k < 4; ++k) {
                if (rgb == markers[k]) {
                    markerSlots[m] = k;
                }
            }
            std::memcpy(p.surface.data(), static_cast<uint8_t*>(m) + 0xC, 12);
            ++refs;
            pins.values.push_back(p);
        }
        return atomic;
    }

    inline void restore(const std::vector<MaterialPin>& pins)
    {
        for (const auto& p : pins) {
            game::field<uint32_t>(p.material, 4) = p.color;
            std::memcpy(static_cast<uint8_t*>(p.material) + 0xC, p.surface.data(), 12);
        }
    }

    inline bool onEntityRender(void* entity)
    {
        if (!entity || (!target && retired.empty())) {
            return false;
        }
        if (entity == target) {
            releaseRetired();
            reached = true;
            if (observeVehicle) {
                observeVehicle(entity);
            }
            if (!copyGeometry) {
                return true;
            }
            result = std::make_unique<Mesh>();
            result->capturedDirt = game::field<float>(entity, 0x4B0);
            void* clump = game::field<void*>(entity, 0x18);
            if (clump) {
                restWheelMatrices(entity);
                prepareCementPose(
                    entity, game::field<int16_t>(entity, 0x22),
                    [&](int node) {
                        return game::field<void*>(entity, 0x648 + size_t(node) * 4);
                    },
                    reinterpret_cast<void(__thiscall*)(void*, void*, int, float, bool)>(0x6DBA30));
                void* root = game::field<void*>(clump, 4);
                if (root) {
                    reinterpret_cast<void*(__cdecl*)(void*)>(0x7F0910)(root);
                }
                lastRemaining = sourceGroups(root);
                void* mi = gta::modelInfo(game::field<int16_t>(entity, 0x22));
                const int kind = mi ? game::field<int>(mi, 0x3C) : -1;
                auto frames = cinematicNative::capture(entity, kind, result->cinematic);
                if (kind == 9 || kind == 10) {
                    frames = cinematicNative::captureBikeSteering(entity, kind, result->bikeSteering);
                }
                game::Extract extract{result.get(), 0, frameVisible, &markerSlots};
                inspectedDirtTextures.clear();
                extract.observeTexture = diagnostic ? inspectDirtTexture : nullptr;
                extract.componentFrames = &frames;
                extract.component = cinematicNative::component;
                extract.plateTexture = game::field<void*>(entity, 0x588);
                if (mi) {
                    extract.plateMaterial = game::field<void*>(mi, 0x24);
                }
                reinterpret_cast<void*(__cdecl*)(void*, void*(__cdecl*)(void*, void*), void*)>(0x749B70)(
                    clump, game::extractAtomic, &extract);
                ++prepared;
            }
            return true;
        }
        if (!target && (game::field<uint8_t>(entity, 0x36) & 7) == 2) {
            releaseRetired();
        }
        return false;
    }

    // mihgt need it to render first. if it needs an entity hook, grab the same prepared car here
    template <class Render> void renderForCapture(void* vehicle, Render render)
    {
        render(vehicle);
        if (!reached && target == vehicle) {
            onEntityRender(vehicle);
        }
    }

    template <class Render> void renderMaterialFrames(void* vehicle, Render render, int passes)
    {
        struct RestoreCopy
        {
            bool saved = copyGeometry;
            ~RestoreCopy()
            {
                copyGeometry = saved;
            }
        } restore;
        for (int pass = 0; pass < passes; ++pass) {
            copyGeometry = restore.saved && pass == passes - 1;
            reached = false;
            renderForCapture(vehicle, render);
        }
    }

    inline bool enabledFor(int id)
    {
        void* mi = gta::modelInfo(id);
        if (!game::vehicleInfo(mi)) {
            return false;
        }
        const int kind = game::field<int>(mi, 0x3C);
        return kind >= 0 && kind <= 11;
    }

    template <class Device, class State> bool beginCaptureState(Device* device, State*& saved)
    {
        saved = nullptr;
        if (!device) {
            return false;
        }
        const HRESULT hr = device->CreateStateBlock(D3DSBT_ALL, &saved);
        if (FAILED(hr) || !saved) {
            if (saved) {
                saved->Release();
            }
            saved = nullptr;
            return false;
        }
        return true;
    }

    // modelextras sets its current vehicle at the scene callsite, not inside gta's setup function
    inline void setupMaterials(void* vehicle)
    {
        const HMODULE extras = GetModuleHandleW(L"ModelExtras.asi");
        std::array<uint8_t, 5> call{};
        if (extras && gta::read(0x5532A9, call) && call[0] == 0xE8) {
            int32_t displacement = 0;
            std::memcpy(&displacement, call.data() + 1, sizeof(displacement));
            const uintptr_t address = uintptr_t(0x5532AE) + uintptr_t(intptr_t(displacement));
            HMODULE owner = nullptr;
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  reinterpret_cast<LPCWSTR>(address), &owner) && owner == extras) {
                reinterpret_cast<void(__fastcall*)(void*)>(address)(vehicle);
                if (diagnostic && PreviewDirtScope::level == 0) {
                    diagnostic("Clean material setup: ModelExtras-owned scene callback");
                }
                return;
            }
        }
        reinterpret_cast<void(__thiscall*)(void*)>(0x6D64F0)(vehicle);
        if (diagnostic && PreviewDirtScope::level == 0) {
            diagnostic("Clean material setup: native fallback; ModelExtras scene callback was not selected");
        }
    }

    inline std::unordered_map<int, float> previewDirt;

    // rebuilding a spoiler is not another trip through the mud
    inline float dirtForPreview(int model, float initial)
    {
        initial = std::isfinite(initial) ? std::clamp(initial, 0.f, 15.f) : 0.f;
        const auto found = previewDirt.find(model);
        if (found != previewDirt.end()) {
            return found->second;
        }
        if (previewDirt.size() < 4096) {
            previewDirt.emplace(model, initial);
        }
        return initial;
    }

    inline std::unique_ptr<Mesh> captureVehicle(void* vehicle, int id, void (*prepare)(void*) = nullptr,
                                                bool clean = false)
    {
        if (!vehicle || target) {
            captureFailure = "vehicle missing or capture already active";
            return nullptr;
        }
        void* clump = game::field<void*>(vehicle, 0x18);
        if (!clump) {
            captureFailure = "vehicle has no clump";
            return nullptr;
        }
        auto* device = game::device();
        IDirect3DStateBlock9* saved = nullptr;
        if (!beginCaptureState(device, saved)) {
            captureFailure = "graphics state acquisition failed";
            return nullptr;
        }
        RenderTargets targets(device);
        if (!targets.valid() || !game::renderCamera()) {
            captureFailure = "render targets or native render camera unavailable";
            saved->Release();
            return nullptr;
        }
        targets.track();
        float dirt = 0.f;
        if (!clean) {
            dirt = PreviewDirtScope::level;
            if (dirt < 0) {
                dirt = dirtForPreview(id, game::field<float>(vehicle, 0x4B0));
            }
        }
        PreviewDirtScope dirtScope(dirt);
        // preparation can render this same vehicle again; the requested dirt must already be in force
        game::field<float>(vehicle, 0x4B0) = dirt;
        if (prepare) {
            prepare(vehicle);
        }
        const float preparedDirt = game::field<float>(vehicle, 0x4B0);
        markerSlots.clear();
        Pins pins;
        reinterpret_cast<void*(__cdecl*)(void*, void*(__cdecl*)(void*, void*), void*)>(0x749B70)(clump, pinAtomic,
                                                                                                 &pins);
        if (!pins.valid) {
            captureFailure = "material ownership pin failed";
            for (auto& p : pins.values) {
                reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);
                if (p.texture) {
                    reinterpret_cast<bool(__cdecl*)(void*)>(0x7F3820)(p.texture);
                }
            }
            saved->Apply();
            saved->Release();
            return nullptr;
        }
        game::seedVehicle(vehicle, id);
        game::field<float>(vehicle, 0x4B0) = clean ? 0.f : dirt;
        visibility.bind();
        lastModel = id;
        lastSources = sourceGroups(game::field<void*>(clump, 4));
        lastRemaining = lastSources;
        lastWheels = 0;
        if (copyGeometry && game::field<int>(gta::modelInfo(id), 0x3C) == 0) {
            for (int node : {2, 5, 4, 7, 3, 6}) {
                if (game::field<void*>(vehicle, 0x648 + size_t(node) * 4)) {
                    ++lastWheels;
                }
            }
        }
        target = vehicle;
        result.reset();
        reached = false;
        bool materialsWarmed = false;
        float afterSetupDirt = -1, afterRenderDirt = -1;
        previewMaterialPasses(
            vehicle,
            [&](void* subject) {
                setupMaterials(subject);
                afterSetupDirt = game::field<float>(subject, 0x4B0);
            },
            [&](void* subject) {
                // modelextras waits callbacks before its material pass. let it finish before copying textures
                const int passes = !materialsWarmed && GetModuleHandleW(L"ModelExtras.asi") ? 12 : 1;
                renderMaterialFrames(subject, reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E60), passes);
                afterRenderDirt = game::field<float>(subject, 0x4B0);
                materialsWarmed = true;
            },
            reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E20),
            [&](void* subject) {
                if (!clean || !reached) {
                    return false;
                }
                // same car, same parts, just clean this time. rebuild the textures after the script settles
                result.reset();
                reached = false;
                game::field<float>(subject, 0x4B0) = 0.f;
                return true;
            });
        target = nullptr;
        markerSlots.clear();
        if (saved) {
            saved->Apply();
            saved->Release();
        }
        const bool graphicsRestored = targets.restore();
        restore(pins.values);
        if (diagnostic && prepare) {
            char detail[280]{};
            std::snprintf(detail, sizeof(detail),
                          "Appearance dirt: model=%d clean=%d requested=%.3f prepared=%.3f setup=%.3f "
                          "render=%.3f captured=%.3f reached=%d geometry=%d",
                          id, clean, dirt, preparedDirt, afterSetupDirt, afterRenderDirt,
                          result ? result->capturedDirt : -1.f, reached, copyGeometry);
            diagnostic(detail);
        }
        failedPath = !reached;
        if (reached) {
            retired = std::move(pins.values);
        } else {
            failedPath = true;
            retired.insert(retired.end(), pins.values.begin(), pins.values.end());
        }
        if (!graphicsRestored) {
            captureFailure = "graphics target restoration failed";
            result.reset();
            return nullptr;
        }
        if (result) {
            result->compact();
            result->bounds();
            if (!result->empty() && !result->textureCaptureFailed) {
                captureFailure.clear();
                return std::move(result);
            }
            char detail[320]{};
            std::snprintf(detail, sizeof(detail),
                          "stage=%s atomics=%zu hidden=%zu nativeOnly=%zu batches=%zu texture=%s format=%u size=%ux%u copiedBytes=%zu",
                          result->textureCaptureFailed ? "texture-copy" : "geometry",
                          result->visitedAtomics, result->hiddenAtomics, result->nativeOnly,
                          result->batches.size(), result->failedTexture.c_str(), result->failedTextureFormat,
                          result->failedTextureWidth, result->failedTextureHeight, result->textureBytes);
            captureFailure = detail;
            result.reset();
        } else {
            captureFailure = reached ? "render boundary produced no mesh" : "native render boundary not reached";
        }
        return nullptr;
    }

    inline std::unique_ptr<Mesh> snapshot(int id, void (*prepare)(void*) = nullptr, bool clean = false,
                                          unsigned textureLimit = 0)
    {
        PreviewTextureScope textureScope(textureLimit);
        captureFailure.clear();
        const bool plugin = GetModuleHandleW(L"VehFuncs.asi") || GetModuleHandleW(L"ImVehFt.asi") ||
                            GetModuleHandleW(L"IVF.asi") || GetModuleHandleW(L"ModelExtras.asi");
        auto fallback = [&]() -> std::unique_ptr<Mesh> {
            return plugin || clean ? nullptr : game::snapshot(id);
        };
        if (!enabledFor(id) && !prepare) {
            captureFailure = "unsupported vehicle model type";
            return copyGeometry ? fallback() : nullptr;
        }

        struct Pool
        {
            void* objects;
            uint8_t* flags;
            int count;
        };

        auto* pool = game::at<Pool*>(0xB74494);
        bool free = false;
        if (pool && pool->count > 0 && pool->count < 1000000) {
            for (int i = 0; i < pool->count; ++i) {
                if (pool->flags[i] & 0x80) {
                    free = true;
                    break;
                }
            }
        }
        if (!free) {
            captureFailure = "no free native vehicle pool slot";
            return prepare || !copyGeometry ? nullptr : fallback();
        }
        game::ComponentChoiceScope components(id);
        void* vehicle = reinterpret_cast<void*(__cdecl*)(int, uint8_t)>(0x421440)(id, 2);
        if (!vehicle) {
            captureFailure = "native vehicle factory returned null";
            return prepare || !copyGeometry ? nullptr : fallback();
        }
        auto mesh = captureVehicle(vehicle, id, prepare, clean);
        auto** vt = game::field<void**>(vehicle, 0);
        reinterpret_cast<void*(__thiscall*)(void*, unsigned)>(vt[0])(vehicle, 1);
        return mesh ? std::move(mesh) : prepare || !copyGeometry ? nullptr : fallback();
    }

    struct InspectionScope
    {
        bool saved = copyGeometry;

        InspectionScope()
        {
            copyGeometry = false;
        }

        ~InspectionScope()
        {
            copyGeometry = saved;
        }
    };

    inline bool inspect(int id)
    {
        InspectionScope scope;
        reached = false;
        snapshot(id);
        return reached;
    }

    inline bool initialize(void* vehicle, int id, bool clean = false)
    {
        InspectionScope scope;
        reached = false;
        captureVehicle(vehicle, id, nullptr, clean);
        return reached;
    }
}
