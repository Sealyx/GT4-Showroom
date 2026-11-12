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
        ui.applyPreferences(ui.menuPreferences);
        ui.shadows = ui.menuShadows;
        for (size_t group = 0; group < ui.menuExtras.size(); ++group) {
            if (ui.menu.extraChoice[group] != ui.menuExtras[group]) {
                selectStudioExtra(int(group), ui.menuExtras[group]);
            }
        }
        ui.menu.extraChoice = ui.menuExtras;
        state.viewer = state.menuViewer;
        state.viewer.lastActivity = state.clock;
    } else if (ui.preferences() != ui.menuPreferences) {
        saveStudioPreferences();
    }
    ui.menuSnapshot = false;
}

#include "TransFenderControls.inl"

void requestStudioAction(StudioAction action)
{
    if (action == StudioAction::None || state.busy()) {
        return;
    }
    if (action == StudioAction::EnterPhoto) {
        if ((state.ui.cinematic.active || state.viewer.active) && !state.viewer.transitioning()) {
            state.ui.menu.finish(1);
            finishSettings();
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
            viewer.settings.scene = ViewerScene(value);
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
    const bool persistentView = (action >= StudioAction::SortAscending && action <= StudioAction::SortModelId) ||
                                action == StudioAction::FilterCountry || action == StudioAction::FilterClass;
    if (action >= StudioAction::SortAscending && action <= StudioAction::SortModelId) {
        state.ui.sortMode = int(action) - int(StudioAction::SortAscending);
        studioConfig.sort = menuSort(state.ui.sortMode);
    } else if (action == StudioAction::FilterCountry) {
        state.ui.countryFilter = state.ui.menu.choice;
    } else if (action == StudioAction::FilterClass) {
        state.ui.classFilter = state.ui.menu.choice;
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
    state.ui.menu.hotspots = state.hotspotsEnabled();
    state.ui.menu.viewerSettings = state.viewer.settings;
    state.ui.menu.viewerPaused = state.viewer.paused;
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
    state.pointerMove(dx * mouseSensitivity, dy * mouseSensitivity, W, H);
    const bool click = mouseLeft.poll(lDown, tick), context = mouseRight.poll(rDown || pad.view || key(VK_APPS), tick);
    const bool search = searchKey.poll(key(VK_OEM_3) || pad.search, tick);
    const bool leftPress = left.poll(key(VK_LEFT) || pad.left, tick, true),
               rightPress = right.poll(key(VK_RIGHT) || pad.right, tick, true);
    const bool modal = state.ui.menu.open || state.shop.open() || state.photo.active || state.searching ||
                       state.spawnReview != SpawnReview::None;
    const bool upPress = up.poll(key(VK_UP) || pad.up, tick, modal),
               downPress = down.poll(key(VK_DOWN) || pad.down, tick, modal);
    const bool accept = enter.poll(key(VK_RETURN) || key(VK_SPACE) || pad.accept, tick),
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
    if (state.photo.active) {
        auto& photo = state.photo;
        if (key('H') || key('W') || key('A') || key('S') || key('D') || key('Q') || key('E') || key(VK_OEM_PLUS) ||
            key(VK_OEM_MINUS)) {
            state.ui.controllerUsed = false;
        }
        if ((!state.ui.cinematic.active && !state.viewer.active) || goBack) {
            photo.active = photo.requested = false;
            photo.dragging = photo.movingPanel = false;
            state.cue = Cue::Back;
            blockActions();
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
                photo.begin(state.viewer, state.clock);
            } else {
                photo.beginCinematic(state.clock);
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
                photo.row = photo.row == 4 ? 5 : 4;
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
            } else if (photo.row == 4) {
                photo.requested = true;
                blockActions();
            } else if (photo.row == 5) {
                photo.active = false;
                state.cue = Cue::Back;
                blockActions();
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
            state.reviewChoice = (state.reviewChoice + buttonCount - 1) % buttonCount;
            state.cue = Cue::Move;
        }
        if (rightPress || downPress) {
            state.reviewChoice = (state.reviewChoice + 1) % buttonCount;
            state.cue = Cue::Move;
        }
        if (moved || click) {
            for (int i = 0; i < buttonCount; ++i) {
                auto button = spawnReviewButton(W, H, i, buttonCount);
                button.y += state.reviewOffset(H);
                if (button.contains(state.pointerX * W, state.pointerY * H)) {
                    state.reviewChoice = i;
                    if (click) {
                        state.chooseSpawnReview(static_cast<ReviewChoice>(i));
                        blockActions();
                        return;
                    }
                }
            }
        }
        if (accept) {
            state.chooseSpawnReview(static_cast<ReviewChoice>(state.reviewChoice));
            blockActions();
        } else if (goBack || context) {
            state.back();
            blockActions();
        }
        if (state.ui.dock) {
            auto target = spawnReviewButton(W, H, state.reviewChoice, buttonCount);
            target.y += state.reviewOffset(H);
            state.pointerX = (target.x + target.w * .5f) / W;
            state.pointerY = (target.y + target.h * .5f) / H;
        }
        return;
    }
    if (state.ui.cinematic.active && !state.ui.menu.open && photoHide.poll(key('H') || pad.search, tick)) {
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
        if (state.ui.keyboard.open) {
            auto& kb = state.ui.keyboard;
            if (leftPress) {
                kb.move(-1, 0);
            }
            if (rightPress) {
                kb.move(1, 0);
            }
            if (upPress) {
                kb.move(0, -1);
            }
            if (downPress) {
                kb.move(0, 1);
            }
            int action = 0;
            if (accept && pad.accept) {
                action = kb.type(state.query);
            }
            if (add && !state.query.empty()) {
                state.query.pop_back();
            }
            if (favoritesPress) {
                action = 1;
            }
            if (goBack && pad.back) {
                action = -1;
            }
            if (click) {
                for (int i = 0; i < 40; ++i) {
                    if (kb.box(W, H, i).contains(state.pointerX * W, state.pointerY * H)) {
                        kb.selected = i;
                        action = kb.type(state.query);
                        break;
                    }
                }
            }
            if (action == 1) {
                submitStudioSearch();
                return;
            }
            if (action < 0) {
                state.closeSearch();
                kb.open = false;
                blockActions();
                return;
            }
        } else if (goBack && pad.back) {
            state.closeSearch();
            blockActions();
            return;
        }
        const auto action = searchInput.poll(
            [](int v) {
                return key(v);
            },
            tick,
            state.query);
        if (action == SearchAction::Submit) {
            submitStudioSearch();
        } else if (action == SearchAction::Cancel) {
            state.closeSearch();
            state.ui.keyboard.open = false;
            blockActions();
        }
        dockPointer(state, entries, W, H, dt);
        return;
    }
    if (context && state.page != Page::Prize) {
        auto& ui = state.ui;
        const bool opened = !ui.menu.open;
        if (opened) {
            state.menuViewer = state.viewer;
            ui.menuCinematic = ui.cinematic;
            ui.menuVehicle = ui.vehicle;
            ui.menuPreferences = ui.preferences();
            ui.menuShadows = ui.shadows;
            ui.menuExtras = ui.menu.extraChoice;
            ui.menuSnapshot = true;
            ui.menu.settingsView = SettingsView::Showroom;
            if (state.viewer.active) {
                ui.menu.settingsView = SettingsView::VehicleViewer;
            } else if (ui.cinematic.active) {
                ui.menu.settingsView = SettingsView::Cinematic;
            }
            ui.menu.toggle(state.pointerX, state.pointerY, state.clock);
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
        if (togglePanels && mesh) {
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
