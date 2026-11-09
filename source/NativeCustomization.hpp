#pragma once
#include "CustomizationState.hpp"
#include "NativePrize.hpp"
#include "NativeExtras.hpp"

namespace gt4::customizationNative
{
    struct Resources
    {
        std::set<int> models, txds;
        PrizeLoadGate gate;
        PrizeStreamReport report;

        template <class Backend> bool stream(const PrizePlan& plan, Backend& backend)
        {
            if (plan.enablePaintjob && plan.remapTxd >= 0) {
                if (!backend.validTxd(plan.remapTxd)) {
                    report.stage = PrizeStage::Invalid;
                    report.missing = plan.remapTxd;
                    return false;
                }
                if (!txds.count(plan.remapTxd)) {
                    backend.addTxdRef(plan.remapTxd);
                    txds.insert(plan.remapTxd);
                }
            }
            report = probePrize(plan, backend);
            if (gate.shouldFlush(plan, report)) {
                backend.flush();
                report = probePrize(plan, backend);
            }
            if (!report.ready()) {
                return false;
            }
            auto needed = prizeParts(plan);
            needed.push_back(plan.id);
            for (int id : needed) {
                if (!models.count(id)) {
                    if (!backend.addModelRef(id)) {
                        return false;
                    }
                    models.insert(id);
                }
            }
            return true;
        }

        template <class Backend> void discardUnused(const PrizePlan& plan, Backend& backend)
        {
            auto needed = prizeParts(plan);
            needed.push_back(plan.id);
            for (auto it = models.begin(); it != models.end();) {
                if (std::find(needed.begin(), needed.end(), *it) == needed.end()) {
                    backend.removeModelRef(*it);
                    it = models.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = txds.begin(); it != txds.end();) {
                if (!plan.enablePaintjob || *it != plan.remapTxd) {
                    backend.removeTxdRef(*it);
                    it = txds.erase(it);
                } else {
                    ++it;
                }
            }
        }

        template <class Backend> void clear(Backend& backend)
        {
            for (int id : models) {
                backend.removeModelRef(id);
            }
            for (int id : txds) {
                backend.removeTxdRef(id);
            }
            models.clear();
            txds.clear();
            gate = {};
            report = {};
        }
    };

    struct Backend : prizeNative::Backend
    {
        bool validUpgrade(int id)
        {
            void* info = gta::modelInfo(id);
            uint16_t flags = 0;
            return info && gta::read(uintptr_t(info) + 0x12, flags) && customizationSlotName(tuningSlot(flags));
        }

        bool validTxd(int id)
        {
            return prizeNative::validTxdSlot(id);
        }

        void queueTxd(int id)
        {
            reinterpret_cast<void(__cdecl*)(int, int)>(0x407100)(id, prizeStreamingFlags);
        }

        void addTxdRef(int id)
        {
            reinterpret_cast<void(__cdecl*)(int)>(0x731A00)(id);
        }

        void removeTxdRef(int id)
        {
            if (prizeNative::validTxdSlot(id)) {
                reinterpret_cast<void(__cdecl*)(int)>(0x731A70)(id);
            }
        }

        bool addModelRef(int id)
        {
            if (void* info = gta::modelInfo(id)) {
                reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BA0)(info);
                return true;
            }
            return false;
        }

        void removeModelRef(int id)
        {
            if (void* info = gta::modelInfo(id)) {
                reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BB0)(info);
            }
        }

