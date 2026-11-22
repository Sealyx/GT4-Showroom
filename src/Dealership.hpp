#pragma once
#include <windows.h>
#include <array>
#include <string>
#include <vector>
#include <sstream>
#include <climits>
#include <cerrno>

namespace gt4 {
enum class Gallery { Main, Wang, Coutt, Verdant, Grotti };
struct GalleryProfile {
    const wchar_t* section;
    const char* name;
    const char* prompt;
    V3 entry, spawn, shopSpawn;
    int music;
    unsigned classes;
    bool fullService;
};
enum GalleryClass { Poor, Normal, Rich, Executive, Taxi, Moped, Motorbikes, Bikes, Big, Worker, Planes, Helicopters, Boats, RC, Special };
constexpr unsigned classBit(GalleryClass group) { return 1u << unsigned(group); }
inline constexpr unsigned basicCars = classBit(Poor) | classBit(Normal);
inline constexpr unsigned twoWheels = classBit(Moped) | classBit(Motorbikes) | classBit(Bikes);
inline constexpr GalleryProfile galleries[]{
    {L"GT4Showroom","San Andreas Motor Collection","", {}, {}, {}, 1, 0x7fff, true},
    {L"WangCars","Wang Cars","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Wang Cars vehicle gallery.", {-1954,293,41},{-1927,273,40},{-1937,231,34},2,basicCars|classBit(Rich)|classBit(Moped)|classBit(Motorbikes),true},
    {L"CouttandSchutz","Coutt and Schutz","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Coutt and Schutz gallery.",{2132,-1148,24},{2132,-1128,25},{2132,-1128,25},2,basicCars,false},
    {L"VerdantBikes","Verdant Bikes","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Verdant Bikes gallery.",{538,2364,30},{538,2353,31},{538,2353,31},3,twoWheels,false},
    {L"GrottiDealership","Grotti Dealership","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Grotti Dealership gallery.",{543,-1292,17},{529,-1321,17},{529,-1321,17},2,0,false}
};
inline GalleryClass galleryClass(const Entry& e) {
    switch(e.id) {
        case 441: case 464: case 465: case 501: case 564: case 594: return RC;
    }
    switch(e.kind) {
        case 3: case 7: return Helicopters;
        case 4: case 8: return Planes;
        case 5: return Boats;
        case 6: case 11: return Special;
        case 9: return e.classId == 7 ? Moped : Motorbikes;
        case 10: return Bikes;
    }
    static constexpr GalleryClass classes[]{Normal,Poor,Rich,Executive,Worker,Big,Taxi,Moped,Motorbikes,Boats,Boats,Bikes,Special};
    return classes[classIndex(e.classId)];
}
inline int galleryBackground(const wchar_t* ini, Gallery gallery) {
    const int fallback = gallery == Gallery::Coutt ? AgedConcreteBackground :
                         gallery == Gallery::Verdant ? DarkGrayBackground : gallery == Gallery::Grotti ? RedBackground : DefaultBackground;
    wchar_t value[128]{};
    GetPrivateProfileStringW(galleries[int(gallery)].section, L"Background", L"", value, 128, ini);
    for (int i = 0; i < studioBackgroundCount; ++i) {
        const std::string name = studioBackgroundNames[size_t(i)];
        if (i != IndustrialGarageBackground && !_wcsicmp(value, std::wstring(name.begin(), name.end()).c_str())) return i;
    }
    return fallback;
}
inline bool saveGalleryBackground(const wchar_t* ini, Gallery gallery, int selection) {
    if (gallery == Gallery::Main || selection < 0 || selection >= studioBackgroundCount || selection == IndustrialGarageBackground) return false;
    const std::string name = studioBackgroundNames[size_t(selection)];
    return WritePrivateProfileStringW(galleries[int(gallery)].section, L"Background",
                                     std::wstring(name.begin(), name.end()).c_str(), ini) != FALSE;
}
struct GalleryOptions {
    bool enabled = true;
    unsigned allowed = 0;
    int music = 1;
    std::vector<int> models;
};
inline GalleryOptions readGallery(const wchar_t* ini, Gallery gallery) {
    const auto& profile = galleries[int(gallery)];
    GalleryOptions options;
    options.allowed=profile.classes;
    options.music=profile.music;
    options.music = std::clamp(int(GetPrivateProfileIntW(profile.section,L"Music",profile.music,ini)),1,3);
    if (gallery == Gallery::Main) return options;
    options.enabled = GetPrivateProfileIntW(profile.section,L"Enable",1,ini) != 0;
    if (gallery == Gallery::Grotti) {
        std::vector<wchar_t> list(32768);
        GetPrivateProfileStringW(profile.section,L"Models",L"415,451",list.data(),DWORD(list.size()),ini);
        std::wistringstream input(list.data());
        std::wstring token;
        while (std::getline(input,token,L',')) {
            wchar_t* end=nullptr;
            errno=0;
            const long id=wcstol(token.c_str(),&end,10);
            while (end && iswspace(*end)) ++end;
            if (errno != ERANGE && end != token.c_str() && end && !*end && id>0 && id<=INT_MAX &&
                std::find(options.models.begin(),options.models.end(),int(id))==options.models.end())
                options.models.push_back(int(id));
        }
        return options;
    }
    if (GetPrivateProfileIntW(profile.section,L"AllClass",0,ini)) { options.allowed = 0x7fff; return options; }
    constexpr const wchar_t* keys[]{L"EnablePoor",L"EnableNormal",L"EnableRich",L"EnableExecutive",L"EnableTaxi",L"EnableMoped",L"EnableMotorbikes",L"EnableBikes",L"EnableBig",L"EnableWorker",L"EnablePlanes",L"EnableHelicopters",L"EnableBoats",L"EnableRC",L"EnableSpecial"};
    for (unsigned i=0;i<std::size(keys);++i) {
        if (!(profile.classes & (1u<<i)) && GetPrivateProfileIntW(profile.section,keys[i],0,ini)) options.allowed |= 1u<<i;
    }
    return options;
}
inline bool galleryCustomizes(Gallery context,const Entry& entry) {
    if (!galleries[int(context)].fullService || entry.kind==11) return false;
    switch (galleryClass(entry)) {
        case RC: case Planes: case Helicopters: case Worker: case Moped: case Motorbikes: case Bikes: return false;
        default: return true;
    }
}
}
