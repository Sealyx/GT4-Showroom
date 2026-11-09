void drawShopTile(IDirect3DDevice9* device, float width, float height, float x, float y, const Mesh* part,
                  const Paint& paint, float angle, float size, bool shadow, float lift = 0, float widthLimit = 0)
{
    if (!part || part->empty()) {
        return;
    }
    Camera camera(width, height, 4.f, y);
    camera.offsetX = width * (x - .5f);
    camera.focal *= size;
    const float normalized = 4.7f / std::max(.01f, part->span());
    if (widthLimit > 0) {
        const V3 midpoint{(part->low.x + part->high.x) * .5f, (part->low.y + part->high.y) * .5f, part->low.z};
        const float sine = std::sin(angle), cosine = std::cos(angle);
        float minX = width, maxX = 0;
        for (float x : {part->low.x, part->high.x}) {
            for (float y : {part->low.y, part->high.y}) {
                for (float z : {part->low.z, part->high.z}) {
                    const V3 p = (V3{x, y, z} - midpoint) * normalized;
                    float px, py, pz, rhw;
                    if (camera.project({p.x * cosine - p.y * sine, p.x * sine + p.y * cosine, p.z}, px, py, pz, rhw)) {
                        minX = std::min(minX, px);
                        maxX = std::max(maxX, px);
                    }
                }
            }
        }
        const float span = maxX - minX;
        if (span > 0) {
            const float fit = std::min(1.f, widthLimit / span);
            camera.focal *= fit;
            camera.offsetX -= ((minX + maxX) * .5f - width * x) * fit;
        }
    }
    if (shadow) {
        mesh(device, *part, camera, angle, paint, .34f, normalized, true);
    }
    camera.cy -= height * lift;
    mesh(device, *part, camera, angle, paint, 1, normalized);
}

void shopMirror(IDirect3DDevice9* device, const Mesh& car, const Camera& camera, float angle, const Paint& paint,
                float alpha, float size, float rideHeight, bool clean)
{
    const float scale = std::min(.5f, 720.f / std::max(camera.w, camera.h));
    const UINT width = UINT(std::max(1.f, std::round(camera.w * scale)));
    const UINT height = UINT(std::max(1.f, std::round(camera.h * scale)));
    if (!allocateViewerTarget(device, width, height)) {
        return;
    }
    RenderTargets layers(device);
    const D3DVIEWPORT9 viewport{0, 0, width, height, 0, 1};
    const bool ready = layers.select(viewerTargetSurface, viewerDepth, viewport) &&
                       SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0));
    if (ready) {
        Camera reduced = camera;
        reduced.w = float(width);
        reduced.h = float(height);
        reduced.focal *= reduced.h / camera.h;
        reduced.cy *= reduced.h / camera.h;
        reduced.offsetX *= reduced.w / camera.w;
        device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
        device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
        mesh(device, car, reduced, angle, paint, 1, size, false, true, false, nullptr, false, rideHeight, false, clean);
        device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    }
    if (!layers.restore() || !ready) {
        return;
    }
    d3dShopOverlay(device);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    const float gain = .31f * alpha;
    ScreenVertex floor[] = {
        {0, 0, 0, 1, color(0xFFFFFF, gain, gain), 0, 0},
        {camera.w, 0, 0, 1, color(0xFFFFFF, gain, gain), 1, 0},
        {0, camera.h, 0, 1, 0, 0, 1},
        {camera.w, camera.h, 0, 1, 0, 1, 1},
    };
    device->SetTexture(0, viewerTarget);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, floor, sizeof(ScreenVertex));
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
}

