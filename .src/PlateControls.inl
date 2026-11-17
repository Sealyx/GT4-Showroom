void processStudioPlate(const PadActions& pad, float width, float height, double dt, uint64_t tick,
                        bool leftPress, bool rightPress, bool upPress, bool downPress,
                        bool accept, bool goBack, bool click, bool moved, bool add, bool context)
{
    if (state.plateAction >= 0) {
        return;
    }
    auto& kb = state.ui.keyboard;
    const auto choose = [&](int row) {
        if (row == 0) {
            state.plateEditing = true;
            kb.open = pad.accept;
            state.cue = Cue::Accept;
        } else if (row == 1 || row == 3) {
            state.finishPlate(row == 3);
        } else if (row == 2) {
            static std::mt19937 random{std::random_device{}()};
            state.plate = randomPlate(random);
            state.cue = Cue::Accept;
        }
        blockActions();
    };
    if (context && pad.view) {
        kb.open = !kb.open;
        state.plateEditing = kb.open;
        state.plateRow = kb.open ? 0 : 3;
        state.cue = Cue::Open;
        blockActions();
        return;
    }
    const PlatePanel panel(width, height, kb.open, state.plateMotion.offset(state.clock, height));
    const float x = state.pointerX * width, y = state.pointerY * height;
    if (moved || click || (!state.ui.dock && !state.ui.controllerUsed)) {
        state.ui.dock = false;
        state.ui.controllerUsed = false;
        const int target = panel.hit(x, y);
        if (target >= 0 && target != state.plateHover) {
            state.cue = Cue::Move;
        }
        state.plateHover = target;
        if (click && target >= 0) {
            state.plateRow = target;
            if (target == 0) {
                kb.open = false;
            }
            choose(target);
            return;
        }
    }
    if (kb.open) {
        if (kb.changeCharacters(pad.previousHotspot, pad.nextHotspot, tick)) {
            state.cue = Cue::Move;
        }
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
            action = kb.type(state.plate, 8);
        }
        if (add && !state.plate.empty()) {
            state.plate.pop_back();
        }
        if (click) {
            for (int i = 0; i < 40; ++i) {
                if (kb.box(width, height, i).contains(x, y)) {
                    kb.selected = i;
                    action = kb.type(state.plate, 8);
                    break;
                }
            }
        }
        if (action || (goBack && pad.back)) {
            state.finishPlate(action == 1);
            blockActions();
            return;
        }
    } else {
        if (leftPress || rightPress || upPress || downPress) {
            const int direction = rightPress || downPress ? 1 : -1;
            state.plateRow = (state.plateRow + direction + 4) % 4;
            state.plateEditing = false;
            state.cue = Cue::Move;
        }
        if (accept && (!state.plateEditing || pad.accept)) {
            choose(state.plateRow);
            return;
        }
        if ((goBack && pad.back) || (context && !pad.view)) {
            state.finishPlate(false);
            blockActions();
            return;
        }
    }
    // an empty focused field still owns Backspace; it must not unexpectedly leave this prompt
    const auto action = searchInput.poll([](int vk) { return key(vk); }, tick,
                                         state.plate, state.plateEditing, 8);
    state.plate = plateText(state.plate);
    if (action == SearchAction::Submit && state.plateEditing) {
        state.finishPlate(true);
        blockActions();
        return;
    }
    if (action == SearchAction::Cancel && !state.plateEditing) {
        state.finishPlate(false);
        blockActions();
        return;
    }
    dockPointer(state, entries, width, height, dt);
}
