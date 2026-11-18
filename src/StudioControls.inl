inline Key previousCinematicHotspot, nextCinematicHotspot, pauseCinematic, photoHide, photoReset;

void finishSettings()
{
    auto& ui = state.ui;
    if (!ui.menuSnapshot || ui.menu.open) {
        return;
    }
    if (ui.menu.closeResult < 0) {
        ui.cinematic = ui.menuCinematic;
        ui.vehicle = ui.menuVehicle;
        const bool changed = ui.preferences() != ui.menuPreferences;
        ui.applyPreferences(ui.menuPreferences);
        localization.select(ui.language);
        if (changed) {
            saveStudioPreferences();
        }
        rebuildCatalogueView();
        refreshSelected();
        ui.shadows = ui.menuShadows;
        for (size_t group = 0; group < ui.menuExtras.size(); ++group) {
            if (ui.menu.extraChoice[group] != ui.menuExtras[group]) {
                selectStudioExtra(int(group), ui.menuExtras[group]);
            }
        }
        ui.menu.extraChoice = ui.menuExtras;
        if (ui.menu.volume != ui.menuVolume) {
            setGameVolume(ui.menuVolume);
            ui.menu.volume = ui.menuVolume;
        }
        state.viewer = state.menuViewer;
        state.viewer.lastActivity = state.clock;
    } else if (ui.preferences() != ui.menuPreferences) {
        saveStudioPreferences();
    }
    ui.menuSnapshot = false;
}

#include "TransFenderControls.inl"

void openStudioSettings(float x, float y, bool keyboard)
{
    auto& ui = state.ui;
    if (state.selected < entries.size()) {
        const auto& entry = entries[state.selected];
        ui.menu.setVehicleKind(entry.kind, entry.classId);
    }
    state.menuViewer = state.viewer;
    ui.menuCinematic = ui.cinematic;
    ui.menuVehicle = ui.vehicle;
    ui.menuPreferences = ui.preferences();
    ui.menuShadows = ui.shadows;
    ui.menuExtras = ui.menu.extraChoice;
    readGameVolume(ui.menu.volume);
    ui.menuVolume = ui.menu.volume;
    ui.menuSnapshot = true;
    ui.menu.settingsView = SettingsView::Showroom;
    if (state.viewer.active) {
        ui.menu.settingsView = SettingsView::VehicleViewer;
    } else if (ui.cinematic.active) {
        ui.menu.settingsView = SettingsView::Cinematic;
    }
    ui.menu.toggle(x, y, state.clock);
    ui.menu.monitorKeyboard = keyboard;
}

// returning to settings ends photo rendering but keeps its separate camera for the next visit
void closeStudioPhoto(float width, float height)
{
    auto& photo = state.photo;
    const auto panel = photo.panel(width, height);
    photo.end(state.viewer, state.clock);
    if (state.ui.cinematic.active || state.viewer.active) {
        openStudioSettings(panel.x / width, panel.y / height, true);
        state.ui.dock = true;
    }
    state.cue = Cue::Back;
    blockActions();
}

