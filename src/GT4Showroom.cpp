#include "Game.hpp"
#include "Dealership.hpp"
#include "VehiclePayment.hpp"
#include "DiagnosticLog.hpp"
#include "Audio.hpp"
#include "InstallLock.hpp"
#include "VideoMemoryHook.hpp"
#include "FilterConfig.hpp"
#include "Favorites.hpp"
#include "HotkeyConfig.hpp"
#include "KeyboardInput.hpp"
#include "StudioConfig.hpp"
#include "TutorialConfig.hpp"
#include "NativePreview.hpp"
#include "NativePrize.hpp"
#include "NativeLiveries.hpp"
#include "NativeExtras.hpp"
#include "NativeCustomization.hpp"
#include "SpawnEntry.hpp"
#include "OutcomeCamera.hpp"
#include "FixedCameraBridge.hpp"
#include "RecoveryFade.hpp"
#include "SearchInput.hpp"
#include "Controller.hpp"
#include "PaintSources.hpp"
#include "SecretFeatures.hpp"
#include "MinHook.h"
#include <cstdio>
#include <map>
#include <atomic>
#include <set>
#include <filesystem>

namespace gt4
// a lot of the important shit are in here basically deals with main renderer, cinematics, ui etc
// read and analyze carefully if you want to steal
{
    HMODULE module = nullptr;
    std::wstring base, logFile, ini;
    Model state;
    int extrasModel = -1, dirtyExtraModel = -1, extrasFramingModel = -1;
    std::shared_ptr<const Mesh> extrasFraming;
    std::map<int, std::array<int, 3>> selectedExtras;
    ReturnFade returnFade;
    bool dealershipCameraPending = false;
    int dealershipCameraVehicle = -1;
    double dealershipCameraWait = 0;
    std::unique_ptr<Mesh> prizeMesh;
    uint32_t prizeSerial = 0;
    bool prizePlayed = false, prizeCaptureTried = false;
    int prizeModel = -1;
    std::string prizeProgress;
    Renderer renderer;
    Audio audio;

    void syncGameAudioLevels()
    {
        uint8_t effects = 64, music = 64;
        if (gta::read(0xBA6797, effects) && gta::read(0xBA6798, music)) {
            audio.setGameLevels(effects, music);
        }
    }
    bool gameVolumeDirty = false;

    bool readGameVolume(std::array<int, 2>& levels)
    {
        uint8_t effects = 0, music = 0;
        if (!gta::read(0xBA6797, effects) || !gta::read(0xBA6798, music) || effects > 64 || music > 64) {
            return false;
        }
        levels = {int(music), int(effects)};
        return true;
    }

    // use the game's setters as well as its menu values; direct byte writes alone leave audio unchanged
    bool setGameVolume(const std::array<int, 2>& levels)
    {
        constexpr std::array<uint8_t, 9> expected{0x0F, 0xBE, 0x44, 0x24, 0x04, 0x89, 0x44, 0x24, 0x04};
        std::array<uint8_t, 9> music{}, effects{};
        if (!gta::read(0x506DE0, music) || !gta::read(0x506E10, effects) ||
            music != expected || effects != expected) {
            return false;
        }
        const int radio = std::clamp(levels[0], 0, 64), sfx = std::clamp(levels[1], 0, 64);
        using SetVolume = void(__thiscall*)(void*, char);
        reinterpret_cast<SetVolume>(0x506DE0)(reinterpret_cast<void*>(0xB6BC90), char(radio));
        reinterpret_cast<SetVolume>(0x506E10)(reinterpret_cast<void*>(0xB6BC90), char(sfx));
        game::at<uint8_t>(0xBA6798) = uint8_t(radio);
        game::at<uint8_t>(0xBA6797) = uint8_t(sfx);
        gameVolumeDirty = true;
        syncGameAudioLevels();
        return true;
    }

    // gta_sa.set includes hud/radar state too, so wait until the pause lease has restored gameplay values
    void commitGameVolume()
    {
        constexpr std::array<uint8_t, 7> expected{0x83, 0xEC, 0x0C, 0x56, 0x57, 0x8B, 0xF9};
        std::array<uint8_t, 7> actual{};
        if (gameVolumeDirty && !game::loading() && gta::read(0x57C660, actual) && actual == expected) {
            using Save = void(__thiscall*)(void*);
            reinterpret_cast<Save>(0x57C660)(reinterpret_cast<void*>(0xBA6748));
            gameVolumeDirty = false;
        }
    }

    game::PauseLease pause;
    std::vector<Entry> entries, catalog;
    std::set<int> excluded;
    ExclusionConfig exclusionConfig;
    CountryConfig countryConfig;
    SearchInput searchInput;
    float mouseSensitivity = 1;
    bool suspended = true;
    uint64_t frameSerial = 0, updatedFrame = UINT64_MAX;
    StudioConfig studioConfig;
    Gallery gallery = Gallery::Main;
    GalleryOptions galleryOptions;
    bool dealerShopVisited = false;
    bool allowedHere(const Entry& entry)
    {
        return !excluded.count(entry.id) && (gallery == Gallery::Main ||
            (gallery == Gallery::Grotti ? std::find(galleryOptions.models.begin(),galleryOptions.models.end(),entry.id)!=galleryOptions.models.end() :
             (entry.kind != 6 && (galleryOptions.allowed & classBit(galleryClass(entry))) != 0)));
    }
    bool canCustomizeSelected()
    {
        return state.selected < entries.size() && galleryCustomizes(gallery,entries[state.selected]);
    }
    TypedReload typedReload;
    Key reloadChord;
    MoveCueGate moveCue;
    uint64_t statusUntil = 0;
    OpenIntent openIntent;
    FunctionKeyClaim functionKey;
    ControllerInput joystick;
    Controller controller;
    PadActions sampledPad;
    Key pauseButton, cinematicPanels, shopResetKey;
    bool controllerPauseRequested = false;
    ExitButtons exitButtons;

    struct Cached
    {
        std::shared_ptr<Mesh> mesh;
        double touched = 0;
    };

    std::map<int, Cached> cache;
    std::vector<int> failed;
    std::map<int, uint64_t> pending;
    uint64_t lastRenderLog = 0;
    double drawTotal = 0, drawWorst = 0;
    uint64_t drawSamples = 0;
    std::set<int> known;
    uint64_t lastScan = 0;
    uint32_t scan = 0;
    bool catalogueWarm = false;
    uintptr_t registryTable = 0;
    uint64_t prev = 0;
    int hotkey = VK_F1;
    bool ctrl = false;
    bool initialized = false;
    std::atomic<bool> scheduled = false;
    INIT_ONCE init = INIT_ONCE_STATIC_INIT;
    int result = 0;
    DiagnosticLog diagnostics;
    SpawnEntry spawnEntry;
    OutcomeCamera outcomeCamera;
    void* pendingEntry = nullptr;
    V3 pendingEntryLow{}, pendingEntryHigh{};
    SecretFeatures secretFeatures;
    SecretFeaturesSchoolNative secretSchool;
    SecretFeaturesRenderer secretSchoolView;
    uint64_t secretSchoolSerial = 0;
    bool secretSchoolMusicPlayed = false;
    SecretFeaturesChoice secretSchoolPending = SecretFeaturesChoice::None;
    double secretSchoolClosing = 0;
#include "DealershipWorld.inl"
    void blockOpenInput();
    void finishSettings();

    void log(const char* s)
    {
        diagnostics.write(logFile, s);
    }

    void syncCaptureDiagnostics()
    {
        nativePreview::diagnostic = diagnostics.enabled.load(std::memory_order_relaxed) ? log : nullptr;
    }

    void saveCurrentFavorites()
    {
        const std::filesystem::path file = std::filesystem::path(base) / L"GT4Showroom/favorites.ini";
        if (!saveFavorites(file, entries)) { log("Favorites could not be saved"); }
    }

    void refreshCountryMenu()
    {
        std::array<int, countryCount> counts{};
        std::array<int, classCount> classTotals{};
        for (const auto& e : catalog) {
            if (allowedHere(e) && (!state.ui.favoritesOnly || e.favorite)) {
                ++counts[size_t(e.country)];
                ++classTotals[size_t(classIndex(e.classId))];
            }
        }
        state.hasAvailableFavorites = std::any_of(catalog.begin(), catalog.end(), [&](const Entry& e) {
            return e.favorite && allowedHere(e);
        });
        state.ui.menu.setCountries(counts);
        state.ui.menu.setClasses(classTotals);
    }

    void rebuildCatalogueView(int preferredModel = -1)
    {
        const int retainedModel = preferredModel >= 0 ? preferredModel : entries.empty() ? -1 : entries[std::min(state.selected,entries.size()-1)].id;
        localization.protectedNames.clear();
        for (const auto& entry : catalog) {
            localization.protectedNames.insert(entry.name);
            localization.protectedNames.insert(entry.title);
            localization.protectedNames.insert(entry.modelName);
        }
        buildCatalogueView(
            state, entries, catalog, studioConfig.sort,
            [&](const Entry& e) {
                return !allowedHere(e);
            },
            [](const Entry& e) {
                return e.favorite;
            },
            preferredModel);
        if (gallery == Gallery::Grotti) {
            std::stable_sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b) {
                return std::find(galleryOptions.models.begin(),galleryOptions.models.end(),a.id) <
                       std::find(galleryOptions.models.begin(),galleryOptions.models.end(),b.id);
            });
            state.selected=0;
            for (size_t i=0;i<entries.size();++i) if(entries[i].id==retainedModel) state.selected=i;
            state.scroll=state.scrollTarget();
            for (int id:galleryOptions.models) {
                if (std::none_of(catalog.begin(),catalog.end(),[id](const Entry& e){return e.id==id;})) {
                    char message[100]{};
                    snprintf(message,sizeof(message),"Grotti configured model %d is unavailable in the vehicle catalogue",id);
                    log(message);
                }
            }
        }
        pending.clear();
        refreshCountryMenu();
    }

    void reloadExclusions()
    {
        const auto config = readExclusions(ini);
        exclusionConfig = config;
        excluded = config.ids;
        for (const auto& e : catalog) {
            if (config.hides(e)) {
                excluded.insert(e.id);
            }
        }
        const size_t invalid = config.invalid;
        const auto favoritesFile = base + L"GT4Showroom\\favorites.ini";
        for (auto& e : catalog) {
            e.favorite =
                GetPrivateProfileIntW(L"Favorites", std::to_wstring(e.id).c_str(), 0, favoritesFile.c_str()) != 0;
        }
        rebuildCatalogueView();
        char b[160];
        std::snprintf(b, sizeof(b),
                      "Model filter: %zu excluded IDs, %zu visible registered vehicles, %zu invalid tokens ignored",
                      excluded.size(), entries.size(), invalid);
        log(b);
    }

    void reloadSettings()
    {
        auto savedPreferences = state.ui.preferences();
        if (loadStudioPreferences(std::filesystem::path(base) / L"GT4Showroom/settings.ini", savedPreferences)) {
            state.ui.applyPreferences(savedPreferences);
            mainStyleChoice=savedPreferences.showroomStyle;
        }
        readGalleryAccess();
        galleryOptions = readGallery(ini.c_str(), gallery);
        state.allowRealBig = gallery == Gallery::Main || gallery == Gallery::Wang;
        state.ui.menu.categoriesEnabled = gallery != Gallery::Grotti;
        state.ui.dealerBackground = gallery == Gallery::Main ? -1 : galleryBackground(ini.c_str(), gallery);
        state.ui.showroomStyle = galleryStyle(ini.c_str(), gallery, mainStyleChoice);
        state.ui.menu.musicChoice = galleryOptions.music;
        state.ui.mainDefaultBackground = galleryBackground(ini.c_str(), Gallery::Main);
        studioConfig = readStudioConfig(ini);
        studioConfig.sort = menuSort(state.ui.sortMode);
        diagnostics.reload(ini, L"GT4Showroom");
        syncCaptureDiagnostics();
        state.secretUnlocks = readSecretUnlocks(ini);
        char secretReport[200];
        std::snprintf(secretReport, sizeof(secretReport),
                      "SecretUnlocks: %zu IDs, %zu names, %zu invalid tokens; modkit=%d paintjob=%d abilities=%d",
                      state.secretUnlocks.models.ids.size(), state.secretUnlocks.models.names.size(),
                      state.secretUnlocks.models.invalid, state.secretUnlocks.enableModkit,
                      state.secretUnlocks.enablePaintjob, state.secretUnlocks.enableAbilities);
        log(secretReport);
        countryConfig = readCountries(ini);
        wchar_t exe[32768]{};
        GetModuleFileNameW(nullptr, exe, 32768);
        const auto root = std::filesystem::path(exe).parent_path();
        customizationNative::loadPartNames(root / L"data/shopping.dat");
        const auto names = reloadPaintNames(root);
        char paintReport[240];
        std::snprintf(paintReport, sizeof(paintReport),
                      "Paint names: %zu eligible carcols files, %zu named rows; Mod Loader profile %s Country tokens "
                      "rejected: %zu",
                      names.files, names.rows, names.profile.c_str(), countryConfig.invalid);
        log(paintReport);
        for (auto& e : catalog) {
            e.country = countryConfig.resolve(e);
        }
        for (auto& e : entries) {
            e.country = countryConfig.resolve(e);
        }
        mouseSensitivity = 1.f;
        syncGameAudioLevels();
        renderer.studioZoom = 1.15f;
        secretFeatures = readSecretFeatures(ini);
        secretSchool.script.enabled = secretFeatures.enabled;
    }

    void clearShopGeometry();

    void close(bool reveal = false)
    {
        if (state.ui.countryShortcut) {
            const int model = state.ui.endCountryShortcut();
            studioConfig.sort = menuSort(state.ui.sortMode);
            rebuildCatalogueView(model);
        }
        if (state.ui.menuSnapshot) {
            state.ui.menu.finish(-1);
            finishSettings();
        }
        if (state.page == Page::Prize) {
            const int previousModel = state.beforePrizeModel;
            state.leavePrize();
            for (size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].id == previousModel) {
                    state.selected = i;
                    state.scroll = state.scrollTarget();
                    break;
                }
            }
        }
        renderer.releaseStudioEnvironment();
        nativeExtras::clear();
        customizationNative::clear();
        clearShopGeometry();
        state.shop = {};
        if (state.photo.active && state.photo.studioViewContext) state.studioView=state.photo.originalStudioView;
        state.photo = {};
        extrasModel = -1;
        dirtyExtraModel = extrasFramingModel = -1;
        extrasFraming.reset();
        state.ui.menu.setExtras({}, {}, {});
        liveryNative::clear(state);
        prizeNative::release();
        const bool wasOpen = state.open || pause.owned;
        dealershipCameraVehicle=-1;
        if (reveal && wasOpen) {
            returnFade.begin(state.fade.action==CloseAction::Spawn ? .5 : ReturnFade::duration);
        } else if (!reveal) {
            returnFade.reset();
        }
        prizeMesh.reset();
        prizeSerial = 0;
        prizePlayed = false;
        prizeCaptureTried = false;
        prizeModel = -1;
        prizeProgress.clear();
        if (pause.owned && !game::menu() && !game::loading()) {
            if (void* p = game::pad()) {
                exitButtons.arm(static_cast<int16_t*>(p));
            }
        }
        openIntent.clear();
        blockOpenInput();
        suspended = true;
        state.open = false;
        state.spawn = false;
        state.fade.reset();
        state.searching = false;
        state.informationOpen = false;
        state.tutorial = Tutorial::None;
        state.ui.resetTransient();
        state.ui.favoritesOnly = false;
        audio.stopBed();
        audio.setShopMusic(false);
        audio.setTransitionGain(1.f);
        pause.end();
        commitGameVolume();
        saveCurrentFavorites();
        if (gallery != Gallery::Main) {
            gallery = Gallery::Main;
            state.showDealerPrices = state.dealerPurchaseEnabled = false;
            state.galleryFavorites = true;
            state.ui.dealerBackground = -1;
            state.ui.applyPreferences(mainPreferences);
            studioConfig.sort = menuSort(state.ui.sortMode);
            galleryOptions = readGallery(ini.c_str(), gallery);
            state.allowRealBig = gallery == Gallery::Main || gallery == Gallery::Wang;
            state.ui.menu.categoriesEnabled = gallery != Gallery::Grotti;
            state.ui.showroomStyle = galleryStyle(ini.c_str(), gallery, mainStyleChoice);
        state.ui.menu.musicChoice = galleryOptions.music;
        state.ui.mainDefaultBackground = galleryBackground(ini.c_str(), Gallery::Main);
            rebuildCatalogueView(mainVehicle);
            selectedExtras.clear();
            dealerShopVisited = false;
            renderer.setGalleryBrand(std::filesystem::path(base)/L"GT4Showroom",0,"");
        }
        arrivalTime = -1;
        approaching = Gallery::Main;
        pending.clear();
    }

    void metadata(Entry& e)
    {
        e.country = countryConfig.resolve(e);
        const auto section = std::to_wstring(e.id);
        const std::wstring file = base + L"GT4Showroom/vehicles.ini";
        auto get = [&](const wchar_t* key, std::string& value) {
            char out[256]{};
            const auto k = std::string(key, key + wcslen(key));
            const auto id = std::to_string(e.id);
            GetPrivateProfileStringA(id.c_str(), k.c_str(), "", out, 256,
                                     std::string(file.begin(), file.end()).c_str());
            if (*out) {
                value = out;
            }
        };
        get(L"Name", e.name);
        get(L"Title", e.title);
        get(L"Brand", e.brand);
        get(L"Year", e.year);
        get(L"Power", e.power);
        get(L"Torque", e.torque);
        const auto nativeInformation = e.information;
        get(L"Information", e.information);
        e.customInformation = e.information != nativeInformation;
        for (size_t i = 0; i < e.paints.size(); ++i) {
            const auto key = L"Paint" + std::to_wstring(i + 1);
            get(key.c_str(), e.paints[i].name);
            e.paints[i].name = properPaintName(e.paints[i].name);
        }
    }

    void discover()
    {
        const auto r = gta::registry();
        if (!r.table) {
            return;
        }
        if (r.table != registryTable) {
            if (state.open) {
                close();
            }
            nativeExtras::resetCatalogues();
            entries.clear();
            catalog.clear();
            known.clear();
            selectedExtras.clear();
            extrasFraming.reset();
            extrasFramingModel = -1;
            dirtyExtraModel = -1;
            cache.clear();
            failed.clear();
            pending.clear();
            renderer.reset();
            scan = 0;
            catalogueWarm = false;
            registryTable = r.table;
        }
        if (scan >= r.count) {
            if (GetTickCount64() - lastScan < 3000) {
                return;
            }
            scan = 0;
            lastScan = GetTickCount64();
        }
        const size_t previousCatalog = catalog.size(), previousCount = entries.size();
        const int selectedId = entries.empty() ? -1 : entries[std::min(state.selected, entries.size() - 1)].id;
        const uint32_t stop = std::min(r.count, scan + 384);
        for (; scan < stop; ++scan) {
            void* mi = gta::modelInfo(int(scan));
            if (!game::vehicleInfo(mi) || known.count(int(scan))) {
                continue;
            }
            Entry e = game::entry(int(scan), mi);
            metadata(e);
            if (e.kind < 0 || e.kind > 11) {
                continue;
            }
            const auto key = std::to_wstring(e.id);
            e.favorite = GetPrivateProfileIntW(L"Favorites", key.c_str(), 0,
                                               (base + L"GT4Showroom\\favorites.ini").c_str()) != 0;
            known.insert(e.id);
            localization.protectedNames.insert(e.name);
            localization.protectedNames.insert(e.title);
            localization.protectedNames.insert(e.modelName);
            catalog.push_back(e);
            if (exclusionConfig.hides(e)) {
                excluded.insert(e.id);
            }
            if (allowedHere(e) && (!state.ui.favoritesOnly || e.favorite) &&
                       state.ui.accepts(e.country, e.classId)) {
                entries.push_back(std::move(e));
            }
        }
        if (entries.size() != previousCount) {
            sortCatalogue(entries, studioConfig.sort);
            for (size_t i = 0; i < entries.size(); ++i) {
                if ((!catalogueWarm && !state.open && i == 0) || entries[i].id == selectedId) {
                    state.selected = i;
                    if (!catalogueWarm && !state.open) {
                        break;
                    }
                }
            }
            state.scroll = state.scrollTarget();
        }
        state.count = entries.size();
        if (scan >= r.count) {
            catalogueWarm = true;
        }
        if (catalog.size() != previousCatalog) {
            refreshCountryMenu();
        }
    }

    std::pair<int, int> selectedLivery()
    {
        if (!state.usesLivery() || state.livery >= state.liveries.size()) {
            return {-1, -1};
        }
        const auto& livery = state.liveries[state.livery];
        return {livery.index, livery.txd};
    }

    const Mesh* lookup(int id)
    {
        if (state.page == Page::Prize && state.prize.id == id) {
            return prizeMesh.get();
        }
        if (state.postSelectionCinematic && state.ui.cinematic.active && state.selected < entries.size() &&
            entries[state.selected].id == id) {
            const auto livery = selectedLivery();
            if (customizationNative::current(state.appearanceEntry(entries[state.selected]), state.paint, livery.first, livery.second,
                                             state.shop.washed)) {
                return customizationNative::preview.get();
            }
        }
        if (state.usesLivery() && state.selected < entries.size() && entries[state.selected].id == id &&
            state.livery > 0 && state.livery < state.liveries.size() && liveryNative::preview &&
            liveryNative::capturedModel == id && liveryNative::capturedPaint == int(state.paint) &&
            liveryNative::displayedLivery == state.liveries[state.livery].index &&
            !state.liveries[state.livery].appearanceTexture && !state.liveries[state.livery].appearanceMaterials) {
            return liveryNative::preview.get();
        }
        auto it = cache.find(id);
        return it != cache.end() ? it->second.mesh.get() : nullptr;
    }

    void refreshSelected()
    {
        if (entries.empty() || state.selected >= entries.size()) {
            return;
        }
        auto& e = entries[state.selected];
        state.twoWheelerSelected = twoWheelerPreview(e);
        state.galleryCustomization = galleryCustomizes(gallery,e);
        state.extrasResolved = extrasModel == e.id && nativeExtras::metadataReady && !nativeExtras::selectionPending();
        if (extrasFramingModel != e.id) {
            extrasFramingModel = -1;
            extrasFraming.reset();
        }
        if (customizationNative::state.model >= 0 && customizationNative::state.model != e.id) {
            customizationNative::clear();
        }
        void* mi = gta::modelInfo(e.id);
        if (!game::vehicleInfo(mi)) {
            return;
        }
        e.classId = game::field<int8_t>(mi, 0x4D);
        e.category = classLabel(e.classId);
        e.paints = game::paints(mi);
        const auto h = handling(game::field<uint16_t>(mi, 0x4A));
        e.topSpeed = h.valid ? h.limit : -1;
        e.information = stats(h);
        e.price = h.price;
        e.handlingMass = h.mass;
        e.handlingGears = h.gears;
        e.handlingDrive = h.drive;
        e.handlingValid = h.valid;
        metadata(e);
        liveryNative::scan(state, e.id);
    }

    bool selectStudioCustomization(int row, int option)
    {
        if (!state.postSelectionCinematic || !state.ui.cinematic.active || entries.empty() ||
            state.selected >= entries.size() || row < 0 ||
            customizationNative::state.model != entries[state.selected].id) {
            return false;
        }
        const bool selected = customizationNative::select(size_t(row), option);
        state.ui.menu.setCustomization(customizationNative::state, canCustomizeSelected());
        return selected;
    }

