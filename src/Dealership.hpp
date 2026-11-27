#pragma once
#include "TrackCatalog.hpp"
#include <windows.h>
#include <array>
#include <string>
#include <vector>
#include <sstream>
#include <climits>
#include <cerrno>

namespace gt4 {
enum class Gallery { Main, Wang, Coutt, Verdant, Grotti, Ottos, AutoBahn };
inline bool galleryChargesVehicle(Gallery context) {
    return context != Gallery::Main && context != Gallery::Wang;
}
struct GalleryProfile {
    const wchar_t* section;
    const char* name;
    const char* prompt;
    V3 entry, spawn, shopSpawn;
    int music;
    unsigned classes;
    bool fullService;
    float heading;
};
enum GalleryClass { Poor, Normal, Rich, Executive, Taxi, Moped, Motorbikes, Bikes, Big, Worker, Planes, Helicopters, Boats, RC, Special };
constexpr unsigned classBit(GalleryClass group) { return 1u << unsigned(group); }
inline constexpr unsigned basicCars = classBit(Poor) | classBit(Normal);
inline constexpr unsigned twoWheels = classBit(Moped) | classBit(Motorbikes) | classBit(Bikes);
inline constexpr GalleryProfile galleries[]{
    {L"Main","San Andreas Motor Collection","", {}, {}, {}, 1, 0x7fff, true, 0.f},
    {L"WangCars","Wang Cars","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Wang Cars vehicle gallery.", {-1954,293,41},{-1928,269,40},{-1937,231,34},2,basicCars|classBit(Rich)|classBit(Moped)|classBit(Motorbikes),true,pi},
    {L"CouttandSchutz","Coutt and Schutz","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Coutt and Schutz gallery.",{2132,-1148,24},{2132,-1128,25},{2132,-1128,25},2,basicCars,false,.785398f},
    {L"VerdantBikes","Verdant Bikes","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Verdant Bikes gallery.",{538,2364,30},{538,2353,31},{538,2353,31},3,twoWheels,false,2.51327f},
    {L"GrottiDealership","Grotti Showroom","Press ~k~~VEHICLE_ENTER_EXIT~ to view the Grotti Dealership gallery.",{543,-1292,17},{529,-1321,17},{529,-1321,17},2,0,false,2.51327f},
    {L"OttosAutos","Otto's Autos","Press ~k~~VEHICLE_ENTER_EXIT~ to view Otto's Autos' vehicle gallery.",{-1660,1209,21},{-1648,1223,6},{-1648,1223,6},2,classBit(Rich)|classBit(Executive)|classBit(Motorbikes),false,-2.4279f},
    {L"AutoBahn","Auto Bahn","Press ~k~~VEHICLE_ENTER_EXIT~ to view Auto Bahn's vehicle gallery.",{2200,1391,10},{2168,1410,10},{2168,1410,10},2,0x7fff,false,0.0f}
};
inline int galleryLogoNumber(Gallery gallery) {constexpr int slots[]{0,1,3,6,4,2,5};return slots[int(gallery)];}
inline bool galleryStoryAllows(Gallery gallery,float cities,bool testDrive) {
    if(gallery==Gallery::Ottos)return cities>=1.f&&!testDrive;
    if(gallery==Gallery::AutoBahn||gallery==Gallery::Verdant)return cities>=2.f;
    return true;
}
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
inline int galleryStyle(const wchar_t* ini, Gallery gallery, int mainPreference)
{
    if (gallery == Gallery::Main) return mainPreference;
    const auto section = galleries[int(gallery)].section;
    wchar_t value[32]{};
    GetPrivateProfileStringW(section, L"Style", L"Default", value, 32, ini);
    const int preset = !_wcsicmp(value, L"Prologue") ? 1 : 0;
    if (gallery == Gallery::Main) return mainPreference ? mainPreference : preset;
    GetPrivateProfileStringW(section, L"StyleOverride", L"", value, 32, ini);
    return !_wcsicmp(value,L"Prologue") ? 1 : preset;
}
inline int galleryBackground(const wchar_t* ini, Gallery gallery) {
    const int fallback = gallery == Gallery::Ottos ? WeatheredSlateBackground : gallery == Gallery::AutoBahn ? VerticalBrushedBackground : gallery == Gallery::Coutt ? AgedConcreteBackground :
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
    uint64_t countries=0;
    std::vector<int> models;
};
inline GalleryOptions readGallery(const wchar_t* ini, Gallery gallery) {
    musicTrackFolder=std::filesystem::path(ini).parent_path()/L"GT4Showroom/audio";
    const auto& profile = galleries[int(gallery)];
    GalleryOptions options;
    options.allowed=profile.classes;
    options.music=profile.music;
    const auto musicPath=gallery==Gallery::Main ? (std::filesystem::path(ini).parent_path()/L"GT4Showroom/settings.ini").wstring() : std::wstring(ini);
    options.music = validatedMusicTrack(int(GetPrivateProfileIntW(gallery==Gallery::Main?L"Main":profile.section,L"Music",profile.music,musicPath.c_str())));
    if (gallery == Gallery::Main) return options;
    options.enabled = GetPrivateProfileIntW(profile.section,L"Enable",1,ini) != 0;
    if(gallery==Gallery::AutoBahn) {
        if(GetPrivateProfileIntW(profile.section,L"AllCountries",0,ini)) options.countries=allCountryBits;
        else for(auto country:{Country::UK,Country::Germany,Country::Netherlands,Country::Belgium,Country::Italy,Country::Czech})options.countries|=uint64_t(1)<<unsigned(country);
        return options;
    }
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
        if (GetPrivateProfileIntW(profile.section,keys[i],(profile.classes & (1u<<i))?1:0,ini)) options.allowed |= 1u<<i;
        else options.allowed &= ~(1u<<i);
    }
    return options;
}
inline bool galleryAccepts(Gallery context,const GalleryOptions& options,const Entry& entry) {
    if(context==Gallery::Main)return true;
    if(context==Gallery::Grotti)return std::find(options.models.begin(),options.models.end(),entry.id)!=options.models.end();
    if(entry.kind==6)return false;
    if(context==Gallery::AutoBahn)return (options.countries&(uint64_t(1)<<unsigned(entry.country)))!=0;
    return (options.allowed&classBit(galleryClass(entry)))!=0;
}
inline bool galleryCustomizes(Gallery context,const Entry& entry) {
    if (!galleries[int(context)].fullService || entry.kind==11) return false;
    switch (galleryClass(entry)) {
        case RC: case Planes: case Helicopters: case Worker: case Moped: case Motorbikes: case Bikes: return false;
        default: return true;
    }
}
}
