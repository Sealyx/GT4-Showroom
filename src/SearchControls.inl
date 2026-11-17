// share the layout with drawing; decorative labels never become click targets
void processStudioSearch(const PadActions& pad, float width, float height, double dt, uint64_t tick,
                         bool leftPress, bool rightPress, bool upPress, bool downPress,
                         bool accept, bool goBack, bool click, bool moved, bool add, bool favoritesPress, bool context)
{
    auto& kb = state.ui.keyboard;
    if (context && pad.view) {
        kb.open = !kb.open;
        state.searchEditing = kb.open;
        state.searchRow = kb.open ? 1 : 0;
        state.searchPicker = false;
        state.cue = Cue::Open;
        blockActions();
        return;
    }
    if (context && !pad.view) {
        if (state.searchPicker) {
            state.searchPicker = false;
            state.cue = Cue::Back;
        } else {
            state.closeSearch();
        }
        blockActions();
        return;
    }
    const float scale = SearchPanel(width, height, kb.open).scale;
    const SearchPanel panel(width, height, kb.open, state.searchOffset(scale));
    const float x = state.pointerX * width, y = state.pointerY * height;
    const auto submit = [&] {
        state.searchPicker = false;
        submitStudioSearch();
    };
    const auto close = [&] {
        state.closeSearch();
        blockActions();
    };
    const auto chooseMode = [&] {
        state.searchById = state.searchModeCursor == 1;
        state.searchPicker = false;
        state.cue = Cue::Accept;
        blockActions();
    };
    if (click || moved || (!state.ui.dock && !state.ui.controllerUsed)) {
        state.ui.dock = false;
        state.ui.controllerUsed = false;
        const int target = panel.hit(x, y);
        if (moved && target != state.searchHover && target >= 0) {
            state.cue = Cue::Move;
        }
        state.searchHover = target;
        if (state.searchPicker) {
            for (int i = 0; i < 2; ++i) {
                if (panel.option(i).contains(x, y)) {
                    state.searchModeCursor = i;
                    if (click) {
                        chooseMode();
                        return;
                    }
                    break;
                }
            }
        }
        if (click && target >= 0) {
            state.searchRow = target;
            state.searchEditing = target == 1;
            kb.open = false;
            if (target == 0) {
                state.searchPicker = !state.searchPicker;
                state.searchModeCursor = state.searchById ? 1 : 0;
            } else if (target == 2) {
                close();
                return;
            } else if (target == 3 || (target == 1 && monitorArrowPanel(panel.field(1), scale).contains(x, y))) {
                submit();
                return;
            } else {
                state.searchPicker = false;
            }
            state.cue = Cue::Accept;
            blockActions();
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
                if (kb.box(width, height, i).contains(x, y)) {
                    kb.selected = i;
                    action = kb.type(state.query);
                    break;
                }
            }
        }
        if (action == 1) {
            submit();
            return;
        }
        if (action < 0) {
            close();
            return;
        }
    } else if (state.searchPicker) {
        if (goBack && pad.back) {
            state.searchPicker = false;
            state.cue = Cue::Back;
            blockActions();
            return;
        }
        if (leftPress || rightPress || upPress || downPress) {
            state.searchModeCursor = 1 - state.searchModeCursor;
            state.cue = Cue::Move;
        }
        if (accept) {
            chooseMode();
            return;
        }
    } else {
        if (upPress || downPress) {
            const int direction = downPress ? 1 : -1;
            state.searchRow = (state.searchRow + direction + 4) % 4;
            state.searchEditing = state.searchRow == 1;
            state.searchHover = state.searchRow;
            state.cue = Cue::Move;
        }
        if ((leftPress || rightPress) && state.searchRow == 0) {
            state.searchById = !state.searchById;
            state.cue = Cue::Move;
        } else if ((leftPress || rightPress) && state.searchRow >= 2) {
            state.searchRow = state.searchRow == 2 ? 3 : 2;
            state.searchHover = state.searchRow;
            state.searchEditing = false;
            state.cue = Cue::Move;
        }
        if (favoritesPress && pad.favorites) {
            submit();
            return;
        }
        if (accept && (!state.searchEditing || pad.accept)) {
            if (state.searchRow == 0) {
                state.searchPicker = true;
                state.searchModeCursor = state.searchById ? 1 : 0;
                state.cue = Cue::Open;
            } else if (state.searchRow == 1 && pad.accept) {
                kb.open = true;
                state.cue = Cue::Open;
            } else if (state.searchRow == 2) {
                close();
            } else {
                submit();
            }
            blockActions();
            return;
        }
        if (goBack && pad.back) {
            close();
            return;
        }
    }
    const auto action = searchInput.poll([](int keyCode) { return key(keyCode); }, tick,
                                         state.query, state.searchEditing && !state.searchPicker);
    if (action == SearchAction::Submit && state.searchEditing && !state.searchPicker) {
        submit();
        return;
    }
    if (action == SearchAction::Cancel) {
        if (state.searchPicker) {
            state.searchPicker = false;
            state.cue = Cue::Back;
            blockActions();
        } else {
            close();
        }
        return;
    }
    dockPointer(state, entries, width, height, dt);
}
