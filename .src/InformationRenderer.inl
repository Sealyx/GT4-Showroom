bool suppliedIcon(IDirect3DDevice9* device, unsigned index, UIRect box, float alpha)
{
    constexpr std::array<const wchar_t*, 6> files{
        L"wash.png", L"clean.png", L"controller.png", L"description.png", L"info.png", L"links.png"};
    if (index >= suppliedIcons.size()) {
        return false;
    }
    auto& image = suppliedIcons[index];
    if (!suppliedIconsTried[index]) {
        image = VehicleLogos::load(device, assetPath(shopLogoPath.parent_path().parent_path() / L"icons" / files[index]), 256);
        suppliedIconsTried[index] = true;
    }
    if (!image.texture || !image.width || !image.height) {
        return false;
    }
    const float fit = std::min(box.w / image.width, box.h / image.height);
    const float width = image.width * fit, height = image.height * fit;
    rect(device, box.x + (box.w - width) * .5f, box.y + (box.h - height) * .5f, width, height,
         color(0xFFFFFF, alpha), image.texture);
    return true;
}

void informationIcon(IDirect3DDevice9* device, float x, float y, float scale, int kind, float alpha)
{
    const DWORD background = color(0x55565A, alpha), ink = color(0xEEEEF0, alpha);
    roundedRectangle(device, x, y, 44 * scale, 36 * scale, 7 * scale, background);
    const float inset = kind == 2 ? 9.f : 6.f;
    if (suppliedIcon(device, unsigned(kind == 2 ? 5 : kind + 2),
                     {x + inset * scale, y + (inset - 2) * scale,
                      (44 - inset * 2) * scale, (40 - inset * 2) * scale}, alpha)) {
        return;
    }
    const auto block = [&](float left, float top, float width, float height, DWORD rgb) {
        roundedRectangle(device, x + left * scale, y + top * scale, width * scale, height * scale,
                         2 * scale, rgb);
    };
    if (kind == 0) {
        block(7, 10, 30, 17, ink);
        block(10, 11, 3, 12, background);
        block(6, 15, 11, 3, background);
        block(26, 13, 3, 3, background);
        block(31, 17, 3, 3, background);
    } else if (kind == 1) {
        block(13, 5, 20, 27, ink);
        block(15, 7, 16, 23, background);
        for (float top : {12.f, 18.f, 24.f}) {
            block(18, top, 10, 1.5f, ink);
        }
    } else {
        block(7, 14, 19, 11, ink);
        block(9, 16, 15, 7, background);
        block(21, 8, 16, 11, ink);
        block(23, 10, 12, 7, background);
        block(17, 16, 11, 3, ink);
    }
}

void drawInformation(IDirect3DDevice9* device, float width, float height, const Model& model)
{
    if (!model.informationOpen) {
        return;
    }
    const float alpha = model.informationMotion.reveal(model.clock);
    const InformationPanel panel(width, height, model.informationMotion.offset(model.clock, height));
    const auto box = panel.box;
    const float scale = panel.scale;
    roundedRectangle(device, box.x + 3 * scale, box.y + 4 * scale, box.w, box.h, 12 * scale, color(0, .30f * alpha));
    roundedRectangle(device, box.x, box.y, box.w, box.h, 12 * scale, color(0x252527, alpha));
    rect(device, box.x, box.y + 48 * scale, box.w, 346 * scale, color(0xC5C5CA, alpha));
    const auto regular = infoStyle(device);
    const auto write = [&](const std::string& textValue, float x, float y, float size, uint32_t rgb = 0x111114) {
        text(device, textValue, box.x + x * scale, box.y + y * scale, size * scale,
             color(rgb, alpha), false, 0, regular);
    };
    const auto wrap = [&](std::string remaining, float x, float y, float size, float available) {
        remaining = localized(remaining);
        while (!remaining.empty()) {
            size_t count = remaining.size();
            while (count > 1 && textWidth(remaining.substr(0, count), size * scale, regular) > available * scale) {
                --count;
                while (count > 0 && count < remaining.size() &&
                       (static_cast<unsigned char>(remaining[count]) & 0xC0) == 0x80) {
                    --count;
                }
            }
            if (count < remaining.size()) {
                const auto space = remaining.rfind(' ', count);
                if (space != std::string::npos && space) {
                    count = space;
                }
            }
            write(remaining.substr(0, count), x, y, size);
            remaining.erase(0, count);
            while (!remaining.empty() && remaining.front() == ' ') {
                remaining.erase(0, 1);
            }
            y += size * 1.35f;
        }
    };
    if (!suppliedIcon(device, 4, {box.x + 20 * scale, box.y + 9 * scale, 29 * scale, 29 * scale}, alpha)) {
        write("i", 31, 8, 25, 0xD1D1D4);
    }
    write("Information", 63, 11, 24, 0xD1D1D4);
    write("Welcome to my Gran Turismo-inspired showroom!", 27, 63, 20);
    wrap("This project had been in plans and worked on for the past 3 years. I hope you enjoyed all the hard work I've poured into making this!",
         28, 94, 14.5f, 702);
    for (float y : {143.f, 221.f, 295.f}) {
        rect(device, box.x + 27 * scale, box.y + y * scale, box.w - 54 * scale, scale, color(0x8C8D94, alpha));
    }
    informationIcon(device, box.x + 28 * scale, box.y + 157 * scale, scale, 0, alpha);
    informationIcon(device, box.x + 28 * scale, box.y + 235 * scale, scale, 1, alpha);
    informationIcon(device, box.x + 28 * scale, box.y + 310 * scale, scale, 2, alpha);
    write("Controls", 90, 154, 18);
    wrap("All the available controls are displayed in the footer at the bottom of the screen, please check it out if you're confused on which button does what.",
         90, 177, 14, 638);
    write("Descriptions", 90, 231, 18);
    wrap("You can highlight a setting to view its description in the footer, this applies to all the setting boxes!",
         90, 254, 14, 638);
    write("Links", 90, 305, 18);
    constexpr const char* labels[]{"My YouTube", "My Twitter", "Check for updates"};
    for (int i = 0; i < 3; ++i) {
        const auto area = panel.target(i);
        const float size = (i == 2 ? 12.5f : 14.5f) * scale;
        const auto ink = color(model.informationChoice == i ? 0xD48600 : i == 2 ? 0xC7C7CC : 0x233AA5, alpha);
        text(device, labels[i], area.x, area.y + 2 * scale, size, ink, false, 0, regular);
        rect(device, area.x, area.y + size + 4 * scale, textWidth(labels[i], size, regular), scale, ink);
    }
    write("Version 3.0", 650, 411, 12.5f, 0xD1D1D4);
    monitorButton(device, panel.target(3), "OK", alpha, scale, model.informationChoice == 3, 2.5f, 15.f);
    if (model.pointerVisible && !model.ui.controllerUsed) {
        cursor(device, model.pointerX * width, model.pointerY * height, height / 720.f * .85f, 1);
    }
}