void shopPriceMark(IDirect3DDevice9* device, float x, float y, float scale)
{
    d3dShopOverlay(device);
    if (!shopStrokeTried) {
        shopStroke = VehicleLogos::load(device, assetPath(shopLogoPath.parent_path() / L"stroke.png"), 1024);
        shopStrokeTried = true;
    }
    if (shopStroke.texture && shopStroke.width) {
        const float width = 95 * scale;
        const float height = width * shopStroke.height / shopStroke.width;
        rect(device, x - width * .50f, y - height * .52f, width, height, color(0xFFFFFF), shopStroke.texture);
    }
    d3dShopOverlay(device);
    constexpr size_t sides = 64;
    constexpr float tilt = -.14f;
    const float sine = std::sin(tilt), cosine = std::cos(tilt);
    const auto disc = [&](float expand, float offset, DWORD center, DWORD edge) {
        std::array<ScreenVertex, sides + 2> vertices{};
        vertices[0] = {x, y + offset, 0, 1, center, 0, 0};
        for (size_t i = 0; i <= sides; ++i) {
            const float angle = float(i) * 2.f * pi / sides;
            const float dx = std::cos(angle) * 21 * scale * expand;
            const float dy = std::sin(angle) * 19 * scale * expand;
            vertices[i + 1] = {x + dx * cosine - dy * sine, y + dx * sine + dy * cosine + offset, 0, 1, edge, 0, 0};
        }
        device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, sides, vertices.data(), sizeof(ScreenVertex));
    };
    disc(1.28f, 2 * scale, color(0, .35f), color(0, 0));
    disc(1.f, 0, color(0x060606), color(0x060606));
    text(device, "$", x, y - 17 * scale, 31 * scale, color(0xFF9500), true, 0, textStyle(true), tilt);
}

void shopFadedBand(IDirect3DDevice9* device, float x, float y, float width, float height, float alpha)
{
    const std::array<float, 5> stops{0, .10f, .55f, .91f, 1};
    const std::array<float, 5> opacity{0, .76f, 1, .80f, 0};
    std::array<ScreenVertex, 10> strip{};
    for (size_t i = 0; i < stops.size(); ++i) {
        const float px = x + width * stops[i];
        strip[2 * i] = {px, y, 0, 1, color(0x140E0A, alpha * opacity[i]), 0, 0};
        strip[2 * i + 1] = {px, y + height, 0, 1, color(0x070504, alpha * .74f * opacity[i]), 0, 0};
    }
    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 8, strip.data(), sizeof(ScreenVertex));
}

void shopCapsule(IDirect3DDevice9* device, float x, float y, float width, float height, DWORD upper, DWORD lower)
{
    const float radius = std::min(width, height) * .5f;
    std::array<ScreenVertex, 20> rim{};
    for (size_t i = 0; i < 10; ++i) {
        const bool top = i < 5;
        const float a = float(top ? i : i - 1) * pi / 8;
        const float base = top ? y + radius : y + height - radius;
        const float py = base - std::cos(a) * radius;
        const float round = std::sin(a) * radius;
        const DWORD tint = top ? upper : lower;
        rim[2 * i] = {x + radius - round, py, 0, 1, tint, 0, 0};
        rim[2 * i + 1] = {x + width - radius + round, py, 0, 1, tint, 0, 0};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 18, rim.data(), sizeof(ScreenVertex));
}

