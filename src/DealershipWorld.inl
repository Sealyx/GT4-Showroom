std::array<int,std::size(galleries)> galleryBlips{{-1,-1,-1,-1,-1}};
std::array<bool,std::size(galleries)> galleryEnabled{};
Gallery approaching = Gallery::Main;
constexpr double galleryHalfFade = 1.0;
double arrivalTime = -1;
Key galleryReturn;
StudioPreferences mainPreferences;
int mainStyleChoice = 0;
int mainVehicle = -1;
bool dealerSeatPending = false;

void readGalleryAccess()
{
    for (int i=1;i<int(std::size(galleries));++i) {
        galleryEnabled[i] = readGallery(ini.c_str(), Gallery(i)).enabled;
    }
}

bool wangOwned()
{
    // The mission interpreter's global operand follows the active script-space allocation
    uint8_t addressInstruction = 0;
    uintptr_t globals = 0;
    const bool haveGlobals = gta::read(0x4647CA, addressInstruction) && addressInstruction == 0x05 &&
        gta::read(0x4647CB, globals) && globals;
    // Validate the STEAL loop's comparison against four missions before reading its global
    std::array<uint8_t, 7> condition{};
    if (haveGlobals && gta::read(globals + 69513, condition) &&
        condition[0] == 0x38 && condition[1] == 0 && condition[2] == 2 &&
        condition[5] == 4 && condition[6] == 4) {
        const unsigned offset = condition[3] | (unsigned(condition[4]) << 8);
        int passed = 0;
        if (offset == 544 * 4 && gta::read(globals + offset, passed) && passed > 0 && passed <= 4) return true;
    }
    uintptr_t script = 0;
    if (!gta::read(0xA8B42C,script)) return false;
    for (int i=0;script && i<256;++i) {
        std::array<char,8> name{};
        if (!gta::read(script+8,name)) break;
        if (!_strnicmp(name.data(),"STEAL",6)) return true;
        if (!gta::read(script,script)) break;
    }
    return false;
}

void removeGalleryBlips()
{
    for (int& handle:galleryBlips) {
        if (handle != -1) reinterpret_cast<void(__cdecl*)(int)>(0x587CE0)(handle);
        handle = -1;
    }
}

bool (__cdecl* originalRadarSave)() = nullptr;
bool __cdecl saveRadarWithoutDealers()
{
    removeGalleryBlips();
    return originalRadarSave();
}

void (__cdecl* originalCoordBlip)(int,bool) = nullptr;
void (__cdecl* originalRadarSprite)(int,float,float,unsigned char) = nullptr;
const char* galleryMapLabel = nullptr;
void __cdecl drawGalleryCoord(int index,bool sprite)
{
    const auto* previous = galleryMapLabel;
    galleryMapLabel = nullptr;
    if (sprite && game::at<bool>(0xBA6748 + 0x59)) {
        for (int i=1;i<int(std::size(galleries));++i) {
            if (galleryBlips[i] != -1 && (galleryBlips[i] & 0xffff) == index) galleryMapLabel = galleries[i].name;
        }
    }
    originalCoordBlip(index,sprite);
    galleryMapLabel = previous;
}
void __cdecl drawGallerySprite(int sprite,float x,float y,unsigned char alpha)
{
    originalRadarSprite(sprite,x,y,alpha);
    if (sprite != 55 || !galleryMapLabel) return;
    const auto scale = game::at<std::array<float,2>>(0xC71A64);
    const auto color = game::at<uint32_t>(0xC71A60);
    game::at<std::array<float,2>>(0xC71A64) = {.32f,.75f};
    game::at<uint32_t>(0xC71A60) = 0xffffffff;
    reinterpret_cast<void(__cdecl*)(float,float,const char*)>(0x71A700)(x+12.f,y-7.f,galleryMapLabel);
    game::at<std::array<float,2>>(0xC71A64) = scale;
    game::at<uint32_t>(0xC71A60) = color;
}

bool (__thiscall* originalGalleryEnterVehicle)(void*) = nullptr;
bool (__thiscall* originalGalleryEnterVehicleJustDown)(void*) = nullptr;

