struct ShopGeometry
{
    std::map<int, std::unique_ptr<Mesh>> parts;
    std::map<int, uint64_t> requestAt;
    std::unique_ptr<Mesh> stockWheel;
    uint64_t stockSerial = 0;

    const Mesh* get(int id) const
    {
        if (id == -1) {
            return stockWheel.get();
        }
        const auto found = parts.find(id);
        return found == parts.end() ? nullptr : found->second.get();
    }

    void clear()
    {
        parts.clear();
        requestAt.clear();
        stockWheel.reset();
        stockSerial = 0;
    }

    void updateStock(const Mesh* vehicle)
    {
        if (!vehicle || vehicle->serial == stockSerial) {
            return;
        }
        stockSerial = vehicle->serial;
        stockWheel = std::make_unique<Mesh>();
        for (const auto& batch : vehicle->batches) {
            if (batch.component == int(CinematicComponent::FrontLeftWheel)) {
                stockWheel->batches.push_back(batch);
                if (batch.texture) {
                    batch.texture->AddRef();
                }
                if (batch.cleanTexture) {
                    batch.cleanTexture->AddRef();
                }
            }
        }
        stockWheel->bounds();
    }

    bool updatePart(int id, uint64_t now)
    {
        if (id < 0 || parts.count(id) || !gta::modelInfo(id)) {
            return false;
        }
        if (!gta::modelReady(id)) {
            auto& next = requestAt[id];
            if (now < next) {
                return false;
            }
            game::queue(id);
            next = now + 1000;
            return true;
        }
        if (parts.size() >= 48) {
            parts.erase(parts.begin());
        }
        void* info = gta::modelInfo(id);
        void* atomic = nullptr;
        uint8_t type = 0;
        if (!gta::read(uintptr_t(info) + 0x1C, atomic) || !atomic || !gta::read(uintptr_t(atomic), type) || type != 1) {
            parts.emplace(id, nullptr);
            return true;
        }
        auto result = std::make_unique<Mesh>();
        game::Extract context{result.get()};
        game::extractAtomic(atomic, &context);
        result->compact();
        result->bounds();
        parts.emplace(id, result->empty() ? nullptr : std::move(result));
        return true;
    }
};

ShopGeometry shopGeometry;

int* shopWallet()
{
    uint8_t player = 0;
    void* ped = nullptr;
    constexpr uintptr_t playerInfo = 0xB7CD98, playerFocus = 0xB7CD74;
    constexpr uintptr_t stride = 0x190, money = 0xB8;
    if (!gta::read(playerFocus, player) || player >= 2 || !gta::read(playerInfo + stride * player, ped) || !ped ||
        ped != game::player()) {
        return nullptr;
    }
    const uintptr_t address = playerInfo + stride * player + money;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &region, sizeof(region)) || region.State != MEM_COMMIT ||
        (region.Protect & PAGE_GUARD) ||
        ((region.Protect & 0xFF) != PAGE_READWRITE && (region.Protect & 0xFF) != PAGE_EXECUTE_READWRITE)) {
        return nullptr;
    }
    return reinterpret_cast<int*>(address);
}

void updateShopGeometry()
{
    if (!state.shop.open() || state.selected >= entries.size()) {
        return;
    }
    if (const auto* wallet = shopWallet()) {
        state.shop.balance = *wallet;
    } else {
        state.shop.balance = 0;
    }
    const auto& car = entries[state.selected];
    const auto cached = cache.find(car.id);
    shopGeometry.updateStock(cached == cache.end() ? nullptr : cached->second.mesh.get());
    const uint64_t now = GetTickCount64();
    if (state.shop.page == ShopPage::Menu) {
        for (const int id : {5409, 1082, 1147}) {
            if (shopGeometry.updatePart(id, now)) {
                return;
            }
        }
    }
    if (state.shop.page == ShopPage::CarWash) {
        shopGeometry.updatePart(5409, now);
    }
    const auto& rows = state.ui.menu.customizations;
    for (int delta = 0; delta < 4; ++delta) {
        for (int offset : {delta, -delta}) {
            const int index = state.shop.option + offset;
            if (index < 0 || size_t(index) >= state.shop.parts.size()) {
                continue;
            }
            const auto part = state.shop.parts[size_t(index)];
            if (part.group < int(rows.size()) &&
                shopGeometry.updatePart(rows[size_t(part.group)].partModels[size_t(part.option)], now)) {
                return;
            }
        }
    }
}

void clearShopGeometry()
{
    shopGeometry.clear();
}