#include "TransFenderNative.inl"

    // load a little at a time while paused. the whole catalogue does not need to pile in at once
    void syncStudioExtraPreview();

    void fillCache(UINT outputWidth, UINT outputHeight)
    {
        if (entries.empty()) {
            liveryNative::clear(state);
            return;
        }
        if (state.page == Page::Prize) {
            if (!renderer.prizeAssetReady()) {
                state.status = "Prize backdrop unavailable BACKSPACE returns to cars";
                return;
            }
            if (prizeSerial != state.prize.serial || prizeModel != state.prize.id) {
                nativeExtras::clear();
                extrasModel = -1;
                state.ui.menu.setExtras({}, {}, {});
                prizeNative::release();
                prizeNative::loadGate = {};
                prizeMesh.reset();
                prizeSerial = state.prize.serial;
                prizeModel = state.prize.id;
                prizePlayed = false;
                prizeCaptureTried = false;
                prizeProgress.clear();
                state.status = "Loading bonus car...";
                char info[220];
                std::snprintf(info, sizeof(info), "Secret unlock selected: model=%d modkit=%d ability=%s serial=%u",
                              prizeModel, state.prize.modkit,
                              *abilityName(state.prize.ability) ? abilityName(state.prize.ability) : "NONE",
                              prizeSerial);
                log(info);
            }
            if (!prizeMesh && !prizeCaptureTried && state.clock - state.prizeBegan < 20) {
                bool ready = prizeNative::stream(state.prize);
                if (!ready && state.prize.enablePaintjob && prizeNative::report.stage == PrizeStage::Paint &&
                    state.clock - state.prizeBegan > 8) {
                    log("Optional paintjob did not finish in8s; retaining the compatible tuned car with its original "
                        "paint");
                    state.prize.enablePaintjob = false;
                    state.prize.remapTxd = -1;
                    ready = prizeNative::stream(state.prize);
                }
                const auto& progress = prizeNative::report;
                char report[240];
                std::snprintf(report, sizeof(report),
                              "Secret unlock load: model=%d stage=%s missing=%d upgrades=%d/%d paintTxd=%d.",
                              state.prize.id, prizeStageName(progress.stage), progress.missing, progress.loaded,
                              progress.total, progress.txd);
                if (prizeProgress != report) {
                    prizeProgress = report;
                    log(report);
                }
                if (ready) {
                    {
                        char kit[220];
                        std::snprintf(kit, sizeof(kit),
                                      "Resolved prize: model=%d native-compatible parts=%zu paintjob=%d paintTxd=%d; "
                                      "standalone streaming flags=%d",
                                      state.prize.id, state.prize.upgrades.size(),
                                      state.prize.enablePaintjob ? state.prize.paintjob : -1, state.prize.remapTxd,
                                      prizeStreamingFlags);
                        log(kit);
                    }
                    prizeCaptureTried = true;
                    std::unique_ptr<Mesh> mesh;
                    if (state.prize.modkit || state.prize.enablePaintjob) {
                        prizeNative::preparing = &state.prize;
                        const auto& paints = entries[state.selected].paints;
                        prizeNative::preparingPaint =
                            paints.empty() ? nullptr : &paints[std::min(size_t(state.prize.paint), paints.size() - 1)];
                        prizeNative::prepared = false;
                        mesh = nativePreview::snapshot(state.prize.id, prizeNative::preparePreview);
                        prizeNative::preparing = nullptr;
                        prizeNative::preparingPaint = nullptr;
                        if (!prizeNative::prepared) {
                            mesh.reset();
                        }
                    } else {
                        mesh = nativePreview::snapshot(state.prize.id);
                    }
                    if (mesh) {
                        {
                            char detail[160];
                            std::snprintf(
                                detail, sizeof(detail),
                                "Prize capture: %d/%d selected visual upgrades observed on the private native vehicle.",
                                prizeNative::installedVisuals, prizeNative::expectedVisuals);
                            log(detail);
                        }
                        prizeMesh = std::move(mesh);
                        state.prizeReady = true;
                        state.prizeBegan = state.clock;
                        state.status.clear();
                        prizeNative::loadGate = {};
                        audio.startPrize();
                        prizePlayed = true;
                        log("Secret unlock preview ready; confirmation available after the reveal");
                    } else {
                        state.status = "Vehicle preview unavailable BACKSPACE returns to cars";
                        log("Secret unlock capture failed after native loading completed; private preview preparation "
                            "or "
                            "geometry unavailable");
                        state.cue = Cue::Error;
                    }
                }
                if (pause.owned) {
                    game::pumpStreaming();
                }
            }
            if (!state.prizeReady && !prizeCaptureTried && state.clock - state.prizeBegan >= 20) {
                prizeCaptureTried = true;
                state.status = prizeNative::report.stage == PrizeStage::Model
                                   ? "Vehicle assets unavailable BACKSPACE returns to cars"
                                   : "Tuning assets unavailable BACKSPACE returns to cars";
                log(("Secret unlock timed out: " + prizeProgress).c_str());
                state.cue = Cue::Error;
            }
            return;
        }
        if (prizePlayed) {
            prizeNative::release();
            audio.endPrize(true);
            prizePlayed = false;
            prizeMesh.reset();
            prizeSerial = 0;
        }
        const auto selectedEntry = state.appearanceEntry(entries[state.selected]);
        state.ui.menu.setVehicleKind(selectedEntry.kind, selectedEntry.classId);
        if (extrasModel != selectedEntry.id) {
            nativeExtras::clear();
            state.ui.menu.setExtras({}, {}, {});
            state.ui.menu.extraChoice = {{0, 0, 0}};
            extrasModel = -1;
        }
        if (extrasModel < 0 && gta::modelReady(selectedEntry.id)) {
            nativeExtras::scan(selectedEntry);
            state.ui.menu.setExtras(nativeExtras::labels(0), nativeExtras::labels(1), nativeExtras::labels(2));
            if (const auto found = selectedExtras.find(selectedEntry.id); found != selectedExtras.end()) {
                for (int group = 0; group < 3; ++group) {
                    if (nativeExtras::select(group, found->second[size_t(group)])) {
                        state.ui.menu.extraChoice[size_t(group)] = found->second[size_t(group)];
                    }
                }
            }
            if (selectedExtras.find(selectedEntry.id) == selectedExtras.end()) {
                state.extrasResetPending = true;
            }
            extrasModel = selectedEntry.id;
            if (nativeExtras::available) {
                cache.erase(selectedEntry.id);
            }
        }
        if (state.appearanceResetPending || state.extrasResetPending) {
            syncStudioExtraPreview();
        }
        if (state.page != Page::Extras) {
            const auto stockPreview = cache.find(selectedEntry.id);
            liveryNative::update(state, selectedEntry, stockPreview == cache.end() ? nullptr : stockPreview->second.mesh.get());
        }
        if (state.postSelectionCinematic) {
            const auto livery = selectedLivery();
            const int previousCustomizationModel = customizationNative::state.model;
            const bool ready =
                customizationNative::update(selectedEntry, state.paint, livery.first, livery.second, state.shop.washed);
            if (diagnostics.enabled.load(std::memory_order_relaxed) &&
                previousCustomizationModel != customizationNative::state.model) {
                size_t wheels = 0, components = 0;
                for (const auto& group : customizationNative::state.groups) {
                    const size_t options = group.options.empty() ? 0 : group.options.size() - 1;
                    (group.slot == 34 ? wheels : components) += options;
                }
                char detail[192];
                std::snprintf(detail, sizeof(detail),
                              "Customization catalogue: model=%d groups=%zu wheel options=%zu component options=%zu",
                              customizationNative::state.model, customizationNative::state.groups.size(), wheels,
                              components);
                log(detail);
            }
            const bool customized = customizationNative::state.hasSelection();
            state.appearanceReady = ready || !customized;
            state.ui.menu.setCustomization(customizationNative::state, canCustomizeSelected());
            if (!ready && customized) {
                state.status = customizationNative::captureRetry.exhausted()
                                   ? "Customization preview unavailable Change the part or choose Stock"
                                   : "Preparing customization preview...";
            } else if (state.status == "Preparing customization preview..." ||
                       state.status == "Customization preview unavailable Change the part or choose Stock") {
                state.status.clear();
            }
        } else {
            state.appearanceReady = true;
            state.shop = {};
            customizationNative::clear();
            state.ui.menu.setCustomization(customizationNative::state, false);
        }
        const Carousel layout(float(outputWidth), float(outputHeight), state, entries.size());
        const int start = layout.first, end = layout.last;
        std::vector<int> want{entries[state.selected].id};
        for (int i = start; !state.ui.cinematic.active && i < end; ++i) {
            if (entries[i].id != want[0]) {
                want.push_back(entries[i].id);
            }
        }
        for (int i = std::max(0, start - 2); !state.ui.cinematic.active && i < std::min(int(entries.size()), end + 2);
             ++i) {
            if (std::find(want.begin(), want.end(), entries[i].id) == want.end()) {
                want.push_back(entries[i].id);
            }
        }
        for (auto it = pending.begin(); it != pending.end();) {
            if (std::find(want.begin(), want.end(), it->first) == want.end()) {
                it = pending.erase(it);
            } else {
                ++it;
            }
        }
        const auto tick = GetTickCount64();
        const bool selectedPending = !gta::modelReady(want.front());
        bool extracted = false;
        for (int id : want) {
            if (selectedPending && id != want.front()) {
                continue;
            }
            auto it = cache.find(id);
            const bool refreshExtras = id == want.front() && id == dirtyExtraModel;
            if (it != cache.end() && (id != want.front() || (it->second.mesh->textureLimit == 0 && !refreshExtras))) {
                it->second.touched = state.clock;
                continue;
            }
            if (std::find(failed.begin(), failed.end(), id) != failed.end()) {
                continue;
            }
            if (!gta::modelReady(id)) {
                auto pendingIt = pending.emplace(id, tick).first;
                if (tick - pendingIt->second > 20000) {
                    failed.push_back(id);
                    pending.erase(id);
                    char b[160];
                    std::snprintf(b, sizeof(b), "Preview streaming timed out for model %d after 20 seconds", id);
                    log(b);
                } else {
                    game::queue(id);
                }
                continue;
            }
            if (extracted) {
                continue;
            }
            extracted = true;
            pending.erase(id);
            if (diagnostics.enabled.load(std::memory_order_relaxed)) {
                char capture[160];
                std::snprintf(capture, sizeof(capture), "Preview capture begin: model=%d, cache=%zu, pending=%zu",
                              id, cache.size(), pending.size());
                log(capture);
            }
            const bool selected = id == entries[state.selected].id;
            if (refreshExtras) { dirtyExtraModel = -1; }
            auto mesh = selected ? nativeExtras::capture(id) : nativePreview::snapshot(id, nullptr, false, 512);
            static std::set<int> wheelReports;
            if (nativePreview::lastModel == id && wheelReports.size() < 1000 && wheelReports.insert(id).second &&
                (nativePreview::lastSources || id == 554)) {
                char report[256];
                std::snprintf(
                    report, sizeof(report),
                    "VehFuncs preview model %d: wheel source groups %d -> %d; wheel matrix helper=%d; verified "
                    "frame visibility=%d; native captures=%llu",
                    id, nativePreview::lastSources, nativePreview::lastRemaining, nativePreview::wheelMatrices,
                    nativePreview::visibility.verified, static_cast<unsigned long long>(nativePreview::prepared));
                log(report);
            }
            static bool nativeFailureLogged = false;
            if (nativePreview::failedPath && !nativeFailureLogged) {
                nativeFailureLogged = true;
                log("Native vehicle capture unavailable; later captures remain enabled");
            }
            if (mesh) {
                mesh->textureLimit = selected ? 0 : 512;
                if (diagnostics.enabled) {
                    char textures[260];
                    std::snprintf(textures, sizeof(textures),
                                  "Preview textures model=%d declared=%zu missing=%zu unsupported=%zu "
                                  "nativeOnlyAtomics=%zu privateTextureBytes=%zu prepared=%d IVF=%d",
                                  id, mesh->declaredTextures, mesh->missingTextures, mesh->unsupportedTextures,
                                  mesh->nativeOnly, mesh->textureBytes, nativePreview::lastModel == id,
                                  GetModuleHandleW(L"ImVehFt.asi") != nullptr ||
                                      GetModuleHandleW(L"IVF.asi") != nullptr);
                    log(textures);
                }
                cache.insert_or_assign(id, Cached{std::move(mesh), state.clock});
            } else {
                if (it != cache.end()) {
                    it->second.mesh->textureLimit = 0;
                    log("Full preview capture unavailable; retaining the valid lower-resolution texture snapshot");
                    continue;
                }
                failed.push_back(id);
                char b[512];
                std::snprintf(b, sizeof(b),
                              "Preview unavailable for loaded model %d: %s", id,
                              nativePreview::captureFailure.empty() ? "capture returned no mesh" :
                                  nativePreview::captureFailure.c_str());
                log(b);
            }
        }
        // Resolve the livery against the new extra's geometry before exposing its materials to the renderer
        if (state.page == Page::Extras) {
            const auto stockPreview = cache.find(selectedEntry.id);
            liveryNative::update(state, selectedEntry, stockPreview == cache.end() ? nullptr : stockPreview->second.mesh.get());
        }
        if (state.page == Page::Extras && !extrasFraming) {
            const auto current = cache.find(selectedEntry.id);
            if (liveryNative::preview && lookup(selectedEntry.id) == liveryNative::preview.get()) {
                extrasFraming = liveryNative::preview;
            } else if (current != cache.end()) {
                extrasFraming = current->second.mesh;
            }
            if (extrasFraming) { extrasFramingModel = selectedEntry.id; }
        }
        const bool extrasScanned = extrasModel == selectedEntry.id && nativeExtras::metadataReady;
        if (extrasScanned) {
            if (state.selectionAdvancePending) {
                nativeExtras::probeSelection(tick);
            } else if (state.ui.menu.selectionExtras && cache.count(selectedEntry.id)) {
                nativeExtras::pump(tick);
            }
            state.ui.menu.setExtras(nativeExtras::labels(0), nativeExtras::labels(1), nativeExtras::labels(2));
        }
        state.extrasResolved = extrasScanned ? !nativeExtras::selectionPending() && !state.extrasResetPending
            : std::find(failed.begin(), failed.end(), selectedEntry.id) != failed.end();
        if (state.selectionAdvancePending && state.extrasResolved) {
            state.reviewSelectedAppearance();
        }
        size_t textureBytes = 0;
        for (const auto& item : cache) {
            textureBytes += item.second.mesh->textureBytes;
        }
        while (cache.size() > 24 || textureBytes > 192u * 1024 * 1024) {
            auto old = cache.end();
            for (auto it = cache.begin(); it != cache.end(); ++it) {
                if (it->first != entries[state.selected].id &&
                    (old == cache.end() || it->second.touched < old->second.touched)) {
                    old = it;
                }
            }
            if (old == cache.end()) {
                break;
            }
            textureBytes -= old->second.mesh->textureBytes;
            cache.erase(old);
        }
        if (pause.owned) {
            game::pumpStreaming();
        }
    }

    Key openKey, left, right, up, down, enter, backspace, back, favorite, addFavorite, mouseLeft, mouseRight, searchKey, informationKey;

    void blockOpenInput()
    {
        openKey.blockUntilRelease();
    }

    bool key(int v)
    {
        return (GetAsyncKeyState(v) & 0x8000) != 0 || nativeKeyDown(reinterpret_cast<void*>(0xB73190), v);
    }

    game::OpenState openState(bool focus)
    {
        return game::openState(focus);
    }

    void logOpenState(const char* event, bool focus)
    {
        const auto g = openState(focus);
        char b[480];
        std::snprintf(
            b, sizeof(b),
            "%s: key=%d ctrl=%d focus=%d gameState=%d loadingData=%d startLoadRequest=%d menu=%d codePause=%d "
            "userPause=%d alive=%d cutscene=%d enex=%d catalogue=%zu padFrames=%llu frontendRead=%d rawMenu=%u "
            "rawLoading=%u inVehicle=%d",
            event, hotkey, ctrl, g.focus, g.gameState, g.loadingData, g.frontend.startLoadRequested(), g.menu,
            g.codePause, g.userPause, g.alive, g.cutscene, g.enex, catalog.size(),
            static_cast<unsigned long long>(frameSerial), g.frontend.valid, unsigned(g.frontend.menuByte),
            unsigned(g.frontend.loadingByte), g.inVehicle);
        log(b);
    }

    HWND gameWindow()
    {
        auto* d = game::device();
        D3DDEVICE_CREATION_PARAMETERS cp{};
        if (d && SUCCEEDED(d->GetCreationParameters(&cp)) && IsWindow(cp.hFocusWindow)) {
            return cp.hFocusWindow;
        }
        return nullptr;
    }

    bool focused()
    {
        DWORD pid = 0;
        HWND window = GetForegroundWindow();
        return window && GetWindowThreadProcessId(window, &pid) && pid == GetCurrentProcessId();
    }

    void blockActions()
    {
        state.photo.dragging = state.photo.movingPanel = false;
        searchInput.quarantine();
        state.ui.keyboard.characterSetKey.blockUntilRelease();
        searchKey.blockUntilRelease();
        cinematicPanels.blockUntilRelease();
        shopResetKey.blockUntilRelease();
        state.dragging = false;
        for (Key* k :
             {&left, &right, &up, &down, &enter, &backspace, &back, &favorite, &addFavorite, &mouseLeft, &mouseRight}) {
            k->blockUntilRelease();
        }
    }

    void reloadAll()
    {
        if (state.open) {
            return;
        }
        nativeExtras::resetCatalogues();
        extrasModel = -1;
        selectedExtras.clear();
        extrasFraming.reset();
        extrasFramingModel = -1;
        dirtyExtraModel = -1;
        cache.clear();
        failed.clear();
        if (state.page == Page::Prize) {
            prizeNative::release();
            state.fade.reset();
            state.spawn = false;
            state.leavePrize();
        }

        WritePrivateProfileStringW(nullptr, nullptr, nullptr, ini.c_str());
        const auto keys = readHotkey(ini);
        hotkey = keys.vk;
        ctrl = keys.ctrl;
        openKey.blockUntilRelease();
        auto savedPreferences = state.ui.preferences();
        if (loadStudioPreferences(std::filesystem::path(base) / L"GT4Showroom/settings.ini", savedPreferences)) {
            state.ui.applyPreferences(savedPreferences);
            mainStyleChoice=savedPreferences.showroomStyle;
        }
        readGalleryAccess();
        galleryOptions = readGallery(ini.c_str(), gallery);
        state.allowRealBig = gallery == Gallery::Main || gallery == Gallery::Wang;
        state.ui.menu.categoriesEnabled = gallery != Gallery::Grotti;
        state.ui.dealerBackground = gallery == Gallery::Main ? -1 : galleryBackground(ini.c_str(), gallery);
        state.ui.showroomStyle = galleryStyle(ini.c_str(), gallery, mainStyleChoice);
        state.ui.menu.musicChoice = galleryOptions.music;
        state.ui.mainDefaultBackground = galleryBackground(ini.c_str(), Gallery::Main);
        studioConfig = readStudioConfig(ini);
        studioConfig.sort = menuSort(state.ui.sortMode);
        diagnostics.reload(ini, L"GT4Showroom");
        syncCaptureDiagnostics();
        diagnostics.begin(logFile);
        localization.load(std::filesystem::path(base) / L"GT4Showroom/languages");
        localization.select(state.ui.language);
        state.ui.language = localization.selected;
        reloadSettings();
        for (auto& e : catalog) {
            void* mi = gta::modelInfo(e.id);
            if (!game::vehicleInfo(mi)) {
                continue;
            }
            const bool favorite = e.favorite;
            e = game::entry(e.id, mi);
            e.favorite = favorite;
            metadata(e);
        }
        entries.clear();
        state.selected = 0;
        reloadExclusions();
        refreshSelected();
        renderer.setLogoRoot(std::filesystem::path(base) / L"GT4Showroom/logos");
        game::notification(showroomReloadMessage);
        char message[160];
        std::snprintf(message, sizeof(message),
                      "GTSPAWN settings reloaded: key=%d ctrl=%d active menu order=%d descending=%d Spawn=%d; selected "
                      "model reset to first in saved order",
                      hotkey, ctrl, int(studioConfig.sort.by), studioConfig.sort.descending,
                      static_cast<int>(studioConfig.spawn));
        log(message);
        if (keys.invalid || studioConfig.invalidSort) {
            log("Invalid opening key used its documented fallback");
        }
    }

