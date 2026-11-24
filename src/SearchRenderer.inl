void messageWindow(IDirect3DDevice9* device, UIRect box, const std::string& title, float scale, float alpha)
{
    rect(device, box.x + 3 * scale, box.y + 3 * scale, box.w, box.h, color(0, .24f * alpha));
    rect(device, box.x, box.y, box.w, box.h, color(0xB6B6BB, alpha));
    rect(device, box.x, box.y, box.w, 27 * scale, color(0x17130A, alpha));
    text(device, title, box.x + box.w * .5f, box.y + 4 * scale,
         fitted(title, 15 * scale, box.w - 24 * scale, true), color(0xD0D0D2, alpha), true, 0, true);
}

UIRect messageRow(IDirect3DDevice9* device, UIRect box, int index, const std::string& label,
                 const std::string& value, float scale, float alpha, bool field = true,
                 bool regular = false, bool dropdown = false, bool selected = false)
{
    const float y = box.y + (28 + index * 34) * scale;
    rect(device, box.x, y, box.w, 34 * scale, color(index % 2 ? 0xACACB1 : 0xB6B6BB, alpha));
    text(device, label, box.x + 15 * scale, y + 7 * scale,
         fitted(label, 14.5f * scale, box.w * .36f - 24 * scale, true), color(0x08080C, alpha), false, 0, true);
    const UIRect input{box.x + box.w * .36f, y + 5 * scale, box.w * .61f, 24 * scale};
    if (field) {
        monitorBevel(device, input, alpha, scale, selected, false, dropdown);
    }
    const auto style = regular ? infoStyle(device) : textStyle(true);
    const float size = (regular ? 12.25f : 14.5f) * scale;
    const float available = input.w - (dropdown ? monitorArrowPanel(input, scale).w : 0) - 18 * scale;
    text(device, value, input.x + 9 * scale, input.y + (regular ? 5 : 4) * scale,
         fitted(value, size, available, style), color(0x08080C, alpha), false, 0, style);
    return input;
}

void searchIcon(IDirect3DDevice9* device, UIRect box, float scale, float alpha)
{
#ifndef GT4_ICON_EXPORT
    if(pngIcon(device,L"search.png",{box.x+2*scale,box.y+2*scale,box.w-4*scale,box.h-4*scale},alpha)) return;
#endif
    if (!searchGlyph) {
        constexpr unsigned size = 128;
        const auto pixels = magnifierPixels(size);
        bool mips = SUCCEEDED(device->CreateTexture(size, size, 0, D3DUSAGE_AUTOGENMIPMAP,
                                                    D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &searchGlyph, nullptr));
        if (!mips && FAILED(device->CreateTexture(size, size, 1, 0, D3DFMT_A8R8G8B8,
                                                  D3DPOOL_MANAGED, &searchGlyph, nullptr))) {
            return;
        }
        D3DLOCKED_RECT lock{};
        if (!searchGlyph || FAILED(searchGlyph->LockRect(0, &lock, nullptr, 0))) {
            if (searchGlyph) {
                searchGlyph->Release();
                searchGlyph = nullptr;
            }
            return;
        }
        for (unsigned y = 0; y < size; ++y) {
            std::memcpy(static_cast<uint8_t*>(lock.pBits) + size_t(y) * lock.Pitch,
                        pixels.data() + size_t(y) * size, size * sizeof(uint32_t));
        }
        searchGlyph->UnlockRect(0);
        if (mips) {
            searchGlyph->GenerateMipSubLevels();
        }
    }
    rect(device, box.x + 2 * scale, box.y + 2 * scale, box.w - 4 * scale, box.h - 4 * scale,
         color(0xFFFFFF, alpha), searchGlyph);
}

void drawSearch(IDirect3DDevice9* device, float width, float height, const Model& model)
{
    const float alpha = model.searchOpacity();
    const bool keyboard = model.searching ? model.ui.keyboard.open : model.searchKeyboard;
    const float scale = SearchPanel(width, height, keyboard).scale;
    const SearchPanel layout(width, height, keyboard, model.searchOffset(scale));
    const auto box = layout.box;
    const bool dock = model.ui.dock || model.ui.controllerUsed;
    const int focus = dock ? model.searchRow : layout.hit(model.pointerX * width, model.pointerY * height);
    messageWindow(device, box, "Search", scale, alpha);
    messageRow(device, box, 0, "Search By", model.searchById ? "Model ID" : "Model Name",
               scale, alpha, true, false, true, focus == 0);
    const auto field = messageRow(device, box, 1, "Keyword", "", scale, alpha);
    const auto icon = monitorArrowPanel(field, scale);
    monitorBevel(device, icon, alpha, scale, false, false, false);
    searchIcon(device, icon, scale, alpha);
    const float inputSize = fitted(model.query, 14.5f * scale, icon.x - field.x - 18 * scale, true);
    text(device, model.query, field.x + 9 * scale, field.y + 4 * scale,
         inputSize, color(0x08080C, alpha), false, 0, true);
    if (model.searchEditing && int(model.clock * 2) % 2 == 0) {
        const float x = field.x + 9 * scale + textWidth(model.query, inputSize, true);
        rect(device, x + scale, field.y + 4 * scale, scale, 15 * scale, color(0x08080C, alpha));
    }
    const std::string info = model.searchById ? "Search by model ID, e.g. 411."
                                             : "Search by model name, e.g. infernus.";
    messageRow(device, box, 2, "Info", info, scale, alpha, false, true);
    rect(device, box.x + 12 * scale, box.y + 132 * scale, box.w - 24 * scale, scale, color(0x96979D, alpha));
    rect(device, box.x + 12 * scale, box.y + 133 * scale, box.w - 24 * scale, scale, color(0xD7D7DB, alpha));
    monitorButton(device, layout.button(0), "Cancel", alpha, scale, focus == 2, 1.f);
    monitorButton(device, layout.button(1), "Search", alpha, scale, focus == 3, 1.f);
    if (model.searchPicker) {
        for (int i = 0; i < 2; ++i) {
            const auto option = layout.option(i);
            rect(device, option.x, option.y, option.w, option.h,
                 color(model.searchModeCursor == i ? 0xEEB039 : 0xE1E1E3, alpha));
            text(device, i ? "Model ID" : "Model Name", option.x + 9 * scale, option.y + 4 * scale,
                 14.5f * scale, color(0x08080C, alpha), false, 0, true);
        }
    }
}

