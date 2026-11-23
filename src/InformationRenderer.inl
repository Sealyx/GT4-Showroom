bool suppliedIcon(IDirect3DDevice9* device, unsigned index, UIRect box, float alpha)
{
    constexpr std::array<const wchar_t*, 6> files{
        L"wash.png", L"clean.png", L"controller.png", L"description.png", L"info.png", L"links.png"};
    if (index >= suppliedIcons.size()) {
        return false;
    }
    auto& image = suppliedIcons[index];
    if (!suppliedIconsTried[index]) {
        image = iconDictionary.load(device, assetPath(shopAssetRoot / L"icons.txd"), std::filesystem::path(files[index]).stem().wstring());
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

UIRect tutorialButton(float width, float height, const Model& model) const
{
    if (tutorialLayoutMode != model.tutorial || tutorialLayoutBegan != model.tutorialMotion.began) {
        return {};
    }
    return InformationPanel(width, height, model.tutorialMotion.offset(model.clock, height),
                            tutorialLayoutWidth, tutorialLayoutHeight).target(3);
}

void drawInformation(IDirect3DDevice9* device, float width, float height, const Model& model)
{
    if (!model.informationOpen && !model.tutorialOpen()) {
        return;
    }
    const bool tutorial = model.tutorialOpen();
    const auto& motion = tutorial ? model.tutorialMotion : model.informationMotion;
    const float alpha = motion.reveal(model.clock);
    const float scale = InformationPanel(width, height).scale;
    const auto regular = infoStyle(device);
    const auto linesFor = [&](std::string remaining, float size, float available) {
        std::vector<std::string> lines;
        remaining = localized(remaining);
        while (!remaining.empty()) {
            size_t count = remaining.size();
            while (count > 1 && textWidth(remaining.substr(0, count), size * scale, regular) > available * scale) {
                --count;
                while (count > 1 && (static_cast<unsigned char>(remaining[count]) & 0xC0) == 0x80) {
                    --count;
                }
            }
            if (count < remaining.size()) {
                const auto space = remaining.rfind(' ', count);
                if (space != std::string::npos && space) {
                    count = space;
                }
            }
            lines.push_back(remaining.substr(0, count));
            size_t next = remaining.find_first_not_of(' ', count);
            remaining = next == std::string::npos ? "" : remaining.substr(next);
        }
        return lines;
    };

    struct Topic { const char* title; const char* body; };
    constexpr Topic overview[]{
        {"Cinematic", "Hold the left mouse button on the vehicle and drag to rotate it. Scroll to zoom. Use the right stick to rotate with a controller."},
        {"Hotspots", "Click a hotspot to open or close the supported door, bonnet, boot or other part. Use Tab or the controller to cycle through hotspots."},
        {"Studio settings", "Open Settings to change the background, lighting, shadows and independent floor or vehicle reflection quality. Open Photo Lab to frame and save a photograph."},
        {"Customization", "Open Customization from Settings to wash the vehicle, adjust suspension, and preview wheels and components."}
    };
    constexpr Topic studioView[]{
        {"Inspect the vehicle", "Drag with the left mouse button or use the right stick to rotate. Scroll to zoom in on details."},
        {"Set up the studio", "Open Settings to change the scene, floor, palette, reflections or shadows. Set Rotation to Paused to hold your angle."},
        {"Photo Lab", "Open Photo Lab from Settings to frame a shot. Use the controls in the footer to hide the interface or save a photo."}
    };
    constexpr Topic photo[]{
        {"Frame the shot", "Drag to look, use WASD to pan and scroll to zoom. Adjust FOV, roll or aspect ratio in the camera panel."},
        {"Visuals", "Change the studio for this shot. The previous background returns when you leave Photo Lab."},
        {"Take an image", "Press Space or select Take Image. Focus and shutter sounds accompany the capture. Photos are saved in the showroom photos folder."}
    };
    constexpr Topic customization[]{
        {"Choose a service", "Highlight Car Wash, Suspension, Wheels or Components. The footer describes the current service."},
        {"Preview your changes", "Browse compatible wheels and components to preview them on the vehicle. Adjust suspension with the lift control."},
        {"Apply and return", "Confirm a change to keep it. Backspace returns to the previous screen. Your choices carry through when you spawn."}
    };
    constexpr std::array<const wchar_t*, 3> demoNames{
        L"cinematic", L"viewer", L"customization"};
    const auto loadDemo = [&](size_t index) {
        auto& demo = tutorialDemos[index];
        if (!tutorialDemoTried[index]) {
            tutorialDemoTried[index] = true;
            demo = tutorialArtworkDictionary.load(device, assetPath(shopAssetRoot / L"tutorial.txd"), demoNames[index]);
        }
        return demo.texture && demo.width == 2048 && demo.height == 1152;
    };
    const bool combined = model.tutorial == Tutorial::CinematicWithCustomization;
    const bool intro = model.tutorial == Tutorial::CustomizationIntro;
    const bool inStudioView = false;
    const bool inShop = model.tutorial == Tutorial::Customization;
    const bool legacy = false;
    const size_t mainDemo = inShop || intro ? 2 : 1;
    const bool mainAnimated = tutorial && model.tutorial != Tutorial::Photo && loadDemo(mainDemo);
    const bool secondAnimated = combined && loadDemo(2);
    const bool animated = mainAnimated || secondAnimated;
    const Topic* topics = model.tutorial == Tutorial::Photo ? photo : intro ? overview + 3 : (inStudioView ? studioView : (inShop ? customization : overview));
    const size_t rows = intro ? 1 : (combined ? 4 : 3);
    const float panelWidth = intro ? 680.f : 760.f;
    const float demoWidth = intro ? 280.f : 327.f;
    const float demoHeight = legacy ? 184.f : demoWidth * .5625f;
    const float textWidthUnits = animated ? (intro ? 303.f : 352.f) : panelWidth - 58.f;
    constexpr float bodyTop = 48, bodyFont = 15, lineAdvance = bodyFont * 1.35f;
    constexpr float footerHeight = 50, bodyInset = 19, separatorPadding = 9;
    std::array<std::vector<std::string>, 4> paragraphs;
    std::array<float, 4> topicTop{}, divider{};
    float bodyHeight = 346;
    if (tutorial) {
        float textBottom = 0;
        float next = bodyTop + bodyInset;
        for (size_t i = 0; i < rows; ++i) {
            paragraphs[i] = linesFor(topics[i].body, bodyFont, textWidthUnits);
            topicTop[i] = legacy ? 71.f + i * 103.f : next;
            textBottom = topicTop[i] + 28.f + float(paragraphs[i].size()) * lineAdvance;
            divider[i] = legacy ? 167.f + i * 103.f : textBottom + separatorPadding;
            next = divider[i] + 15;
        }
        if (!legacy) {
            const float textHeight = textBottom - bodyTop - bodyInset;
            bodyHeight = std::max(textHeight, animated ? demoHeight : 0.f) + 2 * bodyInset;
            if (intro) {
                topicTop[0] = bodyTop + (bodyHeight - textHeight) * .5f;
            }
        }
    }
    const InformationPanel panel(width, height, motion.offset(model.clock, height), panelWidth,
                                 bodyTop + bodyHeight + footerHeight);
    const auto box = panel.box;
    if (tutorial) {
        tutorialLayoutMode = model.tutorial;
        tutorialLayoutBegan = motion.began;
        tutorialLayoutWidth = panelWidth;
        tutorialLayoutHeight = panel.heightUnits;
    }
    roundedRectangle(device, box.x + 3 * scale, box.y + 4 * scale, box.w, box.h, 12 * scale, color(0, .30f * alpha));
    roundedRectangle(device, box.x, box.y, box.w, box.h, 12 * scale, color(0x252527, alpha));
    rect(device, box.x, box.y + bodyTop * scale, box.w, bodyHeight * scale, color(0xC5C5CA, alpha));
    const auto write = [&](const std::string& textValue, float x, float y, float size, uint32_t rgb = 0x111114) {
        text(device, textValue, box.x + x * scale, box.y + y * scale, size * scale,
             color(rgb, alpha), false, 0, regular);
    };
    const auto wrap = [&](const std::string& content, float x, float y, float size, float available) {
        for (const auto& line : linesFor(content, size, available)) {
            write(line, x, y, size);
            y += size * 1.35f;
        }
    };
    if (!suppliedIcon(device, 4, {box.x + 20 * scale, box.y + 9 * scale, 29 * scale, 29 * scale}, alpha)) {
        write("i", 31, 8, 25, 0xD1D1D4);
    }
    const char* title = "Information";
    write(title, 63, 11, 24, 0xD1D1D4);
    const auto separator = [&](float y, float width) {
        rect(device, box.x + 27 * scale, box.y + y * scale, width, scale, color(0x8C8D94, alpha));
    };
    if (tutorial) {
        for (size_t i = 0; i < rows; ++i) {
            write(topics[i].title, 27, topicTop[i], 19);
            float y = topicTop[i] + 28;
            for (const auto& line : paragraphs[i]) {
                write(line, 27, y, bodyFont);
                y += lineAdvance;
            }
            if (i + 1 < rows) {
                separator(divider[i], animated ? textWidthUnits * scale : box.w - 54 * scale);
            }
        }
        if (animated) {
            const float x = panelWidth - demoWidth - 32;
            const float y = legacy ? 116.f : bodyTop + (bodyHeight - demoHeight) * .5f;
            const UIRect area{box.x + x * scale, box.y + y * scale, demoWidth * scale, demoHeight * scale};
            rect(device, area.x - 2 * scale, area.y - 2 * scale, area.w + 4 * scale, area.h + 4 * scale,
                 color(0x252527, alpha));
            constexpr double clipLength = 6.4, fadeLength = .7, switchAt = clipLength - fadeLength;
            const auto drawFrame = [&](size_t index, double elapsed, float opacity) {
                const auto frame = unsigned(std::fmod(elapsed * 10., 64.));
                const float u = float(frame % 8 * 256), v = float(frame / 8 * 144);
                rect(device, area.x, area.y, area.w, area.h, color(0xFFFFFF, alpha * opacity),
                     tutorialDemos[index].texture,
                     (u + .5f) / 2048.f, (v + .5f) / 1152.f, (u + 255.5f) / 2048.f, (v + 143.5f) / 1152.f);
            };
            const double age = std::max(0., model.clock - motion.began);
            if (mainAnimated && secondAnimated) {
                const bool showShop = std::fmod(age, 2 * switchAt) >= switchAt;
                const double local = std::fmod(age, switchAt);
                const size_t current = showShop ? 2 : mainDemo;
                const size_t previous = showShop ? mainDemo : 2;
                if (age >= switchAt && local < fadeLength) {
                    drawFrame(previous, switchAt + local, 1);
                    drawFrame(current, local, ease(float(local / fadeLength)));
                } else {
                    drawFrame(current, local, 1);
                }
            } else {
                drawFrame(mainAnimated ? mainDemo : 2, age, 1);
            }
        }
        monitorButton(device, panel.target(3), "OK", alpha, scale, model.tutorialFocus, 2.5f, 15.f);
        if (model.pointerVisible && !model.ui.controllerUsed) {
            cursor(device, model.pointerX * width, model.pointerY * height, height / 720.f * .85f, 1);
        }
        return;
    }
    write("Welcome to my Gran Turismo-inspired showroom!", 27, 63, 20);
    wrap("This project had been in plans and worked on for the past 3 years. I hope you enjoyed all the hard work I've poured into making this!",
         28, 94, 14.5f, 702);
    for (float y : {143.f, 221.f, 295.f}) {
        separator(y, box.w - 54 * scale);
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
