inline Key previousCinematicHotspot, nextCinematicHotspot, pauseCinematic, photoHide, photoReset;

void finishSettings()
{
    auto& ui = state.ui;
    if (!ui.menuSnapshot || ui.menu.open) {
        return;
    }
    if (ui.menu.closeResult < 0 && ui.dealerBackgroundWritten && gallery != Gallery::Main) {
        const wchar_t* original = ui.menuDealerRaw.empty() ? nullptr : ui.menuDealerRaw.c_str();
        if (!WritePrivateProfileStringW(galleries[int(gallery)].section, L"Background", original, ini.c_str())) {
            log("Could not restore dealership background after Cancel");
        }
        ui.dealerBackground = ui.menuDealerBackground;
    }
    if (ui.menu.closeResult >= 0 && ui.menuStyleEdited && gallery==Gallery::Main) {
        mainStyleChoice=ui.showroomStyle;
        ui.showroomStyle=galleryStyle(ini.c_str(),gallery,mainStyleChoice);
        saveStudioPreferences();
    }
    if (ui.menu.closeResult >= 0 && ui.menuStyleEdited && gallery != Gallery::Main) {
        if (!WritePrivateProfileStringW(galleries[int(gallery)].section,L"StyleOverride",
                ui.showroomStyle ? L"Prologue" : nullptr,ini.c_str())) log("Could not save dealership style override");
        ui.showroomStyle = galleryStyle(ini.c_str(),gallery,0);
    }
    const auto styleMusicPath=(std::filesystem::path(base)/L"GT4Showroom/settings.ini").wstring();
    const int prologueTrack=validatedMusicTrack(int(GetPrivateProfileIntW(L"Main",L"PrologueMusic",9,styleMusicPath.c_str())));
    if(ui.menu.closeResult<0) {
        ui.menu.musicChoice=ui.menuPreferences.showroomStyle?prologueTrack:galleryOptions.music;
        audio.chooseMusic(ui.menu.musicChoice,gameWindow(),base+L"GT4Showroom/audio/");
    } else if(ui.showroomStyle) {
        WritePrivateProfileStringW(L"Main",L"PrologueMusic",std::to_wstring(ui.menu.musicChoice).c_str(),styleMusicPath.c_str());
    } else if(ui.menu.musicChoice!=galleryOptions.music) {
        const auto musicPath=gallery==Gallery::Main?styleMusicPath:ini;
        if(WritePrivateProfileStringW(gallery==Gallery::Main?L"Main":galleries[int(gallery)].section,L"Music",std::to_wstring(ui.menu.musicChoice).c_str(),musicPath.c_str()))galleryOptions.music=ui.menu.musicChoice;
    }
    if(ui.menu.closeResult<0 && ui.dealershipLogo!=ui.menuDealershipLogo) {
        ui.dealershipLogo=ui.menuDealershipLogo;
        saveDealershipLogoSetting(ui.dealershipLogo);
    }
    if (ui.menu.closeResult < 0) {
        ui.cinematic = ui.menuCinematic;
        ui.vehicle = ui.menuVehicle;
        const bool changed = ui.preferences() != ui.menuPreferences;
        ui.applyPreferences(ui.menuPreferences);
        localization.select(ui.language);countryLanguage=ui.language;
        if (changed) {
            saveStudioPreferences();
        }
        rebuildCatalogueView();
        refreshSelected();
        if (ui.menu.volume != ui.menuVolume) {
            setGameVolume(ui.menuVolume);
            ui.menu.volume = ui.menuVolume;
        }
        state.inspection=state.menuInspection;
        state.studioView = state.menuStudioView;
        state.studioView.lastActivity = state.clock;
    } else if (ui.preferences() != ui.menuPreferences) {
        saveStudioPreferences();
    }
    if (gallery != Gallery::Main && ui.menu.closeResult >= 0 && ui.presets != ui.menuPreferences.presets) {
        auto saved = mainPreferences;
        const auto path = std::filesystem::path(base) / L"GT4Showroom/settings.ini";
        loadStudioPreferences(path, saved);
        for (size_t i = 0; i < ui.presets.size(); ++i) {
            if (!(ui.presets[i] == ui.menuPreferences.presets[i])) saved.presets[i] = ui.presets[i];
        }
        if (saveStudioPreferences(path, saved)) mainPreferences.presets = saved.presets;
    }
    ui.menuSnapshot = false;
}

#include "TransFenderControls.inl"

void openStudioSettings(float x, float y, bool keyboard)
{
    auto& ui = state.ui;
    if (ui.favoritesOnly && !state.hasAvailableFavorites) { return; }
    if (state.selected < entries.size()) {
        const auto& entry = entries[state.selected];
        ui.menu.setVehicleKind(entry.kind, entry.classId);
    }
    ui.menuDealershipLogo=ui.dealershipLogo;
    state.menuInspection=state.inspection;
    state.menuStudioView = state.studioView;
    ui.menuCinematic = ui.cinematic;
    ui.menuVehicle = ui.vehicle;
    ui.dealerBackgroundWritten = false;
    ui.menuDealerBackground = ui.dealerBackground;
    if (gallery != Gallery::Main) {
        wchar_t original[512]{};
        GetPrivateProfileStringW(galleries[int(gallery)].section, L"Background", L"", original, 512, ini.c_str());
        ui.menuDealerRaw = original;
    }
    ui.menuStyleEdited = false;
    ui.menuPreferences = ui.preferences();
    readGameVolume(ui.menu.volume);
    ui.menuVolume = ui.menu.volume;
    ui.menuSnapshot = true;
    ui.menu.settingsView = SettingsView::Showroom;
    if (state.studioView.active) {
        ui.menu.settingsView = SettingsView::Cinematic;
    } else if (ui.cinematic.active) {
        ui.menu.settingsView = SettingsView::Cinematic;
    }
    ui.syncMenuPresentation();
    ui.menu.studioViewSettings=state.studioView.settings;
    ui.menu.studioViewSettings.floorQuality=state.studioViewFloorQuality();
    ui.menu.studioViewSettings.vehicleQuality=state.renderVehicleQuality();
    if(ui.cinematic.active) {
        ui.menu.backgroundChoice=state.activeBackground();ui.menu.styleChoice=state.activeStyle();
        ui.menu.floorReflections=state.renderFloorQuality();ui.menu.vehicleReflections=state.renderVehicleQuality();
        ui.menu.shadows=state.renderShadows();
    }
    ui.menu.toggle(x, y, state.clock);
    ui.menu.monitorKeyboard = keyboard;
}

// returning to settings ends photo rendering but keeps its separate camera for the next visit
void closeStudioPhoto(float width, float height)
{
    auto& photo = state.photo;
    const auto panel = photo.panel(width, height);
    state.closePhoto();
    if (state.ui.cinematic.active || state.studioView.active) {
        openStudioSettings(panel.x / width, panel.y / height, true);
        state.ui.dock = true;
    }
    state.cue = Cue::Back;
    blockActions();
}