void requestStudioAction(StudioAction action)
{
    if (action == StudioAction::None || state.busy()) {
        return;
    }
    if (action == StudioAction::Language) {
        if (localization.select(state.ui.menu.choice)) {
            state.ui.language = localization.selected;
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
        auto& quality = action == StudioAction::FloorReflections ? state.ui.floorReflections : state.ui.vehicleReflections;
        quality = std::clamp(state.ui.menu.choice, 0, 4);
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
    if (action == StudioAction::Background) {
        state.ui.backgroundSelection = std::clamp(state.ui.menu.choice, 0, 2);
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
        auto& mask = country ? ui.countryMask : ui.classMask;
        auto& legacy = country ? ui.countryFilter : ui.classFilter;
        const int choice = ui.menu.choice;
        if (choice < 0) {
            mask = 0;
        } else {
            if (!mask && legacy >= 0) {
                mask = 1u << legacy;
            }
            mask ^= 1u << choice;
        }
        legacy = -1;
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
        if ((state.ui.cinematic.active || state.viewer.active) && !state.viewer.transitioning()) {
            state.ui.menu.finish(1);
            finishSettings();
            state.photo.loadPresets(std::filesystem::path(base) / L"GT4Showroom/photo-presets.dat");
            if (state.viewer.active) {
                state.photo.begin(state.viewer, state.clock);
            } else {
                state.photo.beginCinematic(state.clock);
            }
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
        state.ui.menu.finish(1);
        finishSettings();
        state.shop.fromViewer = state.viewer.active;
        state.shop.request(ShopPage::Menu, state.clock);
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::ViewerEnter || action == StudioAction::Cinematic) {
        state.ui.menu.finish(1);
        finishSettings();
        const bool started = action == StudioAction::ViewerEnter ? state.startViewer() : state.startCinematic();
        state.cue = started ? Cue::Accept : Cue::Error;
        blockActions();
        return;
    }
    if (action == StudioAction::ViewerAnimate || action == StudioAction::ViewerScene ||
        action == StudioAction::ViewerColor || action == StudioAction::ViewerFloor ||
        action == StudioAction::ViewerPalette || action == StudioAction::ViewerReflections ||
        action == StudioAction::ViewerShadows || action == StudioAction::ViewerPause) {
        auto& viewer = state.viewer;
        const int value = state.ui.menu.choice;
        if (action == StudioAction::ViewerAnimate) {
            viewer.settings.animate = value != 0;
            viewer.syncBackground(state.clock);
        } else if (action == StudioAction::ViewerScene) {
            viewer.changeScene(ViewerScene(value), state.clock);
        } else if (action == StudioAction::ViewerColor) {
            viewer.settings.wallColor = value;
        } else if (action == StudioAction::ViewerFloor) {
            viewer.settings.floorColor = value;
        } else if (action == StudioAction::ViewerPalette) {
            viewer.settings.palette = value;
        } else if (action == StudioAction::ViewerReflections) {
            viewer.settings.reflections = value != 0;
        } else if (action == StudioAction::ViewerShadows) {
            viewer.settings.shadows = value != 0;
        } else if (viewer.paused != (value != 0)) {
            viewer.toggle(state.clock);
        }
        viewer.settings.validate();
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (action == StudioAction::Shadows || action == StudioAction::CarLogo || action == StudioAction::BrandLogo ||
        action == StudioAction::DisplayModelId || action == StudioAction::DisplayModelName) {
        const bool value = state.ui.menu.choice != 0;
        if (action == StudioAction::Shadows) {
            state.ui.shadows = value;
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
    const int extraGroup = extraActionGroup(action);
    if (extraGroup >= 0) {
        const bool applied = selectStudioExtra(extraGroup, state.ui.menu.choice);
        state.ui.menu.open = true;
        state.ui.menu.selectedAction();
        state.cue = applied ? Cue::Accept : Cue::Error;
        blockActions();
        return;
    }
    if (action == StudioAction::Hotspots) {
        if (!state.ui.menu.allowHotspots) {
            return;
        }
        auto& visible = state.viewer.active ? state.viewer.hotspots : state.ui.vehicle.hotspots;
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
    state.viewer.reset(state.clock);
    state.viewerSnapshot = state.postSelectionCinematic = false;
    state.spawnReview = SpawnReview::None;
    state.page = Page::Cars;
    state.paint = state.livery = 0;
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
    if (entries.empty() || state.selected >= entries.size() || state.page != Page::Cars || state.busy()) {
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
    WritePrivateProfileStringW(
        L"Favorites", std::to_wstring(e.id).c_str(), add ? L"1" : L"0", (base + L"GT4Showroom\\favorites.ini").c_str());
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
#include "PlateControls.inl"

void interaction(const PadActions& pad, float W, float H, double dt, uint64_t tick)
{
    if (!entries.empty() && state.selected < entries.size()) {
        state.ui.vehicle.bind(entries[state.selected].id);
    }
    applyViewTransition();
    if (state.shopResetGroup >= 0) {
        resetShopSelection(state.shopResetGroup == 0);
        state.shopResetGroup = -1;
    }
    state.ui.menu.sync(state.ui.sortMode,
                       state.ui.countryFilter,
                       state.ui.classFilter,
                       state.ui.shadows,
                       state.ui.carLogo,
                       state.ui.brandLogo,
                       state.ui.displayModelId,
                       state.ui.displayModelName);
    state.ui.menu.countryMask = state.ui.countryMask;
    state.ui.menu.classMask = state.ui.classMask;
    state.ui.menu.antialiasing = state.ui.antialiasing;
    state.ui.menu.ambientOcclusion = state.ui.ambientOcclusion;
    state.ui.menu.floorReflections = state.ui.floorReflections;
    state.ui.menu.vehicleReflections = state.ui.vehicleReflections;
    state.ui.menu.language = state.ui.language;
    state.ui.menu.unitFormat = state.ui.unitFormat;
    state.ui.menu.setCategories(state.ui.japaneseCategory(), state.ui.bikeCategory());
    state.ui.menu.backgroundChoice = state.ui.backgroundSelection;
    state.ui.menu.hotspots = state.hotspotsEnabled();
    state.ui.menu.viewerSettings = state.viewer.settings;
    state.ui.menu.viewerPaused = state.viewer.paused;
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
    const bool modal = state.ui.menu.open || state.shop.open() || state.photo.active || state.searching ||
                       state.spawnReview != SpawnReview::None || state.plateOpen || state.informationOpen;
    const bool upPress = up.poll(key(VK_UP) || pad.up, tick, modal),
               downPress = down.poll(key(VK_DOWN) || pad.down, tick, modal);
    const bool editingText = (state.plateOpen && state.plateEditing) || (state.searching && state.searchEditing);
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
        if ((!state.ui.cinematic.active && !state.viewer.active) || goBack) {
            closeStudioPhoto(W, H);
            return;
        }
        if (photo.requested) {
            photo.dragging = photo.movingPanel = false;
            return;
        }
        if (photoHide.poll(key('H') || pad.search, tick)) {
            photo.hidden = !photo.hidden;
            state.cue = photo.hidden ? Cue::Back : Cue::Open;
            blockActions();
            return;
        }
        if (photoReset.poll(key('R') || pad.favorites, tick)) {
            const float panelX = photo.panelX, panelY = photo.panelY;
            if (state.viewer.active) {
                photo.begin(state.viewer, state.clock, false);
            } else {
                photo.beginCinematic(state.clock, false);
            }
            photo.panelX = panelX;
            photo.panelY = panelY;
            state.cue = Cue::Accept;
            blockActions();
            return;
        }
        if (accept && key(VK_SPACE)) {
            photo.requested = true;
            photo.dragging = photo.movingPanel = false;
            blockActions();
            return;
        }
        if (photo.dragging) {
            if (lDown) {
                photo.place(state.pointerX * W - photo.dragX, state.pointerY * H - photo.dragY, W, H);
            } else {
                photo.dragging = false;
            }
            state.ui.dock = false;
            return;
        }
        if (!photo.hidden && !rDown && click && photo.header(W, H).contains(state.pointerX * W, state.pointerY * H)) {
            const auto box = photo.panel(W, H);
            photo.dragX = state.pointerX * W - box.x;
            photo.dragY = state.pointerY * H - box.y;
            photo.dragging = true;
            state.ui.dock = false;
            state.cue = Cue::Move;
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
        if (!movePanel && (upPress || downPress)) {
            photo.row = (photo.row + int(downPress) - int(upPress) + ViewerPhoto::rowCount) % ViewerPhoto::rowCount;
            state.cue = Cue::Move;
        }
        const int direction = int(rightPress) - int(leftPress);
        if (!movePanel && direction) {
            if (photo.row < ViewerPhoto::settingCount) {
                state.cue = photo.adjust(direction) ? Cue::Slider : Cue::Error;
            } else {
                photo.row = photo.row == ViewerPhoto::captureRow ? ViewerPhoto::backRow : ViewerPhoto::captureRow;
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
        bool activate = accept;
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
                activate = activate || (click && (hit >= ViewerPhoto::settingCount || clickedDirection));
            }
        }
        if (activate) {
            if (photo.row < ViewerPhoto::settingCount) {
                state.cue = photo.adjust(clickedDirection ? clickedDirection : photo.side) ? Cue::Slider : Cue::Error;
            } else if (photo.row == ViewerPhoto::saveRow) {
                const bool stored = photo.storePreset() &&
                    photo.savePresets(std::filesystem::path(base) / L"GT4Showroom/photo-presets.dat");
                photo.status = stored ? "Preset saved" : "Preset could not be saved";
                photo.statusUntil = state.clock + 3;
                state.cue = stored ? Cue::Accept : Cue::Error;
                blockActions();
            } else if (photo.row == ViewerPhoto::loadRow) {
                const bool loaded = photo.restorePreset(state.viewer, state.clock);
                photo.status = loaded ? "Preset loaded" : "This preset is empty";
                photo.statusUntil = state.clock + 3;
                state.cue = loaded ? Cue::Accept : Cue::Error;
                blockActions();
            } else if (photo.row == ViewerPhoto::captureRow) {
                photo.requested = true;
                blockActions();
            } else if (photo.row == ViewerPhoto::backRow) {
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
        state.viewer.activity(state.clock);
        const bool wheelLeft = wheel && game::at<uint8_t>(rawMouse + 3);
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
    if (activeInput && state.viewer.activity(state.clock)) {
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
            state.back();
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
    if (search) {
        if (state.ui.menu.open) {
            state.ui.menu.finish(-1);
            finishSettings();
        }
        if (state.searching) {
            state.closeSearch();
            state.ui.keyboard.open = false;
        } else {
            state.beginSearch();
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
            if (!m.sliderChanged) {
                return;
            }
            if (m.childBranch() == MenuBranch::Extras) {
                const int group = m.extraGroup(m.sortRow);
                if (group >= 0 && !selectStudioExtra(group, m.extraChoice[size_t(group)])) {
                    state.cue = Cue::Error;
                }
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
        auto& viewer = state.viewer;
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
        const bool hasHotspots = mesh && state.hotspotsEnabled() && mesh->cinematic.any();
        if (hasHotspots && (previousHotspot || nextHotspot)) {
            const auto points = renderer.hotspots(*mesh, v);
            v.focus = points.cycle(v.focus, previousHotspot ? -1 : 1);
            state.ui.dock = true;
            state.ui.pointerMoved = state.clock;
        }
        const bool wheelFocused = hasHotspots && v.focus >= 6 && mesh->cinematic.supported(v.focus);
        const auto toggleRotation = [&] {
            if (viewer.active) {
                viewer.toggle(state.clock);
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
        if (viewer.active) {
            if (click && !hotspotClicked && mesh && renderer.hitCar(*mesh, state.pointerX * W, state.pointerY * H)) {
                viewer.toggle(state.clock);
            }
            if ((lDown && !hotspotClicked) || rDown) {
                viewer.orbit(dx * .007f, dy * .004f, state.clock);
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
        if (viewer.active) {
            moveCamera(viewer);
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
        if (click && entry.country != Country::Unknown &&
            UIRect{46 * scale, 33 * scale, 34 * scale, 28 * scale}.contains(x, y)) {
            auto& ui = state.ui;
            if (!ui.countryShortcut) {
                ui.shortcutCountry = ui.countryFilter;
                ui.shortcutMask = ui.countryMask;
                ui.shortcutOrder = ui.sortMode;
                ui.shortcutModel = entry.id;
            }
            ui.countryShortcut = true;
            ui.countryFilter = int(entry.country);
            ui.countryMask = 0;
            ui.sortMode = 4;
            studioConfig.sort = menuSort(4);
            rebuildCatalogueView(entry.id);
            refreshSelected();
            state.cue = Cue::Accept;
            blockActions();
            return;
        }
        if (click && classIndex(entry.classId) == 5 &&
            UIRect{104 * scale, 19 * scale, 170 * scale, 34 * scale}.contains(x, y)) {
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
    if (favoritesPress && state.page != Page::Prize) {
        requestStudioAction(state.ui.favoritesOnly ? StudioAction::AllCars : StudioAction::Favorites);
    }
    if (upPress || add) {
        favoriteSelection(true);
    }
    if (downPress) {
        favoriteSelection(false);
    }
    pointerButton(state, entries, W, H, lDown, click);
    if (!state.dragging) {
        if (game::at<uint8_t>(rawMouse + 3)) {
            state.move(-1, entries);
        } else if (game::at<uint8_t>(rawMouse + 4)) {
            state.move(1, entries);
        }
    }
    dockPointer(state, entries, W, H, dt);
}