void shopWashMeter(IDirect3DDevice9* device, float width, float height, float progress)
{
    const float x = width * .306f, y = height * .813f, w = width * .40f, h = height * .055f;
    shopCapsule(device, x, y + h * .13f, w, h, color(0x442819, .55f), color(0x412314, .40f));
    shopCapsule(device, x, y, w, h, color(0xDBA551, 1), color(0x654617, 1));
    shopCapsule(device, x + h * .1f, y + h * .12f, w - h * .2f, h * .76f, color(0x78572C, 1), color(0x493619, 1));

    constexpr int segments = 13;
    for (int i = 0; i < segments; ++i) {
        const float position = float(i) / (segments - 1);
        const float px = x + w * (.15f + .70f * position);
        const bool filled = progress > 0 && progress >= float(i + 1) / segments;
        const DWORD ink = color(filled ? 0x9FDBFF : 0x20170D, 1);
        ScreenVertex slash[] = {
            {px + h * .04f, y + h * .63f, 0, 1, ink, 0, 0},
            {px + h * .17f, y + h * .63f, 0, 1, ink, 0, 0},
            {px + h * .17f, y + h * .33f, 0, 1, ink, 0, 0},
            {px + h * .30f, y + h * .33f, 0, 1, ink, 0, 0},
        };
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, slash, sizeof(ScreenVertex));
    }
    const float spray = x + w * .050f;
    const DWORD water = color(0xB5E5FF, progress > 0 ? .94f : .65f);
    rect(device, spray - h * .20f, y + h * .65f, h * .40f, h * .085f, water);
    for (int i = -1; i <= 1; ++i) {
        rect(device, spray + i * h * .14f - h * .035f, y + h * .29f, h * .07f, h * .29f, water);
    }
    const DWORD finish = color(0x241608, .85f);
    const float mark = x + w * .95f;
    rect(device, mark - h * .22f, y + h * .38f, h * .42f, h * .25f, finish);
    rect(device, mark - h * .12f, y + h * .29f, h * .23f, h * .11f, finish);
    rect(device, mark - h * .17f, y + h * .57f, h * .07f, h * .16f, finish);
    rect(device, mark + h * .12f, y + h * .57f, h * .07f, h * .16f, finish);
}

void shopWashBack(IDirect3DDevice9* device, float x, float y, float scale)
{
    d3dShopOverlay(device);
    constexpr std::array<std::array<float, 2>, 7> points{{
        {-19, 0},
        {-5, -12},
        {-5, -6},
        {17, -6},
        {17, 6},
        {-5, 6},
        {-5, 12},
    }};
    constexpr std::array<unsigned short, 9> indices{0, 1, 6, 2, 3, 4, 2, 4, 5};
    const auto draw = [&](float offset, DWORD light, DWORD dark) {
        std::array<ScreenVertex, 7> vertices{};
        for (size_t i = 0; i < points.size(); ++i) {
            vertices[i] = {x + points[i][0] * scale,
                           y + points[i][1] * scale + offset,
                           0,
                           1,
                           points[i][1] <= 0 ? light : dark,
                           0,
                           0};
        }
        device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, UINT(vertices.size()), 3, indices.data(), D3DFMT_INDEX16,
                                       vertices.data(), sizeof(ScreenVertex));
    };
    draw(scale * 2, color(0x26301B, .72f), color(0x202616, .72f));
    draw(0, color(0x9ED453, 1), color(0x357526, 1));
}