void requestStudioAction(StudioAction action)
{
    if (action == StudioAction::None || state.busy() ||
        (action == StudioAction::Favorites && !galleries[int(gallery)].fullService)) {
        return;
    }
    if (action == StudioAction::Music) {
        const int choice = validatedMusicTrack(state.ui.menu.choice);
        state.ui.menu.musicChoice = choice;
        audio.chooseMusic(choice,gameWindow(),base + L"GT4Showroom/audio/");
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::Language) {
        if (localization.select(state.ui.menu.choice)) {
            state.ui.language = localization.selected;countryLanguage=state.ui.language;
            state.cue = Cue::Accept;
        } else {
            state.cue = Cue::Back;
        }
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        blockActions();
        return;
    }
    if (action == StudioAction::FloorReflections || action == StudioAction::VehicleReflections) {
        if(state.ui.cinematic.active) {
            if(action==StudioAction::FloorReflections)state.inspection.floor=std::clamp(state.ui.menu.choice,0,4);
            else state.inspection.vehicle=std::clamp(state.ui.menu.choice,0,4);
        } else if (action == StudioAction::FloorReflections) {
            state.ui.chooseFloorReflections(state.ui.menu.choice);
            state.ui.syncMenuPresentation();
        } else {
            state.ui.vehicleReflections = std::clamp(state.ui.menu.choice, 0, 4);
        }
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::Antialiasing || action == StudioAction::AmbientOcclusion) {
        auto& quality = action == StudioAction::Antialiasing ? state.ui.antialiasing : state.ui.ambientOcclusion;
        quality = std::clamp(state.ui.menu.choice, 0, 3);
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::ShowroomStyle) {
        if(state.ui.cinematic.active) {
            state.inspection.style=std::clamp(state.ui.menu.choice,0,studioStyleCount-1);
            state.inspection.floor=state.inspection.shadows=-1;
        } else {
            state.ui.menuStyleEdited=true;
            state.ui.showroomStyle=std::clamp(state.ui.menu.choice,0,studioStyleCount-1);
            const auto path=(std::filesystem::path(base)/L"GT4Showroom/settings.ini").wstring();
            state.ui.menu.musicChoice=state.ui.showroomStyle?validatedMusicTrack(int(GetPrivateProfileIntW(L"Main",L"PrologueMusic",9,path.c_str()))):galleryOptions.music;
            audio.chooseMusic(state.ui.menu.musicChoice,gameWindow(),base+L"GT4Showroom/audio/");
        }
        state.ui.syncMenuPresentation();
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::Background && state.activeStyle() != 0) { return; }
    if (action == StudioAction::Background && state.ui.cinematic.active) {
        state.inspection.background=std::clamp(state.ui.menu.choice,0,studioBackgroundCount-1);
        state.inspection.floor=state.inspection.shadows=-1;
        state.studioView.settings.inheritedFloorQuality=-1;
        state.ui.menu.open=true;state.ui.menu.selectedAction();state.cue=Cue::Accept;blockActions();return;
    }
    if (action == StudioAction::Background) {
        if (gallery != Gallery::Main && !state.ui.favoritesOnly) {
            if (saveGalleryBackground(ini.c_str(), gallery, state.ui.menu.choice)) state.ui.dealerBackgroundWritten = true;
            else log("Could not save dealership background");
        }
        state.ui.chooseBackground(state.ui.menu.choice);
        state.ui.syncMenuPresentation();
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::CinematicAnimation || action == StudioAction::CinematicRotation) {
        auto& camera = state.ui.cinematic;
        const bool enabled = state.ui.menu.choice != 0;
        if (action == StudioAction::CinematicAnimation) {
            camera.setAnimation(enabled, state.clock);
        } else if (camera.paused == enabled) {
            camera.toggle(state.clock);
        }
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::UnitFormat) {
        state.ui.unitFormat = std::clamp(state.ui.menu.choice, 0, 2);
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::FilterCountry || action == StudioAction::FilterClass) {
        auto& ui = state.ui;
        ui.countryShortcut = false;
        const bool country = action == StudioAction::FilterCountry;
        ui.countryPresentationPriority = country;
        auto& mask = country ? ui.countryMask : ui.classMask;
        auto& legacy = country ? ui.countryFilter : ui.classFilter;
        const int choice = ui.menu.choice;
        if (choice < 0) {
            mask = 0;
        } else {
            if (!mask && legacy >= 0) {
                mask = uint64_t(1) << legacy;
            }
            mask ^= uint64_t(1) << choice;
        }
        legacy = -1;
        ui.syncMenuPresentation();
        // filters update the catalogue in place; the picker remains open for the next tick
        rebuildCatalogueView();
        refreshSelected();
        saveStudioPreferences();
        ui.menu.open = ui.menu.sub = true;
        ui.menu.countryFocus = true;
        ui.menu.focusLevel = 2;
        ui.menu.monitorPicker = true;
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::EnterPhoto) {
        if ((state.ui.cinematic.active || state.studioView.active) && !state.studioView.transitioning()) {
            state.ui.menu.finish(1);
            finishSettings();
            state.photo.loadPresets(std::filesystem::path(base) / L"GT4Showroom/photo-presets.dat");
            state.beginPhoto();
            photoHide.blockUntilRelease();
            photoReset.blockUntilRelease();
            state.ui.vehicle.endWheelDrag();
            state.ui.cinematic.grabbed = false;
            state.ui.dock = true;
            state.cue = Cue::Accept;
            blockActions();
        }
        return;
    }
    if (action == StudioAction::EnterTransFender) {
        if (!canCustomizeSelected()) { return; }
        dealerShopVisited = gallery != Gallery::Main;
        state.ui.menu.finish(1);
        finishSettings();
        state.shopExitArmed = state.shopReturnNavigation = false;
        state.shop.fromStudioView = state.studioView.active;
        state.shop.request(ShopPage::Menu, state.clock);
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::Cinematic) {
        state.ui.menu.finish(1);
        finishSettings();
        const bool started = state.startCinematic();
        state.cue = started ? Cue::Accept : Cue::Error;
        blockActions();
        return;
    }
    if (action == StudioAction::StudioViewType || action == StudioAction::StudioViewAnimate || action == StudioAction::StudioViewScene ||
        action == StudioAction::StudioViewColor || action == StudioAction::StudioViewFloor ||
        action == StudioAction::StudioViewPalette || action == StudioAction::StudioViewReflections ||
        action == StudioAction::StudioViewFloorReflections || action == StudioAction::StudioViewVehicleReflections ||
        action == StudioAction::StudioViewShadows || action == StudioAction::StudioViewPause) {
        auto& studioView = state.studioView;
        const int value = state.ui.menu.choice;
        if(action==StudioAction::StudioViewType) {
            const int code=studioBackdropCode(studioView.settings,state.activeStyle(),state.activeBackground());
            state.chooseStudioViewBackground(studioBackdropChoices(!animatedStudioBackdrop(code)).front());
        } else if (action == StudioAction::StudioViewAnimate) {
            studioView.settings.animate = value != 0;
            studioView.syncBackground(state.clock);
            state.ui.cinematic.setAnimation(value != 0,state.clock);
        } else if (action == StudioAction::StudioViewScene) {
            state.chooseStudioViewBackground(value);
        } else if (action == StudioAction::StudioViewColor) {
            studioView.settings.inheritStudio=false;
            studioView.settings.wallColor = value;
        } else if (action == StudioAction::StudioViewFloor) {
            studioView.settings.inheritStudio=false;
            studioView.settings.floorColor = value;
        } else if (action == StudioAction::StudioViewPalette) {
            studioView.settings.inheritStudio=false;
            studioView.settings.palette = value;
        } else if(action==StudioAction::StudioViewFloorReflections) {
            if(studioView.settings.inheritStudio)studioView.settings.inheritedFloorQuality=std::clamp(value,0,4);
            else studioView.settings.floorQuality=std::clamp(value,0,4);
        } else if(action==StudioAction::StudioViewVehicleReflections) {
            studioView.settings.vehicleQuality=std::clamp(value,0,4);
        } else if (action == StudioAction::StudioViewReflections) {
            if (value==0 || value==1) studioView.settings.reflections = value != 0;
            else if(value==2 || value==3) studioView.settings.vehicleReflections = value==3;
            else studioView.settings.reflectionQuality=std::clamp(value-3,1,4);
        } else if (action == StudioAction::StudioViewShadows) {
            studioView.settings.shadows = value != 0;
        } else if (studioView.paused != (value != 0)) {
            studioView.toggle(state.clock);
        }
        studioView.settings.validate();
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if(action==StudioAction::DealershipLogo) {
        if(gallery==Gallery::Main)return;
        const bool enabled=state.ui.menu.choice!=0;
        saveDealershipLogoSetting(enabled);
        state.ui.dealershipLogo=enabled;
        state.cue=Cue::Accept;
        state.ui.menu.dealershipLogo=state.ui.dealershipLogo;state.ui.menu.open=true;blockActions();return;
    }
    if (action == StudioAction::Shadows || action == StudioAction::CarLogo || action == StudioAction::BrandLogo ||
        action == StudioAction::DisplayModelId || action == StudioAction::DisplayModelName) {
        const bool value = state.ui.menu.choice != 0;
        if (action == StudioAction::Shadows) {
            if(state.ui.cinematic.active)state.inspection.shadows=value;else state.ui.chooseShadows(value);
            state.ui.syncMenuPresentation();
        } else if (action == StudioAction::CarLogo) {
            state.ui.carLogo = value;
        } else if (action == StudioAction::BrandLogo) {
            state.ui.brandLogo = value;
        } else if (action == StudioAction::DisplayModelId) {
            state.ui.displayModelId = value;
        } else {
            state.ui.displayModelName = value;
        }
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }

    if (action == StudioAction::Hotspots) {
        if (!state.ui.menu.allowHotspots) {
            return;
        }
        auto& visible = state.studioView.active ? state.studioView.hotspots : state.ui.vehicle.hotspots;
        visible = state.ui.menu.choice != 0;
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    state.ui.menu.open = false;
    state.ui.transition.request(action, state.clock);
    state.dragging = false;
    state.cue = Cue::Accept;
}

void applyViewTransition()
{
    auto& t = state.ui.transition;
    if (!t.ready(state.clock)) {
        return;
    }
    const auto action = t.action;
    const bool returnPointer = state.ui.dock;
    int preferredModel = -1;
    const bool persistentView = action >= StudioAction::SortAscending && action <= StudioAction::SortModelId;
    if (action >= StudioAction::SortAscending && action <= StudioAction::SortModelId) {
        state.ui.countryShortcut = false;
        state.ui.sortMode = int(action) - int(StudioAction::SortAscending);
        studioConfig.sort = menuSort(state.ui.sortMode);
    } else if (action == StudioAction::Favorites && !state.ui.favoritesOnly) {
        if (state.selected < entries.size()) {
            state.beforeFavoritesModel = entries[state.selected].id;
        }
        state.ui.favoritesOnly = true;
    } else if (action == StudioAction::AllCars && state.ui.favoritesOnly) {
        preferredModel = state.beforeFavoritesModel;
        state.beforeFavoritesModel = -1;
        state.ui.favoritesOnly = false;
    }
    if (persistentView) {
        saveStudioPreferences();
    }
    state.leavePrize();
    state.ui.cinematic.leave();
    state.ui.vehicle.centerWheels();
    state.studioView.reset(state.clock);
    state.studioViewSnapshot = state.postSelectionCinematic = false;
    state.spawnReview = SpawnReview::None;
    state.setSelectionPage(Page::Cars);
    state.paintMix = 0;
    state.spawn = false;
    state.closeSearch();
    state.ui.keyboard.open = false;
    rebuildCatalogueView(preferredModel);
    refreshSelected();
    state.ui.dock = returnPointer;
    state.changed = state.motionBegan = state.clock - 2;
    state.status.clear();
    pending.clear();
    t.applied(state.clock);
    blockActions();
}

void favoriteSelection(bool add)
{
    if(state.variantSession)return;
    if (!galleries[int(gallery)].fullService || entries.empty() || state.selected >= entries.size() || state.page != Page::Cars || state.busy()) {
        return;
    }
    auto& e = entries[state.selected];
    if (e.favorite == add) {
        return;
    }
    e.favorite = add;
    for (auto& c : catalog) {
        if (c.id == e.id) {
            c.favorite = add;
        }
    }
    saveCurrentFavorites();
    refreshCountryMenu();
    if (!add && state.ui.favoritesOnly) {
        requestStudioAction(StudioAction::RefreshFavorites);
    }
    state.cue = add ? Cue::Favorite : Cue::Back;
}

void submitStudioSearch()
{
    state.submitSearch(entries);
    state.ui.dock = true;
    state.ui.keyboard.open = false;
    if (state.page == Page::Prize) {
        audio.endPrize(false);
    }
    blockActions();
}

#include "SearchControls.inl"
#include "CountryPickerControls.inl"
#include "PlateControls.inl"

void interaction(const PadActions& pad, float W, float H, double dt, uint64_t tick)
{
    if (!entries.empty() && state.selected < entries.size()) {
        state.ui.vehicle.bind(entries[state.selected].id);
        state.ui.vehicle.rearSteeringAllowed = entries[state.selected].handlingDrive != '4';
    }
    applyViewTransition();
    if (state.shopResetGroup >= 0) {
        resetShopSelection(state.shopResetGroup == 0);
        state.shopResetGroup = -1;
    }
    state.ui.menu.sync(state.ui.sortMode,
                       state.ui.countryFilter,
                       state.ui.classFilter,
                       state.ui.shadows(),
                       state.ui.carLogo,
                       state.ui.brandLogo,
                       state.ui.displayModelId,
                       state.ui.displayModelName);
    state.ui.menu.countryMask = state.ui.countryMask;
    state.ui.menu.classMask = state.ui.classMask;
    state.ui.menu.antialiasing = state.ui.antialiasing;
    state.ui.menu.ambientOcclusion = state.ui.ambientOcclusion;
    state.ui.menu.vehicleReflections = state.ui.vehicleReflections;
    state.ui.menu.language = state.ui.language;
    state.ui.menu.unitFormat = state.ui.unitFormat;
    state.ui.menu.setCategories(state.ui.japaneseCategory(), state.ui.bikeCategory());
    state.ui.syncMenuPresentation();
    state.ui.menu.hotspots = state.hotspotsEnabled();
    state.ui.menu.studioViewSettings = state.studioView.settings;
    state.ui.menu.studioViewSettings.floorQuality=state.studioViewFloorQuality();
    state.ui.menu.studioViewSettings.vehicleQuality=state.renderVehicleQuality();
    if(state.ui.cinematic.active) {
        state.ui.menu.backgroundChoice=state.activeBackground();state.ui.menu.styleChoice=state.activeStyle();
        state.ui.menu.floorReflections=state.renderFloorQuality();state.ui.menu.vehicleReflections=state.renderVehicleQuality();
        state.ui.menu.shadows=state.renderShadows();
    }
    state.ui.menu.studioViewPaused = state.studioView.paused;
    state.ui.menu.cinematicPaused = state.ui.cinematic.paused;
    state.ui.menu.cinematicAnimated = state.ui.cinematic.animate;
    const uintptr_t rawMouse = 0xB73418;
    float dx = game::at<float>(rawMouse + 0xC), dy = game::at<float>(rawMouse + 0x10);
    if (game::flag(0xBA6744)) {
        dx = -dx;
    }
    if (game::flag(0xBA6745)) {
        dy = -dy;
    }
    const bool lDown = game::at<uint8_t>(rawMouse) != 0 || key(VK_LBUTTON),
               rDown = game::at<uint8_t>(rawMouse + 1) != 0 || key(VK_RBUTTON);
    state.cursorPressed = lDown;
    state.pointerMove(dx * mouseSensitivity, dy * mouseSensitivity, W, H);
    const bool click = mouseLeft.poll(lDown, tick), context = mouseRight.poll(rDown || pad.view || key(VK_APPS), tick);
    const bool search = searchKey.poll(key(VK_OEM_3) || pad.search, tick);
    const bool leftPress = left.poll(key(VK_LEFT) || pad.left, tick, true),
               rightPress = right.poll(key(VK_RIGHT) || pad.right, tick, true);
    const bool modal = state.countryPicker.open || state.ui.menu.open || state.shop.open() || state.photo.active || state.searching ||
                       state.spawnReview != SpawnReview::None || state.plateOpen || state.informationOpen || state.tutorialOpen() ||
                       state.page == Page::Extras || state.selectionAdvancePending;
    const bool upPress = up.poll(key(VK_UP) || pad.up, tick, modal),
               downPress = down.poll(key(VK_DOWN) || pad.down, tick, modal);
    const bool editingText = (state.countryPicker.open&&state.countryPicker.editing) || (state.plateOpen && state.plateEditing) || (state.searching && state.searchEditing);
    const bool accept = enter.poll(key(VK_RETURN) || (!editingText && key(VK_SPACE)) || pad.accept, tick),
               goBack = backspace.poll(key(VK_BACK) || pad.back, tick);
    const bool add = addFavorite.poll(pad.add, tick),
               favoritesPress = favorite.poll((!state.searching && key('F')) || pad.favorites, tick);
    const bool previousHotspot = previousCinematicHotspot.poll(
        (pad.previousHotspot && !pad.togglePanels) || (key(VK_TAB) && key(VK_SHIFT)), tick);
    const bool nextHotspot = nextCinematicHotspot.poll(
        (pad.nextHotspot && !pad.togglePanels) || (key(VK_TAB) && !key(VK_SHIFT)), tick);
    const bool pauseView = pauseCinematic.poll(key('P'), tick);
    const bool togglePanels = cinematicPanels.poll(key('T') || pad.togglePanels, tick);
    const bool resetShop = shopResetKey.poll(key('R') || (state.shop.open() && pad.favorites), tick);
    if (pad.accept || pad.back || pad.add || pad.favorites || pad.view || pad.search || pad.left || pad.right ||
        pad.up || pad.down || pad.previousHotspot || pad.nextHotspot || pad.orbitX || pad.orbitY || pad.zoom) {
        state.ui.controllerUsed = true;
    }
    if (lDown || rDown || key(VK_APPS) || key(VK_LEFT) || key(VK_RIGHT) || key(VK_UP) || key(VK_DOWN) ||
        key(VK_RETURN) || key(VK_SPACE) || key(VK_BACK) || key(VK_TAB) || key('P') || key('T') || key('F') ||
        key('R') || key('H') || key(VK_OEM_3)) {
        state.ui.controllerUsed = false;
    }
    if (leftPress || rightPress || upPress || downPress || accept || goBack || search || favoritesPress || add) {
        state.ui.dock = true;
    }
    const bool moved = std::abs(dx) + std::abs(dy) > .03f;
    const bool wheel = game::at<uint8_t>(rawMouse + 3) || game::at<uint8_t>(rawMouse + 4);
    const bool activeInput = moved || click || context || accept || goBack || leftPress || rightPress || upPress ||
                             downPress || search || favoritesPress || add || previousHotspot || nextHotspot ||
                             pauseView || togglePanels || wheel || pad.orbitX || pad.orbitY || pad.zoom;
    const bool information = informationKey.poll(key('I') || (pad.add && pad.favorites), tick);
    if(state.countryPicker.open) {
        auto& picker=state.countryPicker;auto& keyboard=state.ui.keyboard;
        picker.clock=state.clock;
        const int selectionBefore=picker.selected;
        const float px=state.pointerX*W,py=state.pointerY*H,scale=H/720.f;const auto box=picker.panel(W,H);picker.clamp();
        const auto closePicker=[&]{picker.open=false;keyboard.open=false;state.cue=Cue::Back;blockActions();};
        if(context||(goBack&&pad.back)){closePicker();return;}
        // Enter/Space on the highlighted Cancel must close the picker even while the
        // search field is active, where Enter would otherwise only end editing.
        if(picker.cancelHot&&accept){closePicker();return;}
        if(keyboard.open) {
            keyboard.changeCharacters(pad.previousHotspot,pad.nextHotspot,tick);
            if(leftPress||rightPress)keyboard.move(int(rightPress)-int(leftPress),0);if(upPress||downPress)keyboard.move(0,int(downPress)-int(upPress));
            int action=0;if(accept&&pad.accept)action=keyboard.type(picker.query);
            if(click)for(int i=0;i<40;++i)if(keyboard.box(W,H,i).contains(px,py)){keyboard.selected=i;action=keyboard.type(picker.query);break;}
            if(action){keyboard.open=false;picker.editing=false;blockActions();}picker.clamp();return;
        }
        if(search&&pad.search){keyboard.open=true;keyboard.selected=0;picker.editing=true;blockActions();return;}
        if(upPress||downPress||leftPress||rightPress){
            if(picker.cancelHot){if(upPress){picker.cancelHot=false;state.cue=Cue::Move;}}
            else{
                const int before=picker.selected;
                picker.move(int(rightPress)-int(leftPress),int(downPress)-int(upPress));
                if(picker.selected!=before)state.cue=Cue::Move;
                else if(downPress&&picker.selected==int(picker.matches().size())-1){picker.cancelHot=true;state.cue=Cue::Move;}
            }
        }
        if(wheel){picker.firstRow=std::clamp(picker.firstRow+(game::at<uint8_t>(rawMouse+3)?-1:1),0,picker.maxRow(picker.matches().size()));picker.editing=false;state.cue=Cue::Move;}
        const auto input=countrySearchInput.poll([](int vk){return key(vk);},tick,picker.query,picker.editing,80);
        if(input==SearchAction::Cancel){closePicker();return;}
        if(input==SearchAction::Submit){picker.editing=false;blockActions();}
        if(!lDown){picker.dragging=picker.scrolling=false;}
        if(moved){
            const UIRect cancel{box.x+box.w*.5f-70*scale,box.y+box.h-34*scale,140*scale,22*scale};
            if(!picker.editing&&cancel.contains(px,py)){if(!picker.cancelHot){picker.cancelHot=true;state.cue=Cue::Move;}}
            else if(!picker.editing){auto list=picker.matches();for(int i=picker.firstRow*4;i<int(list.size())&&i<(picker.firstRow+4)*4;++i)if(picker.cell(W,H,i).contains(px,py)){if(picker.selected!=i){picker.selected=i;state.cue=Cue::Move;}picker.cancelHot=false;break;}}
        }
        if(click){
            if(UIRect{box.x,box.y,box.w,22*scale}.contains(px,py)){picker.dragging=true;picker.dragX=px-box.x;picker.dragY=py-box.y;}
            else if(UIRect{box.x+box.w*.5f-70*scale,box.y+box.h-34*scale,140*scale,22*scale}.contains(px,py)){closePicker();return;}
            else if(UIRect{box.x,box.y+22*scale,box.w,34*scale}.contains(px,py)){picker.editing=true;countrySearchInput.quarantine();}
            else if(picker.scrollTrack(W,H).contains(px,py)){picker.scrolling=true;picker.editing=false;}
            else {auto list=picker.matches();for(int i=picker.firstRow*4;i<int(list.size())&&i<(picker.firstRow+4)*4;++i)if(picker.cell(W,H,i).contains(px,py)){picker.selected=i;picker.editing=false;assignPickerCountry(list[size_t(i)]);return;}}
        }
        if(picker.dragging&&lDown){picker.x=(px-picker.dragX)/W;picker.y=(py-picker.dragY)/H;}
        if(picker.scrolling&&lDown){auto track=picker.scrollTrack(W,H);picker.firstRow=std::clamp(int(std::lround((py-track.y)/track.h*picker.maxRow(picker.matches().size()))),0,picker.maxRow(picker.matches().size()));}
        if(accept&&!picker.editing&&!picker.dragging&&!picker.scrolling&&input!=SearchAction::Submit){
            if(picker.cancelHot){closePicker();return;}
            auto list=picker.matches();if(!list.empty())assignPickerCountry(list[size_t(picker.selected)]);
        }
        picker.clamp();
        if(picker.selected!=selectionBefore){picker.previous=selectionBefore;picker.changedAt=state.clock;}
        if(upPress||downPress||leftPress||rightPress||wheel)state.ui.dock=true;
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (state.purchase.active()) {
        auto& scene = state.purchase;
        if(scene.leaving>=0) return;
        const int count = scene.buttons();
        if (!count) return;
        if (count == 2 && (leftPress || rightPress)) {
            scene.choice = 1 - scene.choice;
            state.ui.dock = true;
            state.cue = Cue::Move;
        }
        bool clicked = false;
        for (int i = 0; i < count; ++i) {
            if ((moved || click) && scene.button(i,W,H).contains(state.pointerX*W-scene.entryOffset(state.clock,W),state.pointerY*H)) {
                scene.choice = i;
                clicked = click;
                state.ui.dock = false;
            }
        }
        if (accept || clicked || goBack || context) {
            state.advancePurchase(goBack || context);
            blockActions();
        }
        return;
    }
    if (state.tutorialOpen()) {
        if (state.tutorialMotion.leaving >= 0.) { return; }
        const auto button = renderer.tutorialButton(W, H, state);
        if ((moved || click) && button.w > 0) {
            state.tutorialFocus = button.contains(state.pointerX * W, state.pointerY * H);
        }
        if (upPress || downPress || leftPress || rightPress) { state.tutorialFocus = true; }
        if (goBack || ((accept || (click && button.w > 0)) && state.tutorialFocus)) {
            state.closeTutorial(!goBack);
            blockActions();
        }
        return;
    }
    if (state.informationOpen) {
        if (state.informationMotion.leaving >= 0 || state.clock - state.opened < MenuFade::entryDuration) {
            return;
        }
        if (upPress || downPress || leftPress || rightPress) {
            const int direction = int(downPress || rightPress) - int(upPress || leftPress);
            state.informationChoice = (std::max(0, state.informationChoice) + direction + 4) % 4;
            state.ui.dock = true;
            state.cue = Cue::Move;
        }
        const InformationPanel panel(W, H, state.informationMotion.offset(state.clock, H));
        if (moved || click) {
            const int target = panel.hit(state.pointerX * W, state.pointerY * H);
            if (target != state.informationChoice && target >= 0) {
                state.cue = Cue::Move;
            }
            state.informationChoice = target;
        }
        if (goBack || information || ((click || accept) && state.informationChoice == 3)) {

            state.informationMotion.dismiss(state.clock);
            state.cue = Cue::Back;
            blockActions();
        } else if ((click || accept) && state.informationChoice >= 0 && state.informationChoice < 3) {
            state.cue = openInformationLink(state.informationChoice) ? Cue::Accept : Cue::Error;
            blockActions();
        }
        if (state.ui.dock && state.informationChoice >= 0) {
            const auto target = panel.target(state.informationChoice);
            state.pointerX = (target.x + target.w * .5f) / W;
            state.pointerY = (target.y + target.h * .5f) / H;
        }
        return;
    }
    if (information && !state.photo.requested && !state.ui.menu.open && !state.plateOpen && state.spawnReview == SpawnReview::None &&
        (state.photo.active || state.shop.open() || state.studioView.active || state.ui.cinematic.active)) {
        state.tutorial = state.photo.active ? Tutorial::Photo : state.shop.open() ? Tutorial::Customization :
                         Tutorial::Cinematic;
        state.tutorialMotion = {state.clock};
        state.tutorialFocus = true;
        state.cue = Cue::Open;
        blockActions();
        return;
    }
    if (information && !modal && state.page == Page::Cars && !state.ui.cinematic.active && !state.busy()) {
        state.informationOpen = true;
        state.informationChoice = 3;
        state.informationMotion = {state.clock};
        state.cue = Cue::Open;
        state.ui.dock = true;
        blockActions();
        return;
    }
    if (state.plateOpen) {
        processStudioPlate(pad, W, H, dt, tick, leftPress, rightPress, upPress, downPress,
                           accept, goBack, click, moved, add, context);
        return;
    }
    if (state.photo.active) {
        auto& photo = state.photo;
        if (key('H') || key('W') || key('A') || key('S') || key('D') || key('Q') || key('E') || key(VK_OEM_PLUS) ||
            key(VK_OEM_MINUS)) {
            state.ui.controllerUsed = false;
        }
        const bool heldSettings=rDown || pad.view;
        if(context && heldSettings) { photo.settingsPressed=true; photo.settingsTravel=0; }
        if(heldSettings && photo.settingsPressed) photo.settingsTravel+=std::abs(dx)+std::abs(dy)+10.f*(std::abs(pad.orbitX)+std::abs(pad.orbitY));
        const bool settingsTap=(context && !heldSettings) || (!heldSettings && photo.settingsPressed && photo.settingsTravel<3.f);
        if(!heldSettings) photo.settingsPressed=false;
        if (photo.requested || state.clock < photo.flashBegan + .35) {
            photo.dragging = photo.movingPanel = false;
            return;
        }
        const auto tools=photo.visualPanel(W,H);
        const float px=state.pointerX*W, py=state.pointerY*H;
        if (photo.dragging) {
            if (!lDown) photo.dragging=false;
            else if(photo.draggingVisual) {
                photo.visualX=std::clamp(px-photo.dragX,0.f,std::max(0.f,W-tools.w))/W;
                photo.visualY=std::clamp(py-photo.dragY,0.f,std::max(0.f,H*.907f-tools.h))/H;
            } else photo.place(px-photo.dragX,py-photo.dragY,W,H);
            return;
        }
        if (!photo.hidden && click && !rDown) {
            if(tools.contains(px,py)) {
                photo.visuals=true;
                if(py<tools.y+21*StudioViewPhoto::uiScale(W,H)) {
                    photo.dragging=true; photo.draggingVisual=true;
                    photo.dragX=px-tools.x;photo.dragY=py-tools.y;photo.row=0;return;
                }
            } else if(photo.panel(W,H).contains(px,py)) {
                if(photo.visuals) photo.row=StudioViewPhoto::captureRow;
                photo.visuals=false;
                if(photo.header(W,H).contains(px,py)) {
                    auto box=photo.panel(W,H);photo.dragging=true;photo.draggingVisual=false;
                    photo.dragX=px-box.x;photo.dragY=py-box.y;return;
                }
            }
        }
        if (photo.visuals && (goBack || settingsTap)) {
            photo.visuals=false; photo.row=StudioViewPhoto::captureRow;
            blockActions(); return;
        }
        if ((!state.ui.cinematic.active && !state.studioView.active) || goBack) {
            closeStudioPhoto(W, H);
            return;
        }
        if (settingsTap && !photo.hidden) {
            photo.visuals=true; photo.row=0; state.cue=Cue::Open; blockActions(); return;
        }
        if (photoHide.poll(key('H') || pad.search,tick)) {
            photo.hidden=!photo.hidden; blockActions(); return;
        }
        if (accept && key(VK_SPACE)) {
            photo.triggerCapture(state.clock);
            state.cue = Cue::Flash;
            photo.dragging = photo.movingPanel = false;
            blockActions();
            return;
        }
        if (photo.visuals && !photo.hidden) {
            const int count=photo.visualCount();
            if(upPress || downPress) {photo.row=(photo.row+int(downPress)-int(upPress)+count)%count;photo.side=1;}
            int direction=int(rightPress)-int(leftPress);
            bool selected=accept;
            if(moved || click) {
                photo.side=0;
                for(int i=0;i<count;++i) {
                    const auto b=photo.visualItem(W,H,i);
                    if(!b.contains(state.pointerX*W,state.pointerY*H))continue;
                    photo.row=i;
                    const float scale=StudioViewPhoto::uiScale(W,H);
                    const UIRect field{b.x+b.w*.43f,b.y+3*scale,b.w*.54f,b.h-6*scale};
                    const float edge=field.h*1.2f;
                    if(UIRect{field.x,field.y,edge,field.h}.contains(px,py))photo.side=-1;
                    else if(UIRect{field.x+field.w-edge,field.y,edge,field.h}.contains(px,py))photo.side=1;
                    if(click && photo.side){selected=true;direction=photo.side;}
                    break;
                }
            }
            if((selected || direction) && photo.row<count) state.adjustPhotoVisual(photo.row,direction?direction:1);
            if(selected || direction) { state.cue=Cue::Slider; blockActions(); }
            return;
        }
        if (photoReset.poll(key('R') || pad.favorites, tick)) {
            state.resetPhotoCamera();
            state.cue = Cue::Accept;
            blockActions();
            return;
        }
        const float seconds = float(std::clamp(dt, 0., .1));
        const bool panelChord = !photo.hidden && pad.add;
        const bool movePanel = panelChord && (pad.orbitX || pad.orbitY);
        if (movePanel) {
            const auto box = photo.panel(W, H);
            // native right-stick up is positive, while screen y grows downward
            photo.place(box.x + pad.orbitX * W * seconds * .45f, box.y - pad.orbitY * H * seconds * .45f, W, H);
            if (!photo.movingPanel) {
                state.cue = Cue::Move;
            }
            state.ui.dock = false;
        }
        photo.movingPanel = movePanel;
        if (!photo.hidden && !movePanel && (upPress || downPress)) {
            photo.row = (photo.row + int(downPress) - int(upPress) + StudioViewPhoto::rowCount) % StudioViewPhoto::rowCount;
            state.cue = Cue::Move;
        }
        const int direction = int(rightPress) - int(leftPress);
        if (!photo.hidden && !movePanel && direction) {
            if (photo.row < StudioViewPhoto::settingCount) {
                state.cue = photo.adjust(direction) ? Cue::Slider : Cue::Error;
            } else {
                photo.row = photo.row == StudioViewPhoto::captureRow ? StudioViewPhoto::backRow : StudioViewPhoto::captureRow;
                state.cue = Cue::Move;
            }
        }
        const bool pan = key(VK_SHIFT) || pad.view;
        const float lookX = (rDown ? dx * .003f : 0.f) + (panelChord ? 0.f : pad.orbitX * seconds * 1.5f);
        const float lookY = (rDown ? dy * .003f : 0.f) + (panelChord ? 0.f : pad.orbitY * seconds);
        if (pan) {
            photo.panX = std::clamp(photo.panX - lookX * 3, -8.f, 8.f);
            photo.panZ = std::clamp(photo.panZ + lookY * 3, -3.f, 8.f);
        } else {
            photo.yaw = std::remainder(photo.yaw + lookX, 2 * pi);
            photo.pitch = std::clamp(photo.pitch + lookY, -.3f, 1.3f);
        }
        photo.panX = std::clamp(photo.panX + (float(key('D')) - float(key('A'))) * seconds * 2, -8.f, 8.f);
        photo.panZ = std::clamp(photo.panZ + (float(key('W')) - float(key('S'))) * seconds * 2, -3.f, 8.f);
        photo.roll = std::remainder(
            photo.roll + (float(key('E') || pad.nextHotspot) - float(key('Q') || pad.previousHotspot)) * seconds * 30,
            360.f);
        photo.fov =
            std::clamp(photo.fov + (float(key(VK_OEM_MINUS)) - float(key(VK_OEM_PLUS))) * seconds * 25, 15.f, 100.f);
        if (wheel) {
            const float direction = game::at<uint8_t>(rawMouse + 3) ? 1.f : -1.f;
            photo.zoom = std::clamp(photo.zoom * std::exp(direction * .08f), .3f, 3.f);
            state.cue = Cue::Slider;
        }
        if (!pad.zoomFromShoulders && pad.zoom) {
            photo.zoom = std::clamp(photo.zoom * std::exp(pad.zoom * seconds), .3f, 3.f);
        }
        bool activate = accept && !photo.hidden;
        int clickedDirection = 0;
        if (!photo.hidden && !rDown && (moved || click)) {
            const int hit = photo.hit(W, H, state.pointerX * W, state.pointerY * H, clickedDirection);
            if (hit >= 0) {
                if (photo.row != hit || (clickedDirection && photo.side != clickedDirection)) {
                    state.cue = Cue::Move;
                }
                photo.row = hit;
                if (clickedDirection) {
                    photo.side = clickedDirection;
                }
                activate = activate || (click && (hit >= StudioViewPhoto::settingCount || clickedDirection));
            }
        }
        if (activate) {
            if (photo.row < StudioViewPhoto::settingCount) {
                state.cue = photo.adjust(clickedDirection ? clickedDirection : photo.side) ? Cue::Slider : Cue::Error;
            } else if (photo.row == StudioViewPhoto::saveRow) {
                const bool stored = photo.storePreset() &&
                    photo.savePresets(std::filesystem::path(base) / L"GT4Showroom/photo-presets.dat");
                photo.status = stored ? "Preset saved" : "Preset could not be saved";
                photo.statusUntil = state.clock + 3;
                state.cue = stored ? Cue::Accept : Cue::Error;
                blockActions();
            } else if (photo.row == StudioViewPhoto::loadRow) {
                const bool loaded = photo.restorePreset(state.studioView, state.clock);
                photo.status = loaded ? "Preset loaded" : "This preset is empty";
                photo.statusUntil = state.clock + 3;
                state.cue = loaded ? Cue::Accept : Cue::Error;
                blockActions();
            } else if (photo.row == StudioViewPhoto::captureRow) {
                photo.triggerCapture(state.clock);
                state.cue = Cue::Flash;
                blockActions();
            } else if (photo.row == StudioViewPhoto::backRow) {
                closeStudioPhoto(W, H);
            }
        }
        if (photo.active) {
            dockPointer(state, entries, W, H, dt);
        }
        return;
    }
    if (resetShop && state.shop.open() && !state.shop.locked() && !state.shop.partReview &&
        state.spawnReview == SpawnReview::None) {
        state.spawnReview = SpawnReview::ShopReset;
        state.review = {state.clock};
        state.pendingReviewChoice = -1;
        state.reviewChoice = 1;
        state.cue = Cue::Accept;
        blockActions();
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (state.shop.open() && state.spawnReview == SpawnReview::None) {
        state.studioView.activity(state.clock);
        const bool wheelLeft = wheel && game::at<uint8_t>(rawMouse + 3);
        if (context && state.shop.page==ShopPage::Menu) state.shopExitArmed=false;
        processShopInput(leftPress || wheelLeft,
                         rightPress || (wheel && !wheelLeft),
                         upPress,
                         downPress,
                         accept,
                         goBack || context,
                         click,
                         lDown,
                         moved,
                         W,
                         H);
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (activeInput && state.studioView.activity(state.clock)) {
        state.ui.vehicle.endWheelDrag();
        state.ui.cinematic.grabbed = false;
        blockActions();
        return;
    }
    if (state.busy()) {
        return;
    }
    if (state.spawnReview != SpawnReview::None) {
        if (state.pendingReviewChoice >= 0) {
            return;
        }
        const int buttonCount = state.reviewButtons();
        if (leftPress || upPress) {
            state.reviewChoice = state.reviewChoice < 0 ? buttonCount - 1
                                                       : (state.reviewChoice + buttonCount - 1) % buttonCount;
            state.cue = Cue::Move;
        }
        if (rightPress || downPress) {
            state.reviewChoice = (state.reviewChoice + 1) % buttonCount;
            state.cue = Cue::Move;
        }
        if (moved || click) {
            const int previous = state.reviewChoice;
            state.reviewChoice = -1;
            for (int i = 0; i < buttonCount; ++i) {
                auto button = spawnReviewButton(W, H, i, buttonCount);
                button.y += state.reviewOffset(H);
                if (button.contains(state.pointerX * W, state.pointerY * H)) {
                    state.reviewChoice = i;
                    if (previous != i) {
                        state.cue = Cue::Move;
                    }
                    if (click) {
                        state.chooseSpawnReview(static_cast<ReviewChoice>(i));
                        blockActions();
                        return;
                    }
                }
            }
        }
        if (accept && state.reviewChoice >= 0) {
            state.chooseSpawnReview(static_cast<ReviewChoice>(state.reviewChoice));
            blockActions();
        } else if (goBack || context) {
            if (!goBack && state.spawnReview == SpawnReview::Cinematic) {
                state.chooseSpawnReview(ReviewChoice::Dismiss);
            } else {
                state.back();
            }
            blockActions();
        }
        if (state.ui.dock && state.reviewChoice >= 0) {
            auto target = spawnReviewButton(W, H, state.reviewChoice, buttonCount);
            target.y += state.reviewOffset(H);
            state.pointerX = (target.x + target.w * .5f) / W;
            state.pointerY = (target.y + target.h * .5f) / H;
        }
        return;
    }
    if (state.selectionAdvancePending) {
        if (goBack || context) { state.back(); blockActions(); }
        return;
    }
    if (state.page == Page::Extras && !state.ui.cinematic.active) {
        if (photoHide.poll(key('H') || pad.togglePanels,tick)) {
            state.ui.footerHidden = !state.ui.footerHidden;
            blockActions();
            return;
        }
        auto& m = state.ui.menu;
        const auto rows = m.items(1);
        const int count = int(rows.size());
        if (goBack || context) {
            state.back();
            blockActions();
            return;
        }
        const float x = state.pointerX * W, y = state.pointerY * H;
        if (count && !state.ui.footerHidden && m.monitorSlide(x, y, W, H, lDown, click)) {
            const int group = m.extraGroup(m.sortRow);
            if (m.sliderChanged && group >= 0) {
                const int wanted = m.sliderValue(m.sortRow);
                m.extraChoice = nativeExtras::choice;
                state.cue = selectStudioExtra(group, wanted) ? Cue::Slider : Cue::Error;
                m.sliderChanged = false;
            }
            state.ui.dock = false;
            return;
        }
        if (!state.ui.footerHidden && (moved || click)) {
            m.monitorKeyboard = false;
            m.monitorHover = true;
            m.monitorHoverLevel = m.monitorHoverIndex = m.monitorFooter = -1;
            for (int i = 0; i < count; ++i) {
                if (m.monitorField(1, i, W, H).contains(x, y)) {
                    m.monitorHoverLevel = 1;
                    m.monitorHoverIndex = m.sortRow = i;
                    break;
                }
            }
            for (int i = 0; i < 2; ++i) {
                if (m.monitorButton(1, i, W, H).contains(x, y)) {
                    m.monitorFooterLevel = 1;
                    m.monitorFooter = i;
                }
            }
        }
        if (upPress || downPress) {
            m.monitorKeyboard = true;
            const int current = m.monitorFooter < 0 ? std::min(m.sortRow, count) : count + m.monitorFooter;
            const int next = (current + (downPress ? 1 : -1) + count + 2) % (count + 2);
            m.monitorFooterLevel = 1;
            m.monitorFooter = next < count ? -1 : next - count;
            if (next < count) { m.sortRow = next; }
            state.cue = Cue::Move;
        }
        if ((leftPress || rightPress) && m.monitorFooter < 0 && count) {
            m.monitorKeyboard = true;
            m.sortRow = std::clamp(m.sortRow, 0, count - 1);
            const int group = m.extraGroup(m.sortRow);
            const int wanted = std::clamp(m.sliderValue(m.sortRow) + (rightPress ? 1 : -1), 0, m.sliderMaximum(m.sortRow));
            if (group >= 0 && wanted != m.sliderValue(m.sortRow)) {
                state.cue = selectStudioExtra(group, wanted) ? Cue::Slider : Cue::Error;
            }
        }
        if (accept || (!state.ui.footerHidden && click && m.monitorFooter >= 0)) {
            if (m.monitorFooter == 0) { state.back(); }
            else { state.accept(entries); }
            blockActions();
        }
        return;
    }
    const bool hideFooter = key('H') || (state.ui.cinematic.active ? pad.search : pad.togglePanels);
    if (!modal && photoHide.poll(hideFooter, tick)) {
        if (!state.ui.cinematic.active && pad.togglePanels) {
            state.ui.controllerUsed = true;
        }
        state.ui.footerHidden = !state.ui.footerHidden;
        state.cue = state.ui.footerHidden ? Cue::Back : Cue::Open;
        blockActions();
        return;
    }
    if (state.page == Page::Prize) {
        if (goBack) {
            state.back();
        } else if (accept) {
            state.accept(entries);
        } else if (click) {
            pointerClick(state, entries, W, H);
        }
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (search && (state.searching || state.canBeginSearch())) {
        if (state.searching) {
            state.closeSearch();
            state.ui.keyboard.open = false;
        } else if (state.beginSearch()) {
            state.ui.keyboard.open = pad.search;
            state.searchKeyboard = pad.search;
            state.ui.keyboard.selected = 0;
        }
        blockActions();
        return;
    }
    if (state.searching) {
        processStudioSearch(pad, W, H, dt, tick, leftPress, rightPress, upPress, downPress,
                            accept, goBack, click, moved, add, favoritesPress, context);
        return;
    }

    if (context && state.ui.favoritesOnly && !state.hasAvailableFavorites && !state.ui.menu.open) {
        blockActions();
        return;
    }
    if (context && !state.ui.cinematic.active && (state.page == Page::Paint || state.page == Page::Livery || state.page==Page::Variant)) {
        state.back();
        blockActions();
        return;
    }
    if (context && state.page != Page::Prize) {
        auto& ui = state.ui;
        const bool opened = !ui.menu.open;
        if (opened) {
            openStudioSettings(state.pointerX, state.pointerY, pad.view || key(VK_APPS));
        } else {
            ui.menu.monitorDragLevel = ui.menu.sliderDrag = -1;
            ui.menu.monitorBack();
        }
        ui.menu.monitorKeyboard = pad.view || key(VK_APPS);
        state.cue = opened ? Cue::Open : Cue::Back;
        state.dragging = false;
        ui.cinematic.grabbed = false;
        ui.vehicle.endWheelDrag();
        if (pad.view || key(VK_APPS)) {
            ui.dock = true;
        }
        finishSettings();
        blockActions();
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (state.ui.menu.open) {
        auto& m = state.ui.menu;
        const int previousTarget = m.audibleTarget();
        const auto previous = std::make_tuple(m.open, m.sub, m.focus(), m.monitorPicker);
        if (goBack) {
            m.monitorDragLevel = m.sliderDrag = -1;
            m.monitorBack();
            finishSettings();
            state.cue = Cue::Back;
            blockActions();
            return;
        }
        auto applySliders = [&] {
            if(m.typeChanged){m.typeChanged=false;requestStudioAction(StudioAction::StudioViewType);return;}
            if (!m.sliderChanged) {
                return;
            }
            if (m.childBranch() == MenuBranch::Volume && !setGameVolume(m.volume)) {
                readGameVolume(m.volume);
                state.cue = Cue::Error;
            }
            m.sliderChanged = false;
            if (state.cue == Cue::None) {
                state.cue = Cue::Move;
            }
        };
        if (m.monitorSlide(state.pointerX * W, state.pointerY * H, W, H, lDown, click)) {
            applySliders();
            state.ui.dock = false;
            return;
        }
        if (m.monitorDrag(state.pointerX * W, state.pointerY * H, W, H, lDown, click)) {
            state.ui.dock = false;
            return;
        }
        if (std::abs(dx) + std::abs(dy) > .03f) {
            m.hover(state.pointerX * W, state.pointerY * H, W, H);
        }
        if (upPress) {
            m.move(0, -1);
        }
        if (downPress) {
            m.move(0, 1);
        }
        if (leftPress) {
            m.move(-1, 0);
        }
        if (rightPress) {
            m.move(1, 0);
        }
        if (game::at<uint8_t>(rawMouse + 3)) {
            m.move(0, -1);
        } else if (game::at<uint8_t>(rawMouse + 4)) {
            m.move(0, 1);
        }
        applySliders();
        if (accept) {
            m.monitorKeyboard = true;
            requestStudioAction(m.activate(state.ui.favoritesOnly));
        }
        if (click) {
            requestStudioAction(m.click(state.pointerX * W, state.pointerY * H, W, H, state.ui.favoritesOnly));
        }
        const auto current = std::make_tuple(m.open, m.sub, m.focus(), m.monitorPicker);
        const int target = m.audibleTarget();
        if (state.cue == Cue::None) {
            if (previous != current && (accept || click || rightPress)) {
                state.cue = Cue::Accept;
            } else if (target >= 0 && target != previousTarget) {
                state.cue = Cue::Move;
            }
        }
        if (previous != current) {
            m.monitorChanged = state.clock;
        }
        finishSettings();
        dockPointer(state, entries, W, H, dt);
        return;
    }

    if (state.ui.cinematic.active) {
        auto& c = state.ui.cinematic;
        auto& v = state.ui.vehicle;
        auto& studioView = state.studioView;
        if (goBack) {
            v.endWheelDrag();
            state.back();
            blockActions();
            return;
        }
        const Mesh* mesh = entries.empty() ? nullptr : lookup(entries[state.selected].id);
        if (togglePanels && mesh && state.ui.menu.allowHotspots) {
            v.togglePanels(mesh->cinematic);
        }
        const bool hasHotspots = !state.ui.footerHidden && mesh && state.hotspotsEnabled() && mesh->cinematic.any();
        if (hasHotspots && (previousHotspot || nextHotspot)) {
            const auto points = renderer.hotspots(*mesh, v);
            v.focus = points.cycle(v.focus, previousHotspot ? -1 : 1);
            state.ui.dock = true;
            state.ui.pointerMoved = state.clock;
        }
        if (photoReset.poll(key('R') || (pad.add && pad.previousHotspot), tick) && v.wheelsTurned()) {
            v.resetWheelTargets();
            state.cue = Cue::Accept;
            return;
        }
        const bool wheelFocused = hasHotspots && v.canSteer(v.focus) && mesh->cinematic.supported(v.focus);
        const auto toggleRotation = [&] {
            if (studioView.active) {
                studioView.toggle(state.clock);
            } else {
                c.toggle(state.clock);
            }
        };
        if (accept) {
            if (hasHotspots && v.focus >= 0) {
                if (v.activate(v.focus, mesh->cinematic)) {
                    state.ui.pointerMoved = state.clock;
                }
            } else {
                toggleRotation();
            }
        }
        if (pauseView) {
            toggleRotation();
        }
        bool hotspotClicked = false;
        if (click && hasHotspots) {
            const int hit = renderer.hitHotspot(*mesh, v, state.pointerX * W, state.pointerY * H);
            if (hit >= 0) {
                hotspotClicked = hit >= 6 ? v.beginWheelDrag(hit, mesh->cinematic, state.pointerX * W)
                                          : v.activate(hit, mesh->cinematic);
                state.ui.pointerMoved = state.clock;
            }
        }
        if (!lDown || !hasHotspots) {
            v.endWheelDrag();
        }
        if (v.wheelDrag >= 0) {
            v.dragWheel(state.pointerX * W, W);
            c.grabbed = false;
            state.ui.dock = false;
            state.ui.pointerMoved = state.clock;
            return;
        }
        if (studioView.active) {
            if (click && !hotspotClicked && mesh && renderer.hitCar(*mesh, state.pointerX * W, state.pointerY * H)) {
                studioView.toggle(state.clock);
            }
            if ((lDown && !hotspotClicked) || rDown) {
                studioView.orbit(dx * .007f, dy * .004f, state.clock);
            }
        } else {
            if (click && !hotspotClicked && mesh && renderer.hitCar(*mesh, state.pointerX * W, state.pointerY * H)) {
                c.toggle(state.clock);
                c.grabbed = true;
                c.dragged = false;
                c.grabX = state.pointerX * W;
                c.grabY = state.pointerY * H;
            }
            if (!lDown) {
                c.grabbed = false;
            }
            if (c.grabbed) {
                const float px = state.pointerX * W;
                const float py = state.pointerY * H;
                if (c.dragged || std::abs(px - c.grabX) + std::abs(py - c.grabY) > 3) {
                    c.orbit(dx * .007f, dy * .004f, state.clock);
                    c.dragged = true;
                }
            }
        }
        if (wheelFocused) {
            const float direction =
                (key(VK_RIGHT) || pad.right ? 1.f : 0.f) - (key(VK_LEFT) || pad.left ? 1.f : 0.f) + pad.orbitX;
            v.steer(direction, dt);
            if (direction) {
                state.ui.pointerMoved = state.clock;
                state.ui.dock = true;
            }
        }
        const auto moveCamera = [&](auto& camera) {
            if (!wheelFocused) {
                camera.orbit(pad.orbitX * float(dt) * 1.8f, pad.orbitY * float(dt) * .9f, state.clock);
                if (leftPress || rightPress) {
                    camera.orbit(leftPress ? -.09f : .09f, 0, state.clock);
                }
            }
            if (upPress || downPress) {
                camera.orbit(0, upPress ? -.06f : .06f, state.clock);
            }
            if (wheel) {
                camera.wheel(game::at<uint8_t>(rawMouse + 3) ? 1.f : -1.f);
            }
            if (pad.zoom && !(hasHotspots && pad.zoomFromShoulders)) {
                camera.wheel(pad.zoom * float(dt) * 8);
            }
        };
        if (studioView.active) {
            moveCamera(studioView);
        } else {
            moveCamera(c);
        }
        if (hasHotspots && state.ui.dock && v.focus >= 0) {
            const auto points = renderer.hotspots(*mesh, v);
            for (size_t i = 0; i < points.count; ++i) {
                if (points.items[i].component == v.focus) {
                    const float a = float(1 - std::exp(-std::max(0., dt) * 14));
                    state.pointerX += (points.items[i].x / W - state.pointerX) * a;
                    state.pointerY += (points.items[i].y / H - state.pointerY) * a;
                    break;
                }
            }
        }
        return;
    }
    if (state.page == Page::Cars && !state.busy() && !entries.empty()) {
        const float scale = H / 720.f;
        const float x = state.pointerX * W, y = state.pointerY * H;
        const auto& entry = entries[state.selected];
        if(click&&!state.ui.footerHidden&&entry.country==Country::Unknown&&UIRect{46*scale,(33+renderer.lastIdentityHeaderOffset/scale)*scale,34*scale,28*scale}.contains(x,y)){openCountryPicker();return;}
        if(pad.add&&accept&&entry.country==Country::Unknown){openCountryPicker();return;}
        if (click && !state.ui.footerHidden && entry.country != Country::Unknown &&
            UIRect{46 * scale, (33+renderer.lastIdentityHeaderOffset/scale) * scale, 34 * scale, 28 * scale}.contains(x, y)) {
            auto& ui = state.ui;
            ui.beginCountryShortcut(entry.country, entry.id);
            studioConfig.sort = menuSort(ui.sortMode);
            rebuildCatalogueView(entry.id);
            refreshSelected();
            state.cue = Cue::Accept;
            blockActions();
            return;
        }
        if (click && classIndex(entry.classId) == 5 &&
            UIRect{104 * scale, (19+renderer.lastIdentityHeaderOffset/scale) * scale, 170 * scale, 34 * scale}.contains(x, y)) {
            if (state.clock - state.classClick <= .35) {
                state.heavyUntil = state.clock + 10;
                state.classClick = -10;
                state.cue = Cue::Accept;
            } else {
                state.classClick = state.clock;
            }
            return;
        }
        if (goBack && state.ui.countryShortcut) {
            const int model = state.ui.endCountryShortcut();
            studioConfig.sort = menuSort(state.ui.sortMode);
            rebuildCatalogueView(model);
            refreshSelected();
            state.cue = Cue::Back;
            blockActions();
            return;
        }
    }
    if(state.page==Page::Variant && click){
        if(UIRect{W*.065f,H*.42f,64*H/720.f,70*H/720.f}.contains(state.pointerX*W,state.pointerY*H))state.move(-1,entries);
        if(UIRect{W*.935f-64*H/720.f,H*.42f,64*H/720.f,70*H/720.f}.contains(state.pointerX*W,state.pointerY*H))state.move(1,entries);
    }
    if (leftPress) {
        state.move(-1, entries);
    }
    if (rightPress) {
        state.move(1, entries);
    }
    if (accept) {
        state.accept(entries);
    }
    if (goBack) {
        state.back();
    }
    if (favoritesPress && !state.variantSession && galleries[int(gallery)].fullService && state.page != Page::Prize) {
        requestStudioAction(state.ui.favoritesOnly ? StudioAction::AllCars : StudioAction::Favorites);
    }
    if (upPress || add) {
        favoriteSelection(true);
    }
    if (downPress) {
        favoriteSelection(false);
    }
    if (!state.ui.footerHidden) pointerButton(state, entries, W, H, lDown, click);
    if (!state.dragging) {
        if (game::at<uint8_t>(rawMouse + 3)) {
            state.move(-1, entries);
        } else if (game::at<uint8_t>(rawMouse + 4)) {
            state.move(1, entries);
        }
    }
    dockPointer(state, entries, W, H, dt);
}