bool shopSetRideHeight(void* vehicle, int rideHeight)
{
    if (!vehicle || rideHeight < -4 || rideHeight > 4 || rideHeight == 0 || game::field<int>(vehicle, 0x590) != 0 ||
        game::field<int>(vehicle, 0x594) != 0 || (game::field<uint32_t>(vehicle, 0x38C) & (1u << 17))) {
        return false;
    }

    const int id = game::field<int16_t>(vehicle, 0x22);
    constexpr std::array<int, 10> movingCollisionModels{406, 407, 432, 443, 486, 524, 525, 530, 531, 601};
    if (std::find(movingCollisionModels.begin(), movingCollisionModels.end(), id) != movingCollisionModels.end()) {
        return false;
    }

    const int existing = game::field<int8_t>(vehicle, 0x48B);
    if (existing >= 4 || (existing >= 0 && game::at<void*>(0xC1CC08 + existing * sizeof(void*)) != vehicle)) {
        return false;
    }

    constexpr uintptr_t function = 0x6DF3D0;
    constexpr std::array<uint8_t, 9> compact{0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x8B, 0x04, 0x00, 0x00};
    constexpr std::array<uint8_t, 9> hoodlum{0x56, 0x8B, 0xF1, 0x90, 0xE9, 0x38, 0xB0, 0xD2, 0xFF};
    std::array<uint8_t, 9> prefix{};
    if (!gta::read(function, prefix) || (prefix != compact && prefix != hoodlum)) {
        return false;
    }

    struct Line
    {
        V3 start;
        float startSize;
        V3 end;
        float endSize;
    };

    static_assert(sizeof(Line) == 0x20);

    auto linesAt = [](uintptr_t model) -> Line* {
        void* data = nullptr;
        uint8_t count = 0, flags = 0;
        Line* lines = nullptr;
        if (!model || !gta::read(model + 0x2C, data) || !data || !gta::read(uintptr_t(data) + 6, count) || count != 4 ||
            !gta::read(uintptr_t(data) + 7, flags) || (flags & 1) || !gta::read(uintptr_t(data) + 0x10, lines) ||
            !lines) {
            return nullptr;
        }
        return lines;
    };
    void* baseModel = nullptr;
    const auto info = gta::modelInfo(id);
    if (!info || !gta::read(uintptr_t(info) + 0x14, baseModel)) {
        return false;
    }
    Line* baseLines = linesAt(uintptr_t(baseModel));
    std::array<Line, 4> originals{};
    if (!baseLines || !gta::read(uintptr_t(baseLines), originals)) {
        return false;
    }
    for (const auto& line : originals) {
        if (!std::isfinite(line.start.z) || !std::isfinite(line.end.z) || line.start.z <= line.end.z) {
            return false;
        }
    }

    if (!reinterpret_cast<bool(__thiscall*)(void*)>(function)(vehicle)) {
        return false;
    }
    const int index = game::field<int8_t>(vehicle, 0x48B);
    if (index < 0 || index >= 4 || game::at<void*>(0xC1CC08 + index * sizeof(void*)) != vehicle) {
        return false;
    }
    const uintptr_t privateModel = 0xC1CC78 + index * 0x30;
    auto* lines = linesAt(privateModel);
    if (!lines || lines == baseLines) {
        return false;
    }
    const float shift = float(rideHeight) * .025f;
    std::array<Line, 4> current{}, target = originals;
    if (!gta::read(uintptr_t(lines), current)) {
        return false;
    }
    for (auto& line : target) {
        line.start.z -= shift;
        line.end.z -= shift;
    }
    if (std::memcmp(current.data(), target.data(), sizeof(target)) == 0) {
        return true;
    }
    if (std::memcmp(current.data(), originals.data(), sizeof(originals)) != 0) {
        return false;
    }
    for (int wheel = 0; wheel < 4; ++wheel) {
        lines[wheel].start.z = target[wheel].start.z;
        lines[wheel].end.z = target[wheel].end.z;
        game::field<float>(vehicle, 0x838 + wheel * sizeof(float)) -= shift;
        auto& minimum = game::at<float>(privateModel + 8);
        minimum = std::min(minimum, lines[wheel].end.z);
    }
    game::field<float>(vehicle, 0x898) += shift;
    game::field<float>(vehicle, 0x89C) += shift;
    auto& bound = game::at<float>(privateModel + 0x24);
    const auto low = game::at<V3>(privateModel);
    bound = std::max(bound, std::sqrt(dot(low, low)));
    return true;
}

void finishShopPurchase(void* vehicle, bool installedParts)
{
    if (!state.postSelectionCinematic || !vehicle || !state.shop.pendingCost()) {
        return;
    }
    int* wallet = shopWallet();
    if (!wallet || *wallet < state.shop.pendingCost()) {
        log("TransFender: balance changed before purchase");
        return;
    }
    int charge = installedParts ? state.shop.partsCost : 0;
    if (state.shop.confirmedRideHeight) {
        if (shopSetRideHeight(vehicle, state.shop.confirmedRideHeight)) {
            charge += TransFender::suspensionPrice;
        } else {
            log("TransFender: suspension could not be installed; no suspension charge");
        }
    }
    if (state.shop.washed) {
        game::field<float>(vehicle, 0x4B0) = 0.f;
        charge += TransFender::washPrice;
    }
    *wallet -= charge;
}
const Mesh* shopPart(int id)
{
    if (state.selected < entries.size() && id == entries[state.selected].id) {
        const auto stock = cache.find(id);
        return stock == cache.end() ? nullptr : stock->second.mesh.get();
    }
    return shopGeometry.get(id);
}