bool galleryCapturesVehicleEntry()
{
    const auto gate = game::openState(true);
    if (state.open || !gate.ready() || gate.inVehicle || game::mission() ||
        game::at<int>(0xB72914) != 0) return false;
    const V3 player = game::position(game::player());
    for (int i=1;i<int(std::size(galleries));++i) {
        if (!galleryEnabled[i] || (Gallery(i)==Gallery::Wang && !wangOwned())) continue;
        const V3 delta = player-galleries[i].entry;
        if (dot(delta,delta)<=10.f) return true;
    }
    return false;
}

bool __fastcall galleryEnterVehicle(void* pad,void*)
{
    const bool pressed = originalGalleryEnterVehicle(pad);
    return pressed && !(pad == reinterpret_cast<void*(__cdecl*)(int)>(0x53FB70)(0) &&
                        galleryCapturesVehicleEntry());
}

bool __fastcall galleryEnterVehicleJustDown(void* pad,void*)
{
    const bool pressed = originalGalleryEnterVehicleJustDown(pad);
    return pressed && !(pad == reinterpret_cast<void*(__cdecl*)(int)>(0x53FB70)(0) &&
                        galleryCapturesVehicleEntry());
}

void updateGalleryMarkers(const game::OpenState& gate, bool confirm, uint64_t tick, double dt)
{
    void* nativePad = reinterpret_cast<void*(__cdecl*)(int)>(0x53FB70)(0);
    const bool nativeConfirm = nativePad && originalGalleryEnterVehicle(nativePad);
    const bool pressed = galleryReturn.poll(confirm || nativeConfirm,tick);
    if (gate.gameState != 9 || gate.loadingData || !gate.alive || gate.menu || state.open || returnFade.active() ||
        game::at<int>(0xB72914) != 0) {
        approaching = Gallery::Main;
        if (!state.open) arrivalTime = -1;
        return;
    }
    const V3 player = game::position(game::player());
    Gallery nearest = Gallery::Main;
    for (int i=1;i<int(std::size(galleries));++i) {
        auto& handle = galleryBlips[i];
        if (!galleryEnabled[i]) {
            if (handle != -1) reinterpret_cast<void(__cdecl*)(int)>(0x587CE0)(handle);
            handle = -1;
            continue;
        }
        const auto& location = galleries[i];
        if (handle == -1) {
            handle = reinterpret_cast<int(__cdecl*)(int,V3,unsigned,int,const char*)>(0x583920)(4,location.entry,0,2,nullptr);
            if (handle != -1) reinterpret_cast<void(__cdecl*)(int,int)>(0x583D70)(handle,55);
        }
        const V3 delta = player-location.entry;
        const float distance = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
        if (distance > 6400 || (Gallery(i)==Gallery::Wang && !wangOwned())) continue;
        V3 at = location.entry;
        reinterpret_cast<void(__cdecl*)(unsigned,unsigned short,V3&,float,unsigned char,unsigned char,unsigned char,unsigned char,unsigned short,float,short)>(0x725BA0)(
            0x47543400u + unsigned(i),1,at,2.f,255,0,0,228,2048,.2f,0);
        if (gate.ready() && !game::mission() && distance <= 10.f) nearest = Gallery(i);
    }
    if (nearest != approaching) {
        approaching = nearest;
        arrivalTime = -1;
        if (nearest != Gallery::Main) reinterpret_cast<void(__cdecl*)(const char*,bool,bool,bool)>(0x588BE0)(
            galleries[int(nearest)].prompt,true,false,false);
    }
    if (nearest == Gallery::Main) return;
    if (pressed && arrivalTime < 0) arrivalTime=0;
    if (arrivalTime < 0) return;
    arrivalTime += dt;
    if (arrivalTime >= galleryHalfFade && gallery == Gallery::Main && !catalog.empty()) {
        mainPreferences = state.ui.preferences();
        mainVehicle = state.selected < entries.size() ? entries[state.selected].id : -1;
        gallery=nearest;
        openIntent.request(tick);
    }
}