#include "SecretFeaturesSchool.inl"
    uint64_t worldGeneration = 0;

    uint64_t secretFeaturesWorldGeneration()
    {
        return worldGeneration;
    }

    bool secretFeaturesRecoverySceneReady();
    void secretFeaturesReleaseGraphics();
#include "SecretFeaturesRecovery.inl"

    bool secretFeaturesRecoverySceneReady()
    {
        void* p = game::player();
        return !game::loading() && p && game::field<float>(p, 0x540) > 0 &&
               worldGeneration > secretRetryFade.resumeWorld;
    }

    void secretFeaturesReleaseGraphics()
    {
        secretRecoveryView.reset();
        secretSchoolView.reset();
        liveryNative::clear(state);
        prizeMesh.reset();
        extrasFraming.reset();
        extrasFramingModel = -1;
        dirtyExtraModel = -1;
        cache.clear();
        failed.clear();
        pending.clear();
        renderer.reset();
    }

    bool world2DComplete = false;
    using Void = void(__cdecl*)();
    Void originalProcess = nullptr, originalReset = nullptr;
    using EndCamera = void*(__cdecl*)(void*);
    EndCamera originalRender = nullptr, originalShowRaster = nullptr;
    using EntityRender = void(__thiscall*)(void*);
    EntityRender originalEntityRender = nullptr;

    void __fastcall entityRender(void* self, void*)
    {
        if (!nativePreview::onEntityRender(self)) {
            originalEntityRender(self);
        }
    }

    using Frontend = void(__thiscall*)(void*);
    Frontend originalFrontend = nullptr;
    using PadUpdate = void(__thiscall*)(void*, int);
    PadUpdate originalPadUpdate = nullptr;

    void __fastcall padUpdate(void* self, void*, int index)
    {
        bool claimView = false;
        if (index == 0) {
            ++frameSerial;
            const auto tick = GetTickCount64();
            joystick.capture(static_cast<uint8_t*>(self) + 0xA8);
            sampledPad = controller.poll(joystick, tick);
            const bool focus = focused();
            if (pauseButton.poll(sampledPad.pause, tick) &&
                (state.open || spawnEntry.playback.active || secretSchool.script.state.active ||
                 secretRecovery.view.active || secretRetryFade.active()) &&
                focus && !game::loading()) {
                controllerPauseRequested = true;
            }
            claimView = sampledPad.view && (state.open || openIntent.pending(tick) || openState(focus).ready());
        }
        originalPadUpdate(self, index);

        if (index == 0 && claimView) {
            game::field<int16_t>(self, 0x1A) = 0;
        }
    }

