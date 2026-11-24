void invalidateStudioExtraPreview(int id)
{
    dirtyExtraModel = id;
    failed.erase(std::remove(failed.begin(), failed.end(), id), failed.end());
    pending.erase(id);
    liveryNative::capturedLivery = -2;
}

bool selectStudioExtra(int group, int option)
{
    if (entries.empty() || state.selected >= entries.size()) {
        return false;
    }
    const auto before = nativeExtras::choice;
    if (!nativeExtras::select(group, option)) {
        return false;
    }
    const int id = entries[state.selected].id;
    state.ui.menu.extraChoice = nativeExtras::choice;
    state.ui.menu.setExtras(nativeExtras::labels(0), nativeExtras::labels(1), nativeExtras::labels(2));
    selectedExtras[id] = state.ui.menu.extraChoice;
    if (before != nativeExtras::choice) {
        invalidateStudioExtraPreview(id);
    }
    return true;
}

void syncStudioExtraPreview()
{
    if (state.appearanceResetPending) {
        dealerShopVisited = false;
        state.appearanceResetPending = false;
        selectedExtras.clear();
    }
    if (entries.empty() || state.selected >= entries.size() || extrasModel != entries[state.selected].id) {
        return;
    }
    const bool initialize = state.extrasResetPending && nativeExtras::metadataReady;
    if (initialize) {
        state.extrasResetPending = false;
        selectedExtras.erase(extrasModel);
        for (int group = 0; group < 2; ++group) {
            const auto& options = nativeExtras::labels(group);
            state.ui.menu.extraChoice[size_t(group)] =
                options.size() > 2 ? 2 : 0;
        }
        state.ui.menu.extraChoice[2] = 0;
    }
    const auto wanted = state.ui.menu.extraChoice;
    const auto before = nativeExtras::choice;
    for (int group = 0; group < 3; ++group) {
        if (wanted[size_t(group)] != before[size_t(group)]) {
            nativeExtras::select(group, wanted[size_t(group)]);
        }
    }
    if (!state.skipSelectionExtras) {
        state.ui.menu.setExtras(nativeExtras::labels(0), nativeExtras::labels(1), nativeExtras::labels(2));
    }
    if (before != nativeExtras::choice) {
        invalidateStudioExtraPreview(extrasModel);
    }
}