void drawSearchNotice(IDirect3DDevice9* device, float width, float height, const Model& model)
{
    const float alpha = model.searchNoticeOpacity();
    if (alpha <= 0) {
        return;
    }
    const float scale = SearchPanel(width, height, false).scale;
    const float entry = CinematicStudio::ease(float((model.clock - model.searchUntil + 2.8) / .16));
    const float leave = 1 - CinematicStudio::ease(float((model.searchUntil - model.clock) / .16));
    const float boxWidth = std::min(width - 30 * scale, 450 * scale);
    const SearchPanel searchPanel(width, height, model.ui.keyboard.open,
                                  model.searchOffset(scale));
    const float y = model.searchFailed
                        ? std::max(8 * scale, searchPanel.box.y - 76 * scale)
                        : height * .735f - 84 * scale;
    const UIRect box{(width - boxWidth) * .5f, y + (1 - entry - leave) * 14 * scale,
                     boxWidth, 64 * scale};
    messageWindow(device, box, "Search", scale, alpha);
    if (model.searchResultId >= 0) {
        const auto field = messageRow(device, box, 0, "Model Found", "", scale, alpha);
        const std::string id = "ID: " + std::to_string(model.searchResultId);
        const float idWidth = textWidth(id, 14.5f * scale, true);
        text(device, model.searchResultName, field.x + 9 * scale, field.y + 4 * scale,
             fitted(model.searchResultName, 14.5f * scale, field.w - idWidth - 30 * scale, true),
             color(0x08080C, alpha), false, 0, true);
        text(device, id, field.x + field.w - idWidth - 9 * scale, field.y + 4 * scale,
             14.5f * scale, color(0x08080C, alpha), false, 0, true);
    } else {
        messageRow(device, box, 0, "Search Result", model.searchFailed ? "Model Not Found" : model.searchNotice,
                   scale, alpha);
    }
}

void drawCountryNotice(IDirect3DDevice9* device, float width, float height)
{
    const float scale = SearchPanel(width, height, false).scale;
    const UIRect box{(width - 620 * scale) * .5f, height * .43f, 620 * scale, 98 * scale};
    messageWindow(device, box, "Vehicle Information", scale, 1);
    messageRow(device, box, 0, "Country", "Not Assigned", scale, 1);
    messageRow(device, box, 1, "Configuration", "Click the white icon on the top left to assign it.", scale, 1, true, true);
}

void drawPlate(IDirect3DDevice9* device, float width, float height, const Model& model)
{
    if (!model.plateOpen) {
        return;
    }
    const float alpha = model.plateMotion.reveal(model.clock);
    if (model.plateReturn != SpawnReview::None) {
        rect(device, 0, 0, width, height, color(0, .2f * alpha));
    }
    const auto panel=model.plateLayout(width,height);
    const float scale = panel.scale;
    messageWindow(device, panel.box, "License Plate", scale, alpha);
    const auto input = messageRow(device, panel.box, 0, "Text", model.plate, scale, alpha);
    if (model.plateEditing && int(model.clock * 2) % 2 == 0) {
        const float x = input.x + 9 * scale + textWidth(model.plate, 14.5f * scale, true);
        rect(device, x + scale, input.y + 4 * scale, scale, 15 * scale, color(0x08080C, alpha));
    }
    messageRow(device, panel.box, 1, "Info", "Enter up to 8 characters for your license plate.", scale, alpha, false, true);
    const int focus = model.ui.dock || model.ui.controllerUsed
                          ? model.plateRow : panel.hit(model.pointerX * width, model.pointerY * height);
    const char* labels[]{"Back", "Random", "Spawn Now"};
    for (int i = 0; i < 3; ++i) {
        monitorButton(device, panel.button(i), labels[i], alpha, scale, focus == i + 1, 1.f);
    }
}