#include "StudioExtras.inl"

    void saveStudioPreferences()
    {
        if (gallery != Gallery::Main) return;
        auto preferences=state.ui.preferences();
        preferences.showroomStyle=mainStyleChoice;
        if (!saveStudioPreferences(std::filesystem::path(base) / L"GT4Showroom/settings.ini", preferences)) {
            log("Studio preferences could not be saved; current session settings remain active");
        }
        if (diagnostics.enabled.load(std::memory_order_relaxed)) {
            char detail[320];
            std::snprintf(detail, sizeof(detail),
                "Settings: language=%s format=%d sort=%d countries=0x%lX classes=0x%lX AA=%d AO=%d floor=%d vehicle=%d",
                Localization::folders[size_t(std::clamp(state.ui.language, 0, 6))], state.ui.unitFormat,
                state.ui.sortMode, static_cast<unsigned long>(state.ui.countryMask),
                static_cast<unsigned long>(state.ui.classMask), state.ui.antialiasing, state.ui.ambientOcclusion,
                state.ui.floorReflections(), state.ui.vehicleReflections);
            log(detail);
        }
    }

#include "StudioControls.inl"

    void logAudioFaults()
    {
        if (!diagnostics.enabled.load(std::memory_order_relaxed)) {
            return;
        }
        while (const auto* fault = audio.nextFault()) {
            char report[192];
            std::snprintf(report, sizeof(report), "Audio: stage=%s HRESULT=0x%08lX decoder=%s file=", fault->stage,
                          static_cast<unsigned long>(fault->code), pcmReadErrorName(fault->decoder));
            log((std::string(report) + utf8Text(fault->file)).c_str());
        }
    }

    void updateStudio()
    {
        if (updatedFrame == frameSerial) {
            return;
        }
        updatedFrame = frameSerial;
        logAudioFaults();
        const uint64_t tick = GetTickCount64();
        const double dt = prev && tick - prev <= 250 ? double(tick - prev) / 1000 : 0;
        prev = tick;
        const bool focus = focused();
        if (exitButtons.mask) {
            if (void* p = game::pad()) {
                exitButtons.filter(static_cast<int16_t*>(p));
            }
        }
        if (spawnEntry.playback.active) {
            const bool paused = !focus || game::menu() || game::codePaused() || game::userPaused();
            if (updateEntryPresentation(spawnEntry, returnFade, dt, paused, key(VK_BACK) || sampledPad.back)) {
                audio.focus(!paused);
                syncGameAudioLevels();
                audio.update();
                return;
            }
            char detail[240];
            std::snprintf(detail, sizeof(detail),
                          "SpawnInside ended: %s at %.2f seconds; camera owner %u, native moving-camera updates %u",
                          spawnEntry.lastStop, spawnEntry.playback.age,
                          game::field<uint32_t>(spawnEntry.camera(), 0xB0), spawnEntry.nativeFrames);
            log(detail);
            if (void* p = game::pad()) {
                exitButtons.arm(static_cast<int16_t*>(p));
            }
            blockOpenInput();
            blockActions();
        }
        if (updateSecretRecovery(tick, dt, focus) || updateSecretSchool(tick, dt, focus)) {
            syncGameAudioLevels();
            audio.update();
            return;
        }
        const auto pad = sampledPad;
        const auto gate = openState(focus);
        const bool blocked = gate.blocked() || (gate.userPause && !pause.owned);
        if (!state.open && returnFade.active()) {
            if (game::loading() || !gate.alive) {
                returnFade.reset();
            } else if (focus && !gate.menu && !gate.codePause && !gate.userPause) {
                if (dealershipCameraVehicle >= 0) {
                    void* target = entryNative::function<void*(__cdecl*)(int)>(0x54FFF0)(dealershipCameraVehicle);
                    dealershipCameraWait += dt;
                    const bool focusedOnVehicle = target && game::field<void*>(spawnEntry.camera(),0x958)==target;
                    if (!target || (focusedOnVehicle && dealershipCameraWait>=.12) || dealershipCameraWait>=1.5)
                        dealershipCameraVehicle=-1;
                }
                returnFade.step(dt);
            }
        }
        const bool reloadAllowed =
            showroomReloadAllowed(state.open, state.searching, focus, gate.menu, gate.codePause, gate.gameState,
                                  gate.loadingData, gate.cutscene, key(VK_CONTROL) || key(VK_MENU));
        const bool typedReloadRequest = typedReload.poll(
            [](int v) {
                return key(v);
            },
            tick, reloadAllowed);
        const bool padReloadRequest =
            reloadChord.poll(controllerReloadRequested(state.open, pad.view, pad.search), tick);
        if (!state.busy() && !state.plateOpen && reloadAllowed && (typedReloadRequest || padReloadRequest)) {
            reloadAll();
            blockOpenInput();
            return;
        }
        if (statusUntil && tick >= statusUntil) {
            state.status.clear();
            statusUntil = 0;
        }
        if (!state.open) updateGalleryMarkers(gate,key(VK_RETURN) || pad.accept,tick,dt);
        else if (arrivalTime >= 0 && focus && !game::menu()) {
            arrivalTime += dt;
            if (arrivalTime >= 2 * galleryHalfFade) arrivalTime=-1;
        }
        const bool physicalOpen = studioOpenInput(key(hotkey), state.open, pad);
        const bool pressed =
            openKey.poll(physicalOpen && (!state.searching || (hotkey >= VK_F1 && hotkey <= VK_F24)), tick);
        if (blocked) {
            blockActions();
            openIntent.clear();
        }
        static bool first = true, readyLogged = false, catalogueLogged = false;
        if (first) {
            first = false;
            logOpenState("First studio update", focus);
        }
        if (gate.gameState == 9 && !gate.loadingData && gate.alive &&
            (state.open || !catalogueWarm || openIntent.pending(tick))) {
            discover();
            if (!catalogueLogged && !catalog.empty()) {
                catalogueLogged = true;
                logOpenState("Vehicle catalogue available", focus);
            }
        }
        if (gate.ready() && !readyLogged) {
            readyLogged = true;
            logOpenState("Opening input ready", focus);
        }
        if (pressed && arrivalTime < 0) {
            logOpenState(blocked ? "Open key rejected while suspended" : "Open key detected", focus);
            if (!blocked) {
                if (state.open) {
                    state.exit();
                    if(state.cue==Cue::Back) { audio.cue(Cue::Back, gallery==Gallery::Main);state.cue=Cue::None; }
                } else {
                    openIntent.request(tick);
                }
            }
        }
        if (!state.open && !returnFade.active() && openIntent.pending(tick) && gate.ready() && !catalog.empty()) {
            reloadSettings();
            if (gallery != Gallery::Main) {
                state.ui.countryMask = state.ui.classMask = 0;
                state.ui.countryFilter = state.ui.classFilter = -1;
                state.ui.countryShortcut = false;
                state.ui.favoritesOnly = false;
                dealerShopVisited = false;
                selectedExtras.clear();
            }
            reloadExclusions();
            renderer.setGalleryBrand(std::filesystem::path(base)/L"GT4Showroom",int(gallery),galleries[int(gallery)].name);
            state.dealerPurchaseEnabled = galleryChargesVehicle(gallery);
            state.showDealerPrices = gallery == Gallery::Coutt || gallery == Gallery::Verdant || gallery == Gallery::Grotti;
            state.galleryFavorites = galleries[int(gallery)].fullService;
            openIntent.clear();
            if (!entries.empty()) {
                for (auto& e : entries) {
                    void* mi = gta::modelInfo(e.id);
                    if (game::vehicleInfo(mi)) {
                        e.paints = game::paints(mi);
                    }
                }
                if (gallery != Gallery::Main) {
                    state.selected = 0;
                    state.scroll = state.scrollTarget();
                }
                state.begin(entries.size());
                const bool enabled = loadTutorialEnabled(ini.c_str());
                state.tutorialsSeen = loadTutorialProgress(ini.c_str(),enabled);
                state.tutorialsEnabled = enabled;
                if (enabled && !(state.tutorialsSeen & welcomeTutorialBit)) {
                    state.informationOpen = true;
                    state.informationMotion = {state.clock};
                    state.informationChoice = 3;
                }
                refreshSelected();
                failed.clear();
                pending.clear();
                lastRenderLog = tick;
                drawTotal = drawWorst = 0;
                drawSamples = 0;
                pause.begin();
                suspended = true;
                blockActions();
                audio.chooseMusic(galleryOptions.music, gameWindow(), base + L"GT4Showroom/audio/");
                audio.setTransitionGain(gallery==Gallery::Main ? 1.f : 0.f);
                audio.start();
                logAudioFaults();
                if (diagnostics.enabled.load(std::memory_order_relaxed)) {
                    char levels[160];
                    std::snprintf(levels, sizeof(levels),
                                  "Audio buses: master=%u music=%u effects=%u; controlled by GTA SFX/radio volume.",
                                  audio.masterVolume, audio.bedVolume, audio.cueVolume);
                    log(levels);
                }
                if (audio.enabled && !audio.ready()) {
                    log("One or more audio clips could not be loaded; check asset folder / DirectSound availability.");
                }
                char b[192];
                std::snprintf(
                    b, sizeof(b),
                    "Opened studio with %zu visible / %zu registered vehicles; palette 0x%08lX; runtime scan %lu.",
                    entries.size(), catalog.size(), static_cast<unsigned long>(gta::vehiclePalette()),
                    static_cast<unsigned long>(scan));
                log(b);
            } else {
                if (gallery != Gallery::Main) close();
                log("No visible vehicle models; check [ExcludedModels] in GT4Showroom.ini");
            }
        }
        if (openIntent.until && !openIntent.pending(tick)) {
            char b[192];
            std::snprintf(b, sizeof(b),
                          "Open request expired: userPause=%d codePause=%d menu=%d loading=%d enex=%d catalogue=%zu",
                          game::userPaused(), game::codePaused(), game::menu(), game::loading(),
                          game::at<int>(0x96A7CC), catalog.size());
            log(b);
            openIntent.clear();
        }
        if (state.open) {
            if (game::loading() || !game::player() || game::field<float>(game::player(), 0x540) <= 0 ||
                gate.inVehicle || gate.cutscene || gate.externalCamera) {
                close();
            } else {
                pause.maintain();
                const bool active = focus && !game::menu() && !game::codePaused();
                if (active && !suspended) {
                    state.step(dt);
                    D3DVIEWPORT9 vp{0, 0, 1280, 720, 0, 1};
                    if (auto* d = game::device()) {
                        d->GetViewport(&vp);
                    }
                    const auto oldSelected = state.selected;
                    const auto oldPrize = state.prize.serial;
                    // the native viewport may belong to an auxiliary pass; input follows the displayed layout
                    const auto width = renderer.presentationWidth ? renderer.presentationWidth : vp.Width;
                    const auto height = renderer.presentationHeight ? renderer.presentationHeight : vp.Height;
                    if (arrivalTime < 0 && state.clock - state.opened >= MenuFade::entryDuration) {
                        interaction(pad, float(width), float(height), dt, tick);
                    } else {
                        blockActions();
                    }
                    if (state.page != Page::Prize &&
                        (!prizeNative::heldTxds.empty() || !prizeNative::heldModels.empty())) {
                        prizeNative::release();
                    }
                    if (state.selected != oldSelected || state.prize.serial != oldPrize) {
                        refreshSelected();
                    }
                    if (state.page == Page::Prize) {
                        state.paint = size_t(state.prize.paint);
                    }
                    if (state.liveryWhite && state.page == Page::Livery) {
                        gta::vehicleColor(1, state.liveryWhiteRgb);
                    }
                    syncStudioExtraPreview();
                    if (state.fade.ready(state.clock) && state.fade.action == CloseAction::Exit) {
                        close(true);
                    }
                    if (state.open && state.fade.ready(state.clock) && state.fade.action == CloseAction::Spawn) {
                        state.spawn = false;
                        std::string error;
                        const bool claimingPrize = state.page == Page::Prize;
                        const auto reward = claimingPrize
                            ? std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
                                return entry.id == state.prize.id;
                            })
                            : entries.end();
                        if (claimingPrize && reward == entries.end()) {
                            state.recovered();
                            blockActions();
                            state.status = "The reward vehicle is no longer available";
                            state.cue = Cue::Error;
                        } else {
                            const Entry spawnEntryModel = claimingPrize ? *reward : state.appearanceEntry(entries[state.selected]);
                            const Mesh* m = lookup(spawnEntryModel.id);
                            float clearance = m ? sideSpawnDistance(m->low.x, m->high.x) : 2.6f;
                            void* mi = gta::modelInfo(spawnEntryModel.id);
                            void* col = mi ? game::field<void*>(mi, 0x14) : nullptr;
                            if (col) {
                                const V3 low = game::field<V3>(col, 0), high = game::field<V3>(col, 0xC);
                                clearance = sideSpawnDistance(low.x, high.x);
                            }
                            const bool assetsReady =
                                state.page == Page::Prize ? prizeNative::stream(state.prize) : liveryNative::ready(state);
                            const auto livery = selectedLivery();
                            const bool customized =
                                !claimingPrize && state.postSelectionCinematic && customizationNative::state.hasSelection();
                            const bool customizationReady =
                                !customized ||
                                (state.appearanceReady &&
                                 customizationNative::current(spawnEntryModel, state.paint, livery.first,
                                                              livery.second, state.shop.washed) &&
                                 customizationNative::ready(livery.first, livery.second));
                            if (!assetsReady || !customizationReady) {
                                if (pause.owned) {
                                    game::pumpStreaming();
                                }
                                if (state.clock - state.fade.began > 10) {
                                    state.recovered();
                                    blockActions();
                                    state.status =
                                        state.page == Page::Prize
                                            ? "Prize tuning assets could not be loaded Try REALBIG again"
                                        : state.postSelectionCinematic
                                            ? "Customization could not be loaded Choose Stock or try again"
                                            : "Livery texture could not be loaded Choose No Livery or try again";
                                    state.cue = Cue::Error;
                                }
                            } else {
                                void* spawned = nullptr;
                                const int due = claimingPrize ? 0 : state.shop.pendingCost();
                                const bool dealershipPurchase = galleryChargesVehicle(gallery) && !claimingPrize;
                                const int vehiclePrice = dealershipPurchase ? spawnEntryModel.price : 0;
                                int* wallet = (dealershipPurchase || due > 0) ? shopWallet() : nullptr;
                                VehiclePayment payment;
                                if (vehiclePrice < 0) {
                                    error = "Vehicle purchase price is unavailable";
                                } else if (dealershipPurchase && !wallet) {
                                    error = "Player money is unavailable Please try again";
                                } else if (!allowedHere(spawnEntryModel)) {
                                    error = "Spawn unavailable for this model";
                                } else if (!payment.reserve(wallet,vehiclePrice,due)) {
                                    error = dealershipPurchase ? "Insufficient funds to purchase this vehicle and selected work"
                                                               : "Insufficient funds for selected TransFender work";
                                }
                                if (error.empty() &&
                                    (spawned = game::spawn(spawnEntryModel, state.paint, error, clearance, claimingPrize,
                                        gallery == Gallery::Main ? nullptr : dealerShopVisited ? &galleries[int(gallery)].shopSpawn : &galleries[int(gallery)].spawn,
                                        gallery == Gallery::Wang ? (dealerShopVisited ? 1.5708f : pi) : gallery == Gallery::Coutt ? .785398f : 2.51327f))) {
                                    bool installedParts = !customized;
                                    const bool prepared = game::finalizeVehicleAppearance(
                                        spawned, spawnEntryModel, state.paint,
                                        [](void* vehicle, int id) {
                                            const bool plugin =
                                                GetModuleHandleW(L"VehFuncs.asi") || GetModuleHandleW(L"ImVehFt.asi") ||
                                                GetModuleHandleW(L"IVF.asi") || GetModuleHandleW(L"ModelExtras.asi");
                                            return !plugin || nativePreview::initialize(vehicle, id, state.shop.washed);
                                        },
                                        [&](void* vehicle) {
                                            if (state.page != Page::Prize) {
                                                liveryNative::apply(vehicle, state);
                                                if (customized) {
                                                    installedParts =
                                                        customizationNative::apply(vehicle, livery.first, livery.second);
                                                    if (!installedParts) {
                                                        log("Customization application did not verify every selected "
                                                            "upgrade");
                                                    }
                                                }
                                            }
                                            if (state.page == Page::Prize) {
                                                const bool tuned = prizeNative::tune(vehicle, state.prize);
                                                char detail[260];
                                                std::snprintf(detail, sizeof(detail),
                                                              "Prize claim tuning: model=%d visual upgrades=%d/%d "
                                                              "preparation=%d; ability is independent of kit/paintjob.",
                                                              state.prize.id, prizeNative::installedVisuals,
                                                              prizeNative::expectedVisuals, tuned);
                                                log(detail);
                                                const bool applied = prizeNative::ability(vehicle, state.prize);
                                                std::snprintf(
                                                    detail, sizeof(detail),
                                                    "Prize ability: %s tracked=%d reference=%d flags=0x%08X mass=%.1f "
                                                    "turnMass=%.1f handling=%p active=%zu",
                                                    *abilityName(state.prize.ability) ? abilityName(state.prize.ability)
                                                                                      : "NONE",
                                                    applied, prizeNative::abilityBackend.reference(uintptr_t(vehicle)),
                                                    game::field<uint32_t>(vehicle, 0x40), game::field<float>(vehicle, 0x8C),
                                                    game::field<float>(vehicle, 0x90), game::field<void*>(vehicle, 0x384),
                                                    prizeNative::abilities.size());
                                                log(detail);
                                            }
                                        });
                                    if (!claimingPrize) {
                                        finishShopPurchase(spawned, installedParts);
                                    }
                                    if (!game::applySpawnPlate(spawned, state.plate)) {
                                        log("Custom plate could not be created; original vehicle plate retained");
                                    }
                                    if (!prepared) {
                                        log("Spawn native preparation did not reach the render boundary; explicit paint "
                                            "was "
                                            "applied but a later plugin override remains possible");
                                    }
                                    if (diagnostics.enabled.load(std::memory_order_relaxed)) {
                                        char paint[192];
                                        std::snprintf(
                                            paint, sizeof(paint),
                                            "Spawn appearance: model=%d choice=%zu prepared=%d colors=%u,%u,%u,%u "
                                            "palette=0x%08lX",
                                            spawnEntryModel.id, state.paint, prepared,
                                            unsigned(game::field<uint8_t>(spawned, 0x434)),
                                            unsigned(game::field<uint8_t>(spawned, 0x435)),
                                            unsigned(game::field<uint8_t>(spawned, 0x436)),
                                            unsigned(game::field<uint8_t>(spawned, 0x437)),
                                            static_cast<unsigned long>(gta::vehiclePalette()));
                                        log(paint);
                                    }
                                    if (gallery != Gallery::Main || studioConfig.spawn != SpawnMode::InFront) {
                                        dealerSeatPending = gallery != Gallery::Main;
                                        dealershipCameraPending = dealerSeatPending;
                                        V3 low = m ? m->low : V3{-1, -2.3f, -.5f}, high = m ? m->high : V3{1, 2.3f, 1.1f};
                                        if (col) {
                                            low = game::field<V3>(col, 0);
                                            high = game::field<V3>(col, 0xC);
                                        }
                                        pendingEntry = spawned;
                                        pendingEntryLow = low;
                                        pendingEntryHigh = high;
                                    }
                                    payment.commit();
                                    if (dealershipPurchase) {
                                        char receipt[240]{};
                                        std::snprintf(receipt,sizeof(receipt),
                                                      "Dealership purchase completed: dealership=%s model=%d vehiclePrice=%d balance=%d",
                                                      galleries[int(gallery)].name,spawnEntryModel.id,vehiclePrice,*wallet);
                                        log(receipt);
                                    }
                                    log("Vehicle spawned; leaving studio");
                                    if (!error.empty()) {
                                        log(error.c_str());
                                    }
                                    state.open = false;
                                } else {
                                    payment.cancel();
                                    if (dealershipPurchase) {
                                        char receipt[240]{};
                                        std::snprintf(receipt,sizeof(receipt),
                                                      "Dealership purchase rejected or rolled back: dealership=%s model=%d price=%d balance=%d",
                                                      galleries[int(gallery)].name,spawnEntryModel.id,vehiclePrice,wallet ? *wallet : -1);
                                        log(receipt);
                                    }
                                    if (error.empty()) {
                                        error = "Spawn unavailable for this model";
                                    }
                                    state.recovered();
                                    blockActions();
                                    state.status = error;
                                    log(error.c_str());
                                    if (claimingPrize && game::player()) {
                                        char gateDetails[128]{};
                                        std::snprintf(gateDetails, sizeof(gateDetails),
                                                      "Prize spawn rejected: mission=%d worldArea=%d pedArea=%u",
                                                      game::mission(), game::at<int>(0xB72914),
                                                      unsigned(game::field<uint8_t>(game::player(), 0x2F)));
                                        log(gateDetails);
                                    }
                                    state.cue = Cue::Error;
                                }
                            }
                        }
                    }
                    if (arrivalTime >= 0 || state.clock - state.opened < MenuFade::entryDuration) state.cue = Cue::None;
                    if (state.cue != Cue::None) {
                        if ((state.cue != Cue::Move && state.cue != Cue::Slider) || moveCue.allow(tick)) {
                            if (state.cue == Cue::WashSpin) {
                                audio.cue(Cue::Accept);
                            }
                            audio.cue(state.cue, gallery==Gallery::Main && state.fade.action==CloseAction::Exit);
                        }
                        state.cue = Cue::None;
                    }
                    if (!state.open && pause.owned) {
                        close(true);
                    }
                    if (pendingEntry) {
                        void* v = pendingEntry;
                        pendingEntry = nullptr;
                        if (dealerSeatPending || studioConfig.spawn == SpawnMode::DriverSeat) {
                            dealerSeatPending = false;
                            void* ped = entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1);
                            const bool seated = seatPlayerAsDriver(ped, v);
                            if (seated && returnFade.active() && dealershipCameraPending) {
                                entryNative::function<void(__thiscall*)(void*)>(0x50BAB0)(spawnEntry.camera());
                                dealershipCameraVehicle=entryNative::function<int(__cdecl*)(void*)>(0x54FFC0)(v);
                                dealershipCameraWait=0;
                            }
                            dealershipCameraPending = false;
                            log(seated ? "Spawn: player seated directly as driver"
                                                           : "Spawn: driver seat unavailable; vehicle left nearby");
                        } else {
                            const bool started = spawnEntry.begin(v, pendingEntryLow, pendingEntryHigh);
                            char detail[240];
                            std::snprintf(detail, sizeof(detail),
                                          "Spawn=3 entry: started=%d reason=%s cameraOwner=%u task=%p", started,
                                          spawnEntry.lastStop, game::field<uint32_t>(spawnEntry.camera(), 0xB0),
                                          spawnEntry.entryTask);
                            log(detail);
                        }
                    }
                } else {
                    blockActions();
                }
                suspended = !active;
                audio.focus(active);
            }
        } else {
            audio.focus(focus && !game::menu());
        }
        if (pressed && !blocked) {
            functionKey.claim(hotkey);
        }
        functionKey.apply(reinterpret_cast<void*>(0xB73190), [](int vk) {
            return key(vk);
        });
        if (exitButtons.mask) {
            if (void* p = game::pad()) {
                exitButtons.filter(static_cast<int16_t*>(p));
            }
        }
        const bool inShop = state.open && state.shop.page != ShopPage::Closed;
        if (inShop) {
            audio.setShopMusic(true, base + L"GT4Showroom/audio/");
        } else {
            audio.setShopMusic(false);
        }
        const bool keyboardEntry = gallery==Gallery::Main && state.open &&
                                   state.clock-state.opened<MenuFade::entryDuration;
        float gain = state.open && !keyboardEntry ? state.entryAudioGain() : 1.f;
        if (arrivalTime >= 0) gain *= ease(float(std::clamp((arrivalTime-galleryHalfFade)/galleryHalfFade,0.,1.)));
        audio.setTransitionGain(gain);
        syncGameAudioLevels();
        audio.update();
        logAudioFaults();
    }

    void __cdecl process()
    {
        static bool first = true;
        if (first) {
            first = false;
            log("Native game-process callback reached");
        }
        if (!game::loading()) {
            prizeNative::abilities.maintain(prizeNative::abilityBackend);
        }
        world2DComplete = false;
        originalProcess();
        if (!game::loading()) {
            prizeNative::abilities.maintain(prizeNative::abilityBackend);
        }
        updateStudio();
    }

    void __fastcall frontend(void* self, void*)
    {
        static bool first = true;
        if (first) {
            first = false;
            log("Native frontend callback reached");
        }
        {
            const bool pulse = controllerPauseRequested &&
                               (state.open || spawnEntry.playback.active || secretSchool.script.state.active ||
                                secretRecovery.view.active || secretRetryFade.active()) &&
                               focused() && !game::loading();
            controllerPauseRequested = false;
            ScopedEscapePulse escape(reinterpret_cast<void*>(0xB73190), reinterpret_cast<void*>(0xB72F20), pulse);
            originalFrontend(self);
        }
        updateStudio();
        if (state.open) {
            pause.maintain();
        }
    }

    using Render2D = void(__cdecl*)();
    Render2D originalRender2D = nullptr, originalWorldEffects = nullptr;

    void __cdecl worldEffects()
    {
        originalWorldEffects();
        if (!game::loading() && !game::menu()) {
            ++worldGeneration;
        }
        if (!game::loading() && !game::menu() && focused()) {
            if (secretRecovery.view.active) {
                secretRecoveryView.captureWorld(game::device(), frameSerial);
            }
            if (secretSchool.script.state.active) {
                secretSchoolView.captureWorld(game::device(), frameSerial);
            }
        }
    }

    uint64_t studioPreparedFrame = UINT64_MAX, studioCompositedFrame = UINT64_MAX;
    bool studioPreparing = false;

    bool studioVisible()
    {
        return state.open && !game::loading() && !game::menu() && !game::codePaused() && focused() &&
               !game::externalCamera();
    }

    bool studioSwapChain(void* camera, IDirect3DSwapChain9*& swapChain)
    {
        constexpr uintptr_t rasterExtensionOffset = 0xB4E9E0;
        const auto* raster = game::field<const uint8_t*>(camera, 0x60);
        const uint8_t* topRaster = nullptr;
        int extensionOffset = 0;
        return raster && gta::read(uintptr_t(raster), topRaster) && topRaster &&
               gta::read(rasterExtensionOffset, extensionOffset) && extensionOffset > 0 &&
               gta::read(uintptr_t(topRaster) + unsigned(extensionOffset) + 0x1C, swapChain);
    }

    void prepareStudioFrame(void* camera)
    {
        if (!camera || camera != game::at<void*>(0xC1703C) || camera != game::renderCamera() || studioPreparing ||
            studioPreparedFrame == frameSerial || !studioVisible() || auxiliaryGraphicsPass()) {
            return;
        }
        auto* device = game::device();
        IDirect3DSwapChain9* swapChain = nullptr;
        if (!device || !studioSwapChain(camera, swapChain)) {
            return;
        }
        D3DSURFACE_DESC output{};
        if (!readBackBufferSize(device, swapChain, output)) {
            return;
        }

        studioPreparing = true;
        fillCache(output.Width, output.Height);
        if (game::renderCamera() == camera) {
            if (state.dealerPurchaseEnabled) {
                const int* wallet = shopWallet();
                state.dealerBalance = wallet ? *wallet : 0;
            }
            updateShopGeometry();
        }
        if (game::renderCamera() == camera &&
            renderer.draw(device, output.Width, output.Height, state, entries, lookup, shopPart, true,
                          extrasFramingModel >= 0 && state.selected < entries.size() &&
                              entries[state.selected].id == extrasFramingModel ? extrasFraming.get() : nullptr)) {
            studioPreparedFrame = frameSerial;
            if (state.photo.active && state.photo.requested && state.clock >= state.photo.captureAt) {
                std::filesystem::path saved;
                bool success = false;
                const bool ready = state.selected < entries.size() && lookup(entries[state.selected].id);
                try {
                    if (ready) {
                        success = renderer.savePhoto(
                            device, state, std::filesystem::path(base) / L"GT4Showroom" / L"photos", saved);
                    }
                } catch (...) {
                    log("Photo could not be saved; studio state retained");
                }
                state.photo.requested = false;
                state.photo.flashBegan = success ? state.clock : -10.;
                if (success) {
                    state.photo.status = "Photo saved to GT4Showroom/photos";
                } else if (ready) {
                    state.photo.status = "Could not save photo";
                } else {
                    state.photo.status = "Wait for the vehicle preview to finish loading";
                }
                state.photo.statusUntil = state.clock + 3;
                state.cue = success ? Cue::None : Cue::Error;
            }
        }
        studioPreparing = false;
    }

    void presentStudioFrame(void* camera)
    {
        if (!camera || camera != game::at<void*>(0xC1703C) || studioPreparedFrame != frameSerial ||
            studioCompositedFrame == frameSerial || !studioVisible()) {
            return;
        }
        auto* d = game::device();
        IDirect3DSwapChain9* swapChain = nullptr;
        if (!d || !studioSwapChain(camera, swapChain)) {
            return;
        }
        constexpr uintptr_t nativeSceneActive = 0xC97C54;
        const bool sceneActive = game::at<int>(nativeSceneActive) != 0;
        if (drawToBackBuffer(d, swapChain, sceneActive, [&](UINT, UINT) {
                const bool presented = renderer.presentStudio(d, state);
                if (presented && arrivalTime >= 0) renderer.veil(d,1.f-ease(float(std::clamp((arrivalTime-galleryHalfFade)/galleryHalfFade,0.,1.))));
                return presented;
            })) {
            studioCompositedFrame = frameSerial;
            const bool tutorialVisible = state.tutorialOpen()
                ? state.tutorialMotion.reveal(state.clock) > 0.f
                : state.informationOpen && state.informationMotion.reveal(state.clock) > 0.f;
            const bool interfaceHidden = state.ui.footerHidden && state.page == Page::Cars &&
                !state.studioView.active && !state.ui.cinematic.active && !state.photo.active &&
                state.shop.page == ShopPage::Closed;
            const unsigned displayedTopics = state.tutorialOpen() ? tutorialTopics(state.tutorial)
                                                                 : welcomeTutorialBit;
            if (state.tutorialsEnabled && tutorialVisible && !interfaceHidden && state.blackAlpha() < 1.f &&
                (arrivalTime < 0 || arrivalTime > galleryHalfFade) && (displayedTopics & ~state.tutorialsSeen)) {
                const unsigned shown = state.tutorialsSeen | displayedTopics;
                if (saveTutorialProgress(ini.c_str(),state.tutorialsEnabled,shown)) {
                    state.tutorialsSeen = shown;
                } else {
                    log("Could not save displayed tutorial progress");
                }
            }
            state.presented();
            if (diagnostics.enabled.load(std::memory_order_relaxed)) {
                drawTotal += renderer.cpuMs;
                drawWorst = std::max(drawWorst, renderer.cpuMs);
                ++drawSamples;
            }
        }
        if (diagnostics.enabled.load(std::memory_order_relaxed) && GetTickCount64() - lastRenderLog >= 5000) {
            char detail[240];
            std::snprintf(
                detail, sizeof(detail),
                "Renderer: %llu draws, %llu triangles, %llu new GPU uploads; submission avg %.2f ms / max %.2f "
                "ms, %zu cached meshes, %zu pending (CPU submission only).",
                static_cast<unsigned long long>(renderer.drawCalls),
                static_cast<unsigned long long>(renderer.triangles), static_cast<unsigned long long>(renderer.uploads),
                drawSamples ? drawTotal / drawSamples : 0, drawWorst, cache.size(), pending.size());
            log(detail);
            size_t textureBytes = 0;
            for (const auto& item : cache) {
                textureBytes += item.second.mesh->textureBytes;
            }
            char resources[320];
            const auto quality = renderer.qualityResources();
            std::snprintf(resources, sizeof(resources),
                "Private resources: textures=%zu bytes, AA=%u samples, AO=%ux%u fallback=%d, environment=%u rejected=%d; language=%s format=%d models=%zu",
                textureBytes, quality[0], quality[1], quality[2], int(quality[3]),
                renderer.mainEnvironmentResolution, renderer.mainEnvironmentUnavailable,
                Localization::folders[size_t(std::clamp(state.ui.language, 0, 6))], state.ui.unitFormat, catalog.size());
            log(resources);
            if (drawWorst > 33.3) {
                log("Performance: showroom CPU submission exceeded 33.3 ms; reduce preview graphics quality or inspect model complexity");
            }
            lastRenderLog = GetTickCount64();
            drawTotal = drawWorst = 0;
            drawSamples = 0;
        }
    }

    void __cdecl render2D()
    {
        world2DComplete = false;
        originalRender2D();
        world2DComplete = true;
        if (state.open) {
            prepareStudioFrame(game::renderCamera());
        }
    }

    void* __cdecl showRaster(void* camera)
    {
        presentStudioFrame(camera);
        return originalShowRaster(camera);
    }

    void* __cdecl render(void* camera)
    {
        const auto caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
        if (world2DComplete || caller == 0x53EBFA) {
            prepareStudioFrame(camera);
        }

        if (secretFeatures.enabled && !secretRecovery.resuming && !game::loading() &&
            camera == game::at<void*>(0xC1703C) &&
            (secretRecovery.view.active || game::at<uint8_t>(0x96A8B0) == 1 || game::at<uint8_t>(0x96A8B0) == 2)) {
            renderer.ensureUiAssets(game::device());
        }
        if (world2DComplete && secretRecovery.view.active && secretRecoveryEligible() &&
            secretRecovery.matches(game::at<uint8_t>(0x96A8B0), game::at<uint32_t>(0x96A8AC)) && !game::menu() &&
            !game::codePaused() && focused() && camera == game::at<void*>(0xC1703C)) {
            if (auto* d = game::device()) {
                if (secretRecoveryView.hasWorld(frameSerial)) {
                    const bool shown = secretRecoveryView.draw(d, renderer, secretRecovery.view, frameSerial);
                    if (shown) {
                        secretRecovery.present(GetTickCount64(), true);
                    }
                }
                if (secretRecovery.pending == SecretFeaturesChoice::Exit) {
                    renderer.veil(d, failSmooth(float(secretRecovery.closing / .18)));
                }
            }
        }
        if (world2DComplete && secretSchool.script.state.active && !game::menu() && !game::codePaused() && focused() &&
            camera == game::at<void*>(0xC1703C)) {
            if (auto* d = game::device()) {
                renderer.ensureUiAssets(d);
                secretSchoolView.draw(d, renderer, secretSchool.script.state, frameSerial);
                if (secretSchoolPending != SecretFeaturesChoice::None) {
                    renderer.veil(d, failSmooth(float(secretSchoolClosing / .18)));
                }
            }
        }
        if (!state.open && arrivalTime >= 0 && !game::menu() && focused() && camera == game::at<void*>(0xC1703C)) {
            if (auto* d = game::device()) renderer.veil(d,ease(float(std::clamp(arrivalTime/galleryHalfFade,0.,1.))));
        }
        if (!state.open && returnFade.active() && !game::menu() && !game::codePaused() && focused() &&
            camera == game::at<void*>(0xC1703C)) {
            if (auto* d = game::device()) {
                renderer.veil(d, returnFade.opacity());
            }
        }
        if (secretRetryFade.active() && !game::menu() && !game::codePaused() && focused() &&
            camera == game::at<void*>(0xC1703C)) {
            if (auto* d = game::device()) {
                renderer.veil(d, secretRetryFade.opacity());
            }
        }
        return originalRender(camera);
    }

    void __cdecl reset()
    {
        removeGalleryBlips();
        studioPreparedFrame = studioCompositedFrame = UINT64_MAX;
        pendingEntry = nullptr;
        spawnEntry.stop(true);
        outcomeCamera.stop(true);
        secretRecoveryReset();
        secretSchoolReset();
        prizeNative::abilities.clear(prizeNative::abilityBackend);
        close();
        nativeExtras::resetCatalogues();
        entries.clear();
        catalog.clear();
        known.clear();
        selectedExtras.clear();
        extrasFraming.reset();
        extrasFramingModel = -1;
        dirtyExtraModel = -1;
        cache.clear();
        failed.clear();
        pending.clear();
        scan = 0;
        catalogueWarm = false;
        registryTable = 0;
        renderer.reset();
        secretSchool.script.assetsReady = false;
        audio.close();
        originalReset();
    }

    void __cdecl video()
    {
        studioPreparedFrame = studioCompositedFrame = UINT64_MAX;
        renderer.releaseStudioEnvironment();
        pendingEntry = nullptr;
        spawnEntry.stop(true);
        outcomeCamera.stop(true);
        secretRecovery.present(GetTickCount64(), false);
        secretSchool.script.assetsReady = false;
        secretRecoveryView.reset();
        secretSchoolView.reset();
        liveryNative::releaseGraphics(state);
        customizationNative::resetCapture();
        clearShopGeometry();
        state.appearanceReady = !state.postSelectionCinematic;
        renderer.reset();
        extrasFraming.reset();
        extrasFramingModel = -1;
        dirtyExtraModel = -1;
        cache.clear();
        failed.clear();
        pending.clear();
    }

    FixedCameraNative originalFixedCamera = nullptr;

    void __fastcall fixedCamera(void* self, void*, const V3& target, float a, float b, float c)
    {
        OwnedFixedPose pose;
        const OwnedFixedPose* owned = nullptr;
        if (spawnEntry.playback.active && spawnEntry.ownsCamera()) {
            pose = {spawnEntry.lastSource, spawnEntry.lastTarget, spawnEntry.lastUp, spawnEntry.desiredFov};
            owned = &pose;
        } else if (outcomeCamera.owns()) {
            pose = {outcomeCamera.source, outcomeCamera.target, {}, outcomeCamera.desiredFov};
            owned = &pose;
        }
        if (processFixedShot(originalFixedCamera, self, target, a, b, c, owned) && spawnEntry.playback.active) {
            ++spawnEntry.nativeFrames;
            if (spawnEntry.nativeFrames == 1) {
                log("SpawnInside: first owned active CCam position/target/roll/FOV update applied.");
            }
        }
    }

    using NativeShutdown = void(__cdecl*)();
    using NativeGameShutdown = bool(__cdecl*)();
    NativeShutdown originalRestartShutdown = nullptr, originalRenderWareShutdown = nullptr;
    NativeGameShutdown originalGameShutdown = nullptr;

    void clearBeforeNativeShutdown()
    {
        try {
            removeGalleryBlips();
            close();
        } catch (...) {
            log("Preview cleanup failed before engine shutdown");
        }
        try {
            nativePreview::releaseRetired();
        } catch (...) {
            log("Retired preview cleanup failed before engine shutdown");
        }
    }

    void __cdecl restartShutdown()
    {
        clearBeforeNativeShutdown();
        originalRestartShutdown();
    }

    bool __cdecl gameShutdown()
    {
        clearBeforeNativeShutdown();
        return originalGameShutdown();
    }

    void __cdecl renderWareShutdown()
    {
        clearBeforeNativeShutdown();
        originalRenderWareShutdown();
    }

    std::vector<void*> hooks;

    template <class F, class O> bool add(uintptr_t address, F fn, O& original)
    {
        uintptr_t end = 0;
        if (!qb::readableCode(address, 5)) {
            return false;
        }
        const uint8_t first = game::at<uint8_t>(address);
        if ((first == 0xE9 && !qb::validateEntryJump(address, end)) || first == 0xEB || first == 0xCC) {
            return false;
        }
        const auto r = MH_CreateHook(reinterpret_cast<void*>(address), reinterpret_cast<void*>(fn),
                                     reinterpret_cast<void**>(&original));
        if (r != MH_OK) {
            return false;
        }
        hooks.push_back(reinterpret_cast<void*>(address));
        return true;
    }

    bool nativeLifecycleBindings()
    {
        struct Binding
        {
            uintptr_t address;
            std::array<uint8_t, 8> prefix;
        };

        const std::array<Binding, 3> bindings{{{0x53C550, {{0x56, 0x57, 0x6A, 0x00, 0xE8, 0x67, 0x45, 0x1F}}},
                                               {0x53C900, {{0x56, 0x57, 0xB9, 0x40, 0x42, 0xBB, 0x00, 0xE8}}},
                                               {0x53BB80, {{0xA1, 0x20, 0x29, 0xB7, 0x00, 0x56, 0x50, 0xE8}}}}};
        for (const auto& binding : bindings) {
            std::array<uint8_t, 8> bytes{};
            if (!gta::processRead(binding.address, bytes.data(), bytes.size())) {
                return false;
            }
            if (bytes != binding.prefix) {
                uintptr_t target = 0;
                if (bytes[0] != 0xE9 || !qb::validateEntryJump(binding.address, target)) {
                    return false;
                }
            }
        }
        return true;
    }

    void installNativeLifecycle()
    {
        const size_t first = hooks.size();
        const bool ready = nativeLifecycleBindings() && add(0x53C550, restartShutdown, originalRestartShutdown) &&
                           add(0x53C900, gameShutdown, originalGameShutdown) &&
                           add(0x53BB80, renderWareShutdown, originalRenderWareShutdown);
        if (!ready) {
            while (hooks.size() > first) {
                MH_RemoveHook(hooks.back());
                hooks.pop_back();
            }
            originalRestartShutdown = originalRenderWareShutdown = nullptr;
            originalGameShutdown = nullptr;
            log("Preview lifecycle hooks unavailable");
        }
    }

    int initialize()
    {
        const auto directory = moduleDirectory(module);
        if (directory.empty()) {
            return 0;
        }
        base = (directory / L"").wstring();
        logFile = assetPath(directory / L"GT4Showroom.log").wstring();
        ini = assetPath(directory / L"GT4Showroom.ini").wstring();
        state.ui.audioLevels = readAudioLevels(std::filesystem::path(base) / L"GT4Showroom/audio.dat");
        auto preferences = state.ui.preferences();
        migrateStudioPreferences(std::filesystem::path(base) / L"GT4Showroom/settings.ini",
                                 std::filesystem::path(base) / L"GT4Showroom/settings.dat", preferences);
        state.ui.applyPreferences(preferences);
        localization.load(std::filesystem::path(base) / L"GT4Showroom/languages");
        localization.select(state.ui.language);
        state.ui.language = localization.selected;
        diagnostics.reload(ini, L"GT4Showroom");
        syncCaptureDiagnostics();
        diagnostics.begin(logFile);
        if (diagnostics.enabled.load(std::memory_order_relaxed)) {
            log(("Asset root: " + utf8Text(directory.wstring())).c_str());
        }
        if (!game::supported()) {
            log("Unsupported executable. Requires classic SA 1.0 US Hoodlum/Compact Win32. No hooks installed.");
            return 0;
        }
        HMODULE pin;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&initialize), &pin);
        auto savedPreferences = state.ui.preferences();
        if (loadStudioPreferences(std::filesystem::path(base) / L"GT4Showroom/settings.ini", savedPreferences)) {
            state.ui.applyPreferences(savedPreferences);
            mainStyleChoice=savedPreferences.showroomStyle;
        }
        readGalleryAccess();
        galleryOptions = readGallery(ini.c_str(), gallery);
        state.allowRealBig = gallery == Gallery::Main || gallery == Gallery::Wang;
        state.ui.menu.categoriesEnabled = gallery != Gallery::Grotti;
        state.ui.dealerBackground = gallery == Gallery::Main ? -1 : galleryBackground(ini.c_str(), gallery);
        state.ui.showroomStyle = galleryStyle(ini.c_str(), gallery, mainStyleChoice);
        state.ui.menu.musicChoice = galleryOptions.music;
        state.ui.mainDefaultBackground = galleryBackground(ini.c_str(), Gallery::Main);
        studioConfig = readStudioConfig(ini);
        studioConfig.sort = menuSort(state.ui.sortMode);
        const auto keys = readHotkey(ini);
        hotkey = keys.vk;
        ctrl = keys.ctrl;
        if (keys.migrated) {
            log("Legacy shipped Ctrl+G binding upgraded to F1 without modifying the INI Set OpenKey explicitly to "
                "override");
        }
        if (keys.invalid) {
            log("Invalid opening key; using F1 without Ctrl");
        }
        reloadSettings();
        reloadExclusions();
        renderer.setShopLogo(std::filesystem::path(base) / L"GT4Showroom/transfender.txd");
        renderer.setLogoRoot(std::filesystem::path(base) / L"GT4Showroom/logos");
        const std::filesystem::path fontPath = std::filesystem::path(base) / L"GT4Showroom/fonts/Michroma-Regular.ttf";
        state.random.seed(uint32_t(GetTickCount64()) ^ GetCurrentProcessId());
        if (!renderer.loadPrize((std::filesystem::path(base) / L"GT4Showroom/prize.txd").wstring())) {
            log("Static prize backdrop unavailable; check the complete packaged asset folder");
        }
        if (!renderer.loadFont(fontPath.wstring(), L"Michroma")) {
            log("Bundled font unavailable; using fallback Check the asset folder");
        } else {
            log("Bundled font loaded privately; no Windows installation needed");
        }
        if (!renderer.loadPrizeNameFont((fontPath.parent_path() / L"LiberationSans-Bold.ttf").wstring())) {
            log("Bundled bold prize-name font unavailable; using Arial Bold fallback");
        }
        if (!renderer.loadInfoFont((fontPath.parent_path() / L"LiberationSans-Regular.ttf").wstring())) {
            log("Bundled regular information font unavailable; using the existing settings font");
        }
        if (diagnostics.enabled.load(std::memory_order_relaxed)) {
            log(("Main font: " + renderer.fontReport()).c_str());
            log(("Prize font: " + renderer.fontReport(true)).c_str());
        }
        secretSchool.script.assetsReady = false;
        if (!modding::videoMemoryBinding()) {
            log("Unsupported video-memory continuation; no hooks installed");
            return 0;
        }
        modding::moddingVideoMemoryCleanup = video;
        modding::InstallLock lock;
        if (!lock.acquired || MH_Initialize() != MH_OK) {
            log("Hook initialization unavailable");
            return 0;
        }
        constexpr uintptr_t showRasterAddress = 0x745240;
        constexpr std::array<uint8_t, 9> showRasterEntry{0xA0, 0x94, 0x67, 0xBA, 0, 0x84, 0xC0, 0x75, 0x0D};
        const bool showRasterReady = qb::readableCode(showRasterAddress, showRasterEntry.size()) &&
                                     (game::at<uint8_t>(showRasterAddress) == 0xE9 ||
                                      std::memcmp(reinterpret_cast<const void*>(showRasterAddress),
                                                  showRasterEntry.data(), showRasterEntry.size()) == 0);
        bool ok = showRasterReady &&
                  add(0x5400D0,galleryEnterVehicle,originalGalleryEnterVehicle) &&
                  add(0x540120,galleryEnterVehicleJustDown,originalGalleryEnterVehicleJustDown) &&
                  add(0x5D5860,saveRadarWithoutDealers,originalRadarSave) &&
                  add(0x586D60,drawGalleryCoord,originalCoordBlip) && add(0x585FF0,drawGallerySprite,originalRadarSprite) && add(showRasterAddress, showRaster, originalShowRaster) &&
                  add(0x53BEE0, process, originalProcess) && add(0x541C40, padUpdate, originalPadUpdate) &&
                  add(0x7EE180, render, originalRender) && add(0x53E230, render2D, originalRender2D) &&
                  add(0x53E170, worldEffects, originalWorldEffects) && add(0x57B440, frontend, originalFrontend) &&
                  add(0x53BCF0, reset, originalReset) &&
                  add(modding::videoMemoryHookAddress, modding::moddingVideoMemoryBridge,
                      modding::moddingVideoMemoryResume) &&
                  add(0x534310, entityRender, originalEntityRender) &&
                  add(0x51D470, fixedCamera, originalFixedCamera) && add(0x465E60, school0, originalSchool0) &&
                  add(0x469390, school2, originalSchool2) && add(0x481300, school8, originalSchool8) &&
                  add(0x483BD0, school9, originalSchool9) && add(0x48A320, school11, originalSchool11) &&
                  add(0x4762D0, school23, originalSchool23) && add(0x58FAE0, secretFeaturesHudDraw, originalHudDraw);
        if (ok) {
            ok = add(0x58CA50, secretFeaturesNativeText, originalSecretFeaturesText) &&
                 add(0x442AD0, secretFeaturesRecoveryUpdate, originalSecretFeaturesRecovery);
            secretRecoveryAvailable = ok;
        }
        if (ok) {
            installNativeLifecycle();
            for (auto* h : hooks) {
                if (MH_QueueEnableHook(h) != MH_OK) {
                    ok = false;
                }
            }
            if (ok && MH_ApplyQueued() != MH_OK) {
                ok = false;
            }
        }
        if (!ok) {
            for (auto* h : hooks) {
                MH_DisableHook(h);
                MH_RemoveHook(h);
            }
            hooks.clear();
            log("Hook transaction refused; owned hooks rolled back");
            return 0;
        }
        initialized = true;
        if (diagnostics.enabled.load(std::memory_order_relaxed)) {
            char detail[256];
            std::snprintf(detail, sizeof(detail),
                          "GT4Showroom 3.0 initialized: language=%s format=%d sort=%d; VehFuncs=%d ProperShaders=%d; "
                          "private studio renderer, EnableLog defaults off",
                          Localization::folders[size_t(std::clamp(state.ui.language, 0, 6))], state.ui.unitFormat,
                          state.ui.sortMode, GetModuleHandleW(L"VehFuncs.asi") != nullptr,
                          GetModuleHandleW(L"ProperShaders.asi") != nullptr);
            log(detail);
        }
        return 1;
    }

    BOOL CALLBACK once(PINIT_ONCE, PVOID, PVOID*)
    {
        result = initialize();
        return TRUE;
    }

    DWORD WINAPI worker(LPVOID)
    {
        InitOnceExecuteOnce(&init, once, nullptr, nullptr);
        return 0;
    }

    void schedule()
    {
        if (scheduled.exchange(true)) {
            return;
        }
        HANDLE t = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
        if (t) {
            CloseHandle(t);
        } else {
            scheduled = false;
        }
    }
} // namespace gt4

extern "C" __declspec(dllexport) void __stdcall InitializeASI()
{
    gt4::schedule();
}

extern "C" __declspec(dllexport) int __stdcall GT4_Init()
{
    InitOnceExecuteOnce(&gt4::init, gt4::once, nullptr, nullptr);
    return gt4::result;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        gt4::module = h;
        DisableThreadLibraryCalls(h);
        gt4::schedule();
    }
    return TRUE;
}