void drawShopBackdrop(IDirect3DDevice9* device, float width, float height, ShopPage page)
{
    d3dShopOverlay(device);
    ScreenVertex floor[] = {
        {0, 0, 0, 1, color(0xA44B1E, 1), 0, 0},
        {width, 0, 0, 1, color(0xCC6C25, 1), 0, 0},
        {0, height, 0, 1, color(0xE9A43B, 1), 0, 0},
        {width, height, 0, 1, color(0xC77C29, 1), 0, 0},
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, floor, sizeof(ScreenVertex));
    constexpr std::array<const wchar_t*, 6> images{L"menu.png",       L"menu.png",       L"wheels.png",
                                                   L"components.png", L"suspension.png", L"car-wash.png"};
    constexpr std::array<uint32_t, 6> tints{0x726326, 0x9E8742, 0xBD725A, 0xA8789C, 0x9C8644, 0x629DC9};
    const size_t section = size_t(page);
    auto& artwork = shopArtwork[section];
    if (!shopArtworkTried[section]) {
        artwork = VehicleLogos::load(device, assetPath(shopLogoPath.parent_path() / images[section]), 1024);
        shopArtworkTried[section] = true;
    }
    const uint32_t tint = tints[section];
    const auto boundary = [width](float y) {
        return width * .46f * (1 - y) * (1 - y);
    };
    auto* art = artwork.texture;
    const float panelWidth = width * .46f;
    const float panelHeight = height * .92f;
    const float cover = art ? std::max(panelWidth / artwork.width, panelHeight / artwork.height) : 1.f;
    const float uRange = art ? panelWidth / (cover * artwork.width) : 1.f;
    const float vRange = art ? panelHeight / (cover * artwork.height) : 1.f;
    const float uStart = (1.f - uRange) * .5f, vStart = (1.f - vRange) * .5f;
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetTexture(0, art);
    device->SetTextureStageState(0, D3DTSS_COLOROP, art ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, art ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    std::array<ScreenVertex, 42> edge{};
    for (size_t i = 0; i <= 20; ++i) {
        const float y = float(i) / 20;
        const float edgeX = boundary(y);
        const float v = vStart + y * vRange;
        edge[2 * i] = {0, y * panelHeight, 0, 1, color(tint, .97f), uStart, v};
        edge[2 * i + 1] = {edgeX,
                           y * panelHeight,
                           0,
                           1,
                           color(viewerBlend(tint, 0xFFFFFF, .2f), .93f),
                           uStart + edgeX / panelWidth * uRange,
                           v};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 40, edge.data(), sizeof(ScreenVertex));

    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    for (size_t i = 0; i <= 20; ++i) {
        const float y = float(i) / 20;
        const float px = boundary(y);
        const float spread = width * (.065f - .038f * y);
        edge[2 * i] = {px - spread, y * height * .92f, 0, 1, color(viewerBlend(tint, 0xE4E8E0, .18f), .48f), 0, 0};
        edge[2 * i + 1] = {px, y * height * .92f, 0, 1, color(tint, .94f), 0, 0};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 40, edge.data(), sizeof(ScreenVertex));
    for (size_t i = 0; i <= 20; ++i) {
        const float y = float(i) / 20;
        const float px = boundary(y);
        edge[2 * i] = {px, y * height * .92f, 0, 1, color(0xBD692F, .84f), 0, 0};
        edge[2 * i + 1] = {px + width * .018f, y * height * .92f, 0, 1, color(0xD5792E, 0), 0, 0};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 40, edge.data(), sizeof(ScreenVertex));
}

template <class Parts>
void drawTransFender(IDirect3DDevice9* device, float width, float height, const Model& model, const Entry& entry,
                     const Mesh* car, Parts parts)
{
    const auto& shop = model.shop;
    const auto page = shop.page;
    const float scale = height / 720.f;
    const float time = float(std::max(0., model.clock - shop.pageStart));
    const float reveal = shop.reveal(model.clock);
    const auto& groups = model.ui.menu.customizations;
    const bool partPage = page == ShopPage::Wheels || page == ShopPage::Components;
    const bool hasParts = partPage && shop.option >= 0 && size_t(shop.option) < shop.parts.size();
    const bool menu = page == ShopPage::Menu;
    const bool wheels = page == ShopPage::Wheels;
    const bool suspension = page == ShopPage::Suspension;
    const bool wash = page == ShopPage::CarWash;
    Paint paint = entry.paints.empty() ? Paint{} : entry.paints[std::min(model.paint, entry.paints.size() - 1)];

    drawShopBackdrop(device, width, height, page);
    lightPhase = lastShadowPhase = 0;
    if (car && reveal > 0) {
        Camera camera(width, height, 5.f, wheels ? .35f : .40f);
        float center = .70f, pitch = .13f, distance = 7.1f, angle = pi;
        float focusHeight = .65f;
        if (wheels) {
            center = .85f;
            pitch = .10f;
            distance = 5.05f;
            angle = pi * .515f;
            focusHeight = .46f;
        } else if (suspension) {
            center = .57f;
            pitch = .97f;
            angle = pi * .69f;
        } else if (wash) {
            center = .55f + .40f * (1.f - reveal);
            distance = 5.35f;
            angle = pi * (.5f + 2.f * std::fmod(shop.washTurns(model.clock), 1.f));
        } else if (!menu) {
            center = .68f;
            distance = 6.9f;
            angle = pi * 1.22f + std::fmod(time, 16.f) * (2.f * pi / 16.f);
        }
        camera.offsetX = width * (center - .5f);
        distance *= 1.f + .085f * (1.f - reveal) + .04f * shop.fade(model.clock);
        const V3 focus{0, 0, focusHeight};
        camera.eye = focus + V3{0, -distance * std::cos(pitch), distance * std::sin(pitch)};
        camera.forward = unit(focus - camera.eye);
        camera.right = unit(cross(camera.forward, {0, 0, 1}));
        camera.up = cross(camera.right, camera.forward);
        const float size = 4.8f / std::max(.2f, car->span());
        const float ride = float(suspension ? shop.rideHeight : shop.confirmedRideHeight) * .025f;
        shopMirror(device, *car, camera, angle, paint, reveal, size, ride, shop.washed);
        if (model.ui.shadows) {
            mesh(device, *car, camera, angle, paint, reveal, size, true, false, false, nullptr, false, ride);
        }
        mesh(device, *car, camera, angle, paint, reveal, size, false, false, false, nullptr, false, ride, false,
             shop.washed);
    }

    d3dShopOverlay(device);
    const float carUi = std::clamp((time - .19f) / .24f, 0.f, 1.f);
    std::string detail;
    int price = 0;
    if (menu) {
        if (!shopLogoTried) {
            shopLogo = VehicleLogos::load(device, assetPath(shopLogoPath));
            shopLogoTried = true;
        }
        if (shopLogo.texture && shopLogo.width) {
            const float logoWidth = std::min(width * .32f, 340 * scale);
            rect(device, width * .035f, height * .041f, logoWidth, logoWidth * shopLogo.height / shopLogo.width,
                 0xFFFFFFFF, shopLogo.texture);
        }
        const std::array<int, 4> iconModels{5409, entry.id, 1082, 1147};
        constexpr std::array<float, 4> angles{3.9f, 3.76f, -1.30f, 2.46f};
        constexpr std::array<float, 4> sizes{.43f, .41f, .38f, .42f};
        for (size_t i = 0; i < iconModels.size(); ++i) {
            float bounce = 0;
            if (int(i) == shop.section) {
                const float phase = std::fmod(time, .72f) / .72f;
                bounce = .044f * 4.f * phase * (1.f - phase);
            }
            Paint iconPaint = paint;
            if (shopSections[i] == ShopPage::Suspension) {
                for (auto& value : iconPaint.rgb) {
                    value = 0xFFFFFF;
                }
            }
            drawShopTile(device, width, height, shopIconX[i], shopIconY[i], parts(iconModels[i]), iconPaint, angles[i],
                         sizes[i], true, bounce);
            d3dShopOverlay(device);
            const float x = width * shopIconX[i];
            const float y = height * (shopIconY[i] + .065f);
            const std::string name = shopTitle(shopSections[i]);
            text(device, name, x + scale, y + 2 * scale, 22 * scale, color(0x583015, .55f), true, 3 * scale, true);
            text(device, name, x, y, 22 * scale, color(0x16120C, 1), true, 3 * scale, true);
        }
        constexpr const char* descriptions[] = {
            "Wash the vehicle before taking it out", "Adjust the ride height and see the change on the car",
            "Choose a set of wheels to suit your vehicle", "Choose compatible bodywork for the vehicle"};
        detail = descriptions[size_t(shop.section)];
    } else if (hasParts) {
        const int count = int(shop.parts.size());
        for (int tile = 0; tile < 7; ++tile) {
            const int index = shop.option + tile - 3;
            if (index < 0 || index >= count) {
                continue;
            }
            const auto part = shop.parts[size_t(index)];
            const auto& group = groups[size_t(part.group)];
            const float x = (float(tile) + .5f) / 7;
            const float size = tile == 3 ? .48f : .40f;
            const auto* geometry = parts(group.partModels[size_t(part.option)]);
            float angle = 1.38f;
            if (wheels) {
                angle = -1.31f;
            } else if (group.slot == 0 || group.slot == 14 || group.slot == 44 || group.slot == 45) {
                angle = group.slot == 45 ? 0.f : pi;
            }
            drawShopTile(device, width, height, x, .751f, geometry, paint, angle, size, true, 0, width * .115f);
        }
        d3dShopOverlay(device);
        const auto selected = shop.parts[size_t(shop.option)];
        const auto& group = groups[size_t(selected.group)];
        const auto& name = group.options[size_t(selected.option)];
        shopFadedBand(device, width * .085f, height * .833f, width * .83f, height * .055f, .83f * carUi);
        text(device, name, width * .5f, height * .840f, fitted(name, 25 * scale, width * .62f, true),
             color(0xFFFAED, carUi), true, 0, true);
        const float track = width * (shopTrackRight - shopTrackLeft);
        const float thumb = std::min(track, std::max(track / count, track * .075f));
        const float left = width * shopTrackLeft;
        const float position = count > 1 ? float(shop.option) / (count - 1) : 0;
        rect(device, left, height * shopTrackY, track, 4 * scale, color(0x302B24, .8f));
        rect(device, left + (track - thumb) * position, height * shopTrackY, thumb, 4 * scale, color(0xBCBAB2, 1));
        detail = wheels ? "Choose a wheel to install" : "Browse compatible bodywork and confirm to select";
        price = shopPartPrice(group.slot);
    } else if (suspension || wash) {
        if (wash) {
            const float progress = shop.washProgress(model.clock);
            shopWashMeter(device, width, height, progress);
            drawShopTile(device, width, height, .79f, .809f, parts(5409), paint, 3.9f, .29f, true);
            d3dShopOverlay(device);
            shopWashBack(device, width * .91f, height * .838f, scale * 1.45f);
            detail = shop.washStart >= 0. ? "Washing vehicle..." : shop.washed ? "Wash complete" : "Start the wash";
            price = TransFender::washPrice;
        } else {
            const float x = width * .91f, w = 23 * scale;
            const float y = height * shopLiftTop, h = height * (shopLiftBottom - shopLiftTop);
            shopCapsule(device, x - w * .59f, y, w * 1.18f, h, color(0xB78037, 1), color(0x654413, 1));
            shopCapsule(device, x - w * .44f, y + 4 * scale, w * .88f, h - 8 * scale, color(0x4D421D, 1),
                        color(0x71602C, 1));
            const float selected = float(shop.rideHeight - shopLiftMin) / (shopLiftMax - shopLiftMin);
            for (int tick = 0; tick < 23; ++tick) {
                const float level = float(tick) / 22;
                const uint32_t tint = level <= selected ? 0xDAB561 : 0x352C16;
                rect(device, x - w * .35f, y + h - 9 * scale - level * (h - 18 * scale), w * .7f, scale,
                     color(tint, .92f));
            }
            const float indicator = y + h - 9 * scale - selected * (h - 18 * scale);
            rect(device, x - w * .57f, indicator - 2 * scale, w * 1.14f, 4 * scale, color(0xFFCE66, 1));
            text(device, std::to_string(shop.rideHeight * 25) + " mm", x, height * .720f, 20 * scale,
                 color(0xFFFAED, 1), true, 0, true);
            shopWashBack(device, width * .91f, height * .838f, scale * 1.45f);
            detail = "Adjust the ride height and confirm your setting";
            price = shop.rideHeight ? TransFender::suspensionPrice : 0;
        }
    }

    if (!menu) {
        fontAtlas(device, shopSerifFont, shopSerifWidths, prizeNameAsset, 48, 80, L"Times New Roman");
        const TextStyle serif =
            shopSerifFont ? TextStyle{shopSerifFont, shopSerifWidths, 48, 80, 1.f} : textStyle(true);
        text(device, "TransFender", width * .033f + scale, height * .020f + 2 * scale, 35 * scale,
             color(0x251811, .82f), false, 0, serif);
        text(device, "TransFender", width * .033f, height * .020f, 35 * scale, color(0xFFF4E5, 1), false, 0, serif);
        const std::string section = shopTitle(page);
        const float titleX = width * .067f, titleY = height * .108f;
        fontAtlas(device, shopHeadingFont, shopHeadingWidths, shopHeadingAsset, 64, 128);
        const TextStyle heading =
            shopHeadingFont ? TextStyle{shopHeadingFont, shopHeadingWidths, 64, 128, 1.f} : textStyle(true);
        for (float radius : {3.f, 1.5f}) {
            for (int i = 0; i < 8; ++i) {
                const float angle = float(i) * pi / 4;
                text(device, section, titleX + std::cos(angle) * radius * scale,
                     titleY + std::sin(angle) * radius * scale, 32 * scale, color(0xFFF2D4, .075f), false, 0, heading);
            }
        }
        text(device, section, titleX, titleY, 32 * scale, color(0xFFF8EC), false, 0, heading);
        const char* explanation = wheels       ? "Browse the shop's wheel selection"
                                  : suspension ? "Preview the vehicle's ride height"
                                  : wash       ? "Clean the vehicle before taking it out"
                                               : "Select parts for the current vehicle";
        const float x = width * .068f, y = height * .163f;
        const float size = fitted(explanation, 17 * scale, width * .285f, true);
        text(device, explanation, x + scale, y + scale, size, color(0x110B09, .91f), false, 0, true);
        text(device, explanation, x, y, size, color(0xFFF4E7, 1), false, 0, true);
    }
    const auto balance = shopMoney(shop.balance - shop.pendingCost());
    SYSTEMTIME date{};
    GetLocalTime(&date);
    constexpr std::array<const char*, 12> months{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                 "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    const float size = 20 * scale, top = 4 * scale, barHeight = 28 * scale;
    const float right = width - 33 * scale;
    const float monthWidth = 61 * scale, dayWidth = 25 * scale;
    const float dateX = right - monthWidth - dayWidth;
    const float moneyX = dateX - 24 * scale - textWidth(balance, size, true);
    const float coinX = moneyX - 24 * scale;
    const float nameSize = fitted(entry.name, size, width * .32f, true);
    const float nameX = coinX - 20 * scale - textWidth(entry.name, nameSize, true);
    const float markerX = nameX - 25 * scale, barX = markerX - 20 * scale;
    ScreenVertex band[] = {
        {barX, 0, 0, 1, color(0x5C5C5C, 0), 0, 0},
        {width, 0, 0, 1, color(0, .97f), 0, 0},
        {barX, top + barHeight + 4 * scale, 0, 1, color(0x5C5C5C, 0), 0, 0},
        {width, top + barHeight + 4 * scale, 0, 1, color(0, .97f), 0, 0},
    };
    d3dShopOverlay(device);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, band, sizeof(ScreenVertex));
    constexpr std::array<float, 5> markerStops{0, .30f, .5f, .70f, 1};
    constexpr std::array<uint32_t, 5> markerColors{0x020202, 0x101010, 0x353535, 0x101010, 0x020202};
    std::array<ScreenVertex, 10> marker{};
    for (size_t i = 0; i < markerStops.size(); ++i) {
        const float y = top + barHeight * markerStops[i];
        marker[2 * i] = {markerX, y, 0, 1, color(markerColors[i]), 0, 0};
        marker[2 * i + 1] = {markerX + 12 * scale, y, 0, 1, color(markerColors[i]), 0, 0};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 8, marker.data(), sizeof(ScreenVertex));
    text(device, entry.name, nameX + scale, top + 2 * scale, nameSize, color(0, .9f), false, 0, true);
    text(device, entry.name, nameX, top + scale, nameSize, color(0xFFFFFF), false, 0, true);
    shopDollar(device, coinX, top + barHeight * .5f, 10 * scale);
    text(device, balance, moneyX + scale, top + 2 * scale, size, color(0, .9f), false, 0, true);
    text(device, balance, moneyX, top + scale, size, color(0xFFFFFF), false, 0, true);
    rect(device, dateX, top, dayWidth, barHeight, color(0xFFBD00));
    rect(device, dateX + dayWidth, top, monthWidth, barHeight, color(0xF4F4F4));
    ScreenVertex shade[] = {
        {dateX, top, 0, 1, color(0, .16f), 0, 0},
        {right, top, 0, 1, color(0, .16f), 0, 0},
        {dateX, top + barHeight * .5f, 0, 1, color(0, 0), 0, 0},
        {right, top + barHeight * .5f, 0, 1, color(0, 0), 0, 0},
    };
    d3dShopOverlay(device);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, shade, sizeof(ScreenVertex));
    text(device, std::to_string(date.wDay), dateX + dayWidth * .5f, top + 2.5f * scale, size, color(0), true, 0, true);
    text(device, months[size_t(std::clamp(int(date.wMonth), 1, 12) - 1)], dateX + dayWidth + monthWidth * .5f,
         top + 2.5f * scale, size, color(0), true, 0, true);
    if (price > 0) {
        const float x = width * (wash ? .090f : .076f);
        const float y = height * (wash ? .845f : .347f);
        const std::string amount = shopMoney(price);
        shopPriceMark(device, x, y, scale);
        text(device, amount, x + 29 * scale + scale, y - 19 * scale + 2 * scale, 35 * scale, color(0x21150D, .8f),
             false, 0, true);
        text(device, amount, x + 29 * scale, y - 19 * scale, 35 * scale, color(0xFFFFFF, 1), false, 0, true);
    }
    rect(device, 0, height * .922f, width, height * .078f, color(0x090909, 1));
    const auto& footer = shop.status.empty() ? detail : shop.status;
    const float footerAlpha = footerReveal(footer, model.clock);
    text(device, footer, width * .5f, height * .936f + (1.f - footerAlpha) * 9 * scale,
         fitted(footer, 18 * scale, width * .90f, true), color(0xE7E5E3, footerAlpha), true, 0, true);
    if (shop.partReview && hasParts) {
        const auto selected = shop.parts[size_t(shop.option)];
        const auto& group = groups[size_t(selected.group)];
        const std::string message = "Install " + group.options[size_t(selected.option)] + "?";
        const std::string priceText =
            shop.status == "Insufficient funds" || shop.status == "Wait for the preview to finish loading"
                ? shop.status
                : "$" + shopMoney(price) + " charged when you spawn the car";
        constexpr std::array<const char*, 2> buttons{"Cancel", "Confirm"};
        drawReviewPanel(device, width, height, shop.review.reveal(model.clock), shop.review.offset(model.clock, height),
                        shopTitle(page), message, priceText, buttons.data(), int(buttons.size()), shop.reviewChoice);
    }
    if (model.spawnReview == SpawnReview::None && model.pointerVisible && !model.ui.controllerUsed) {
        cursor(device, model.pointerX * width, model.pointerY * height, scale * .85f, 1);
    }
    const float fade = shop.fade(model.clock);
    if (fade > 0) {
        rect(device, 0, 0, width, height, color(0, fade));
    }
}

void d3dShopOverlay(IDirect3DDevice9* device)
{
    screen(device);
    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

void shopDollar(IDirect3DDevice9* device, float x, float y, float radius)
{
    d3dShopOverlay(device);
    std::array<ScreenVertex, 26> circle{};
    circle[0] = {x, y, 0, 1, color(0xFFDF77, 1), 0, 0};
    for (size_t i = 0; i < 25; ++i) {
        const float angle = float(i) * 2.f * pi / 24;
        circle[i + 1] = {x + radius * std::cos(angle), y + radius * std::sin(angle), 0, 1, color(0xD89D36, 1), 0, 0};
    }
    device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 24, circle.data(), sizeof(ScreenVertex));
    text(device, "$", x, y - radius * .94f, radius * 1.63f, color(0x281405, 1), true, 0, true);
}