        void flush()
        {
            reinterpret_cast<void(__cdecl*)(bool)>(0x40EA10)(false);
        }
    };

    inline std::map<uint32_t, std::string> partTextKeys;

    inline void loadPartNames(const std::filesystem::path& path)
    {
        partTextKeys.clear();
        std::ifstream data(path);
        std::string line;
        bool inParts = false;
        while (std::getline(data, line)) {
            std::istringstream fields(line.substr(0, line.find('#')));
            std::string model, key, respect;
            fields >> model >> key >> respect;
            if (lowerName(model) == "section") {
                inParts = lowerName(key) == "carmods";
            } else if (inParts && lowerName(model) == "end") {
                break;
            } else if (inParts && lowerName(respect) == "respect" && !model.empty() && !key.empty() &&
                       key.size() <= 8) {
                partTextKeys[modelNameKey(model)] = key;
            }
        }
    }

    inline std::string partName(int id, int slot)
    {
        void* info = gta::modelInfo(id);
        const auto key = partTextKeys.find(info ? game::field<uint32_t>(info, 4) : 0);
        if (key != partTextKeys.end()) {
            const auto lookup = reinterpret_cast<const char*(__thiscall*)(void*, const char*)>(0x6A0050);
            const char* value = lookup(reinterpret_cast<void*>(0xC1B340), key->second.c_str());
            if (value && *value && std::strcmp(value, "NULL")) {
                std::string name;
                for (size_t i = 0; i < 100 && value[i]; ++i) {
                    name += uint8_t(value[i]) < 128 ? value[i] : '?';
                }
                return name;
            }
        }
        if (const char* name = customizationSlotName(slot)) {
            return name;
        }
        return {};
    }

    inline CustomizationState state;
    inline Resources resources;
    inline std::unique_ptr<Mesh> preview;
    inline CustomizationAppearance capturedAppearance{};
    inline CustomizationCaptureRetry captureRetry;
    inline uint64_t capturedRevision = 0;
    inline const Entry* preparingEntry = nullptr;
    inline const PrizePlan* preparingPlan = nullptr;
    inline size_t preparingPaint = 0;
    inline bool prepared = false;

    struct PreparationScope
    {
        PreparationScope(const Entry& entry, const PrizePlan& selection, size_t paint)
        {
            preparingEntry = &entry;
            preparingPlan = &selection;
            preparingPaint = paint;
            prepared = false;
        }

        ~PreparationScope()
        {
            preparingEntry = nullptr;
            preparingPlan = nullptr;
        }

        PreparationScope(const PreparationScope&) = delete;
        PreparationScope& operator=(const PreparationScope&) = delete;
    };

    inline void clear()
    {
        preview.reset();
        Backend backend;
        resources.clear(backend);
        state.clear();
        capturedRevision = 0;
        captureRetry = {};
    }

    inline bool begin(const Entry& entry)
    {
        if (state.model == entry.id) {
            return true;
        }
        if (!gta::modelReady(entry.id)) {
            return false;
        }
        void* info = gta::modelInfo(entry.id);
        if (!game::vehicleInfo(info)) {
            return false;
        }
        clear();
        std::map<int, CustomizationPart> parts;
        const auto compatible = prizeNative::compatible(info, true);
        for (const auto& option : compatible) {
            if (parts.size() >= CustomizationState::maxParts || !customizationSlotName(option.slot)) {
                continue;
            }
            const int linked = prizeNative::pairFor(option.id);
            parts[option.id] = {option.id, option.slot, linked, partName(option.id, option.slot), true};
            if (linked >= 0 && !parts.count(linked)) {
                void* other = gta::modelInfo(linked);
                uint16_t flags = 0;
                if (other && gta::read(uintptr_t(other) + 0x12, flags) && customizationSlotName(tuningSlot(flags))) {
                    parts[linked] = {linked, tuningSlot(flags), prizeNative::pairFor(linked), {}, false};
                }
            }
        }
        std::vector<CustomizationPart> registrations;
        registrations.reserve(parts.size());
        for (auto& part : parts) {
            registrations.push_back(std::move(part.second));
        }
        state.begin(entry.id, registrations);
        return true;
    }

    inline bool select(size_t row, int option)
    {
        return state.select(row, option);
    }

    inline PrizePlan plan(int livery, int txd)
    {
        PrizePlan result;
        result.id = state.model;
        result.modkit = true;
        result.kitResolved = result.paintResolved = true;
        result.upgrades = state.selectedParts();
        result.serial = uint32_t(state.revision) ^ (uint32_t(txd) + 1u) * 0x9E3779B9u;
        result.enablePaintjob = livery >= 0;
        result.paintjob = std::max(0, livery);
        result.remapTxd = txd;
        return result;
    }

    inline bool assetsReady(int livery, int txd)
    {
        if (state.model < 0 || (livery >= 0 && txd < 0)) {
            return false;
        }
        Backend backend;
        return resources.stream(plan(livery, txd), backend);
    }

    inline CustomizationAppearance appearance(const Entry& entry, size_t paint, int livery, int txd, bool clean = false)
    {
        std::array<int, 4> colors{};
        if (!entry.paints.empty()) {
            const auto& selected = entry.paints[std::min(paint, entry.paints.size() - 1)];
            std::copy(std::begin(selected.ids), std::end(selected.ids), colors.begin());
        }
        return {{int(paint), livery, txd, game::extraChoice, game::extraSecondChoice, game::extrasSeed, colors[0],
                 colors[1], colors[2], colors[3], entry.id, int(clean)}};
    }

    inline bool current(const Entry& entry, size_t paint, int livery, int txd, bool clean = false)
    {
        return preview && !preview->empty() && state.model == entry.id && capturedRevision == state.revision &&
               capturedAppearance == appearance(entry, paint, livery, txd, clean);
    }

    inline bool ready(int livery, int txd)
    {
        return preview && !preview->empty() && capturedRevision == state.revision && capturedAppearance[1] == livery &&
               capturedAppearance[2] == txd && capturedAppearance[3] == game::extraChoice &&
               capturedAppearance[4] == game::extraSecondChoice && capturedAppearance[5] == game::extrasSeed &&
               assetsReady(livery, txd);
    }

    inline void resetCapture()
    {
        preview.reset();
        capturedRevision = 0;
        captureRetry = {};
    }

    inline bool tune(void* vehicle, const PrizePlan& selection, bool isPreview)
    {
        if (!prizeNative::tune(vehicle, selection, isPreview)) {
            return false;
        }
        for (int id : selection.upgrades) {
            void* info = gta::modelInfo(id);
            uint16_t flags = 0;
            if (!info || !gta::read(uintptr_t(info) + 0x12, flags)) {
                return false;
            }
            if (!tuningEffect(tuningSlot(flags))) {
                if (!prizeNative::observed.count(info)) {
                    return false;
                }
            } else if (!isPreview) {
                bool installed = false;
                for (int slot = 0; slot < 15; ++slot) {
                    installed =
                        installed || prizeNative::extended16(static_cast<uint8_t*>(vehicle) + 0x43A + slot * 2) == id;
                }
                if (!installed) {
                    return false;
                }
            }
        }
        return true;
    }

    inline void preparePreview(void* vehicle)
    {
        prepared = false;
        if (!preparingEntry || !preparingPlan) {
            return;
        }
        bool applied = false;
        const bool initialized = game::finalizeVehicleAppearance(
            vehicle, *preparingEntry, preparingPaint,
            [](void* target, int id) {
                return nativePreview::initialize(target, id);
            },
            [&](void* target) {
                applied = tune(target, *preparingPlan, true);
            });
        prepared = initialized && applied;
    }

    // washing changes captured textures, so it belongs in the same identity as paint and upgrades.
    inline bool update(const Entry& entry, size_t paint, int livery, int txd, bool clean = false)
    {
        if (!begin(entry)) {
            return false;
        }
        const auto selectedAppearance = appearance(entry, paint, livery, txd, clean);
        if (current(entry, paint, livery, txd, clean)) {
            return true;
        }
        if (!assetsReady(livery, txd) || !captureRetry.allow(state.revision, selectedAppearance, GetTickCount64())) {
            return false;
        }
        const auto selection = plan(livery, txd);
        PreparationScope preparation(entry, selection, paint);
        auto mesh = nativePreview::snapshot(entry.id, preparePreview, clean);
        if (!prepared || !mesh) {
            return false;
        }
        preview = std::move(mesh);
        capturedRevision = state.revision;
        capturedAppearance = selectedAppearance;
        Backend backend;
        resources.discardUnused(selection, backend);
        return true;
    }

    inline bool apply(void* vehicle, int livery, int txd)
    {
        if (!vehicle || !ready(livery, txd)) {
            return false;
        }
        return tune(vehicle, plan(livery, txd), false);
    }
} // namespace gt4::customizationNative
