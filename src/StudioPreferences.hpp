#pragma once
#include "StudioBackgrounds.hpp"
#include "AudioLevels.hpp"
#include "Countries.hpp"
#include <map>
#include <vector>
#include <sstream>
#include <climits>

namespace gt4
{
    struct PresentationPreset
    {
        uint8_t floorQuality = inheritedFloorQuality;
        bool shadows = true;
        uint8_t reflectionQuality = 3;

        PresentationPreset(uint8_t floor = inheritedFloorQuality, bool shadow = true, uint8_t quality = 3)
            : floorQuality(floor), shadows(shadow), reflectionQuality(floor > 0 && floor <= 4 ? floor : quality) {}

        bool operator==(const PresentationPreset& other) const
        {
            return floorQuality == other.floorQuality && shadows == other.shadows && reflectionQuality == other.reflectionQuality;
        }
    };

    inline int presentationDefaultFloor(size_t slot)
    {
        return slot&1 ? 3 : defaultFloorQuality(0,int(slot/2));
    }

    inline bool samePresentationPreset(const PresentationPreset& a,const PresentationPreset& b,size_t slot)
    {
        const auto floor=[&](const PresentationPreset& value) {
            return value.floorQuality==inheritedFloorQuality ? (presentationDefaultFloor(slot)?int(value.reflectionQuality):0) : int(value.floorQuality);
        };
        return floor(a)==floor(b)&&a.shadows==b.shadows&&a.reflectionQuality==b.reflectionQuality;
    }

    inline bool samePresentationPresets(const std::array<PresentationPreset,reflectionPresetCount>& a,
                                        const std::array<PresentationPreset,reflectionPresetCount>& b)
    {
        for(size_t i=0;i<a.size();++i)if(!samePresentationPreset(a[i],b[i],i))return false;
        return true;
    }

    inline constexpr uint64_t favoritesPresentationContext = uint64_t(1) << 63;

    inline bool predefinedPresentationContext(uint64_t key)
    {
        if(key==favoritesPresentationContext)return true;
        for(const auto country:{Country::Japan,Country::Germany,Country::SouthKorea,Country::UK,Country::Australia,Country::Russia,Country::France,Country::Italy,Country::China,Country::Brazil,Country::Sweden,Country::USA,Country::Denmark,Country::Malaysia,Country::Indonesia})
            if(key==(uint64_t(1u)<<unsigned(country)))return true;
        for(unsigned category:{7u,8u,11u})if(key==(uint64_t(1u)<<(32+category)))return true;
        return false;
    }

    inline uint64_t presentationContextKey(uint32_t countries, int country, uint32_t classes, int category)
    {
        const auto normalized = [](uint32_t mask, int selected, unsigned count) {
            const uint32_t all = ~uint32_t(0) >> (32 - count);
            mask = mask ? mask : selected >= 0 && unsigned(selected) < count ? 1u << selected : 0;
            return mask == all ? 0u : mask;
        };
        return uint64_t(normalized(countries, country, countryCount)) |
               (uint64_t(normalized(classes, category, 13)) << 32);
    }

    struct PresentationContext
    {
        int backgroundSelection = 0;
        bool backgroundOverridden = false;
        std::array<PresentationPreset, reflectionPresetCount> presets{};

        bool valid() const
        {
            return backgroundSelection >= 0 && backgroundSelection < studioBackgroundCount &&
                std::all_of(presets.begin(), presets.end(), [](const PresentationPreset& preset) {
                    return (preset.floorQuality <= 4 || preset.floorQuality == inheritedFloorQuality) && preset.reflectionQuality >= 1 && preset.reflectionQuality <= 4;
                });
        }

        bool operator==(const PresentationContext& other) const
        {
            return backgroundSelection == other.backgroundSelection &&
                backgroundOverridden == other.backgroundOverridden && samePresentationPresets(presets,other.presets);
        }
    };

    inline auto canonicalPresentationContexts(std::map<uint64_t,PresentationContext> contexts)
    {
        for(auto item=contexts.begin();item!=contexts.end();) {
            auto& context=item->second;
            if(predefinedPresentationContext(item->first)) {
                const auto key=item->first;
                if(context.backgroundSelection==(key==favoritesPresentationContext?IndustrialGarageBackground:studioBackgroundForFilters(uint32_t(key),-1,uint32_t(key>>32),-1)))
                    context.backgroundOverridden=false;
                if(!context.backgroundOverridden&&samePresentationPresets(context.presets,{})) {
                    item=contexts.erase(item);continue;
                }
            }
            if(!context.backgroundOverridden)context.backgroundSelection=0;
            ++item;
        }
        return contexts;
    }

    struct StudioPreferences
    {
        int sortMode = 5, countryFilter = -1, classFilter = -1;
        uint32_t countryMask = 0, classMask = 0;
        int backgroundSelection = 0, showroomStyle = 0;
        int unitFormat = 0;
        int antialiasing = 2, ambientOcclusion = 2;
        int vehicleReflections = 3;
        bool backgroundOverridden = false;
        std::array<PresentationPreset, reflectionPresetCount> presets{};
        std::map<uint64_t, PresentationContext> contexts;
        bool countryPresentationPriority = false;
        int language = 0;
        bool carLogo = false, brandLogo = false, displayModelId = false, displayModelName = false;
        // bytes keep existing settings readable. playback uses gta's sliders
        AudioLevels audio;

        bool valid() const
        {
            return contexts.size() <= 4096 &&
                   std::all_of(contexts.begin(), contexts.end(), [](const auto& item) {
                       const uint64_t key = item.first;
                       return key == favoritesPresentationContext ? item.second.valid() : key != 0 && presentationContextKey(uint32_t(key), -1, uint32_t(key >> 32), -1) == key &&
                           (uint32_t(key) >> countryCount) == 0 && (key >> 45) == 0 && item.second.valid();
                   }) &&
                   showroomStyle >= 0 && showroomStyle < studioStyleCount && backgroundSelection >= 0 && backgroundSelection < studioBackgroundCount && sortMode >= 0 && sortMode <= 5 && countryFilter >= -1 && countryFilter < int(countryCount) &&
                   classFilter >= -1 && classFilter <= 12 && unitFormat >= 0 && unitFormat <= 2 &&
                   antialiasing >= 0 && antialiasing <= 3 && ambientOcclusion >= 0 && ambientOcclusion <= 3 &&
                   vehicleReflections >= 0 && vehicleReflections <= 4 &&
                   std::all_of(presets.begin(), presets.end(), [](const PresentationPreset& preset) {
                       return (preset.floorQuality <= 4 || preset.floorQuality == inheritedFloorQuality) && preset.reflectionQuality >= 1 && preset.reflectionQuality <= 4;
                   }) &&
                   language >= 0 && language < 7 &&
                   (countryMask >> countryCount) == 0 && (classMask >> 13) == 0 &&
                   std::all_of(
                       audio.value.begin(),
                       audio.value.end(),
                       [](int value) {
                           return value >= 0 && value <= 100;
                       });
        }

        bool operator==(const StudioPreferences& other) const
        {
            return showroomStyle == other.showroomStyle && (!backgroundOverridden || backgroundSelection == other.backgroundSelection) &&
                   backgroundOverridden == other.backgroundOverridden &&
                   countryPresentationPriority == other.countryPresentationPriority &&
                   canonicalPresentationContexts(contexts) == canonicalPresentationContexts(other.contexts) && samePresentationPresets(presets,other.presets) && sortMode == other.sortMode && countryFilter == other.countryFilter &&
                   classFilter == other.classFilter && carLogo == other.carLogo && brandLogo == other.brandLogo &&
                   displayModelId == other.displayModelId && displayModelName == other.displayModelName &&
                   audio == other.audio && countryMask == other.countryMask && classMask == other.classMask &&
                   unitFormat == other.unitFormat &&
                   antialiasing == other.antialiasing && ambientOcclusion == other.ambientOcclusion &&
                   vehicleReflections == other.vehicleReflections && language == other.language;
        }

        bool operator!=(const StudioPreferences& other) const
        {
            return !(*this == other);
        }
    };

    inline uint64_t preferredPresentationContext(uint64_t key, bool countryPriority)
    {
        if(key==favoritesPresentationContext)return key;
        const uint64_t countries=uint32_t(key), classes=key&0xFFFFFFFF00000000ull;
        return countries && classes ? (countryPriority ? countries : classes) : key;
    }

    template<class Preferences> int presentationBackground(const Preferences& settings, uint64_t key)
    {
        if(!key)return settings.backgroundOverridden?settings.backgroundSelection:DefaultBackground;
        const auto found=settings.contexts.find(key);
        if(found!=settings.contexts.end()&&found->second.backgroundOverridden)return found->second.backgroundSelection;
        const auto fallbackKey=preferredPresentationContext(key,settings.countryPresentationPriority);
        const auto fallback=settings.contexts.find(fallbackKey);
        if(fallback!=settings.contexts.end()&&fallback->second.backgroundOverridden)return fallback->second.backgroundSelection;
        if(key==favoritesPresentationContext)return IndustrialGarageBackground;
        return studioBackgroundForFilters(uint32_t(fallbackKey),-1,uint32_t(fallbackKey>>32),-1);
    }

    inline bool decodeLegacyStudioPreferences(const std::vector<uint8_t>& bytes, StudioPreferences& result)
    {
        const size_t size = bytes.size();
        if (size < 20 || size > 287) { return false; }
        const std::array<uint8_t, 4> magic{{'G', 'T', '4', 'P'}};
        const uint8_t version = bytes[4];
        const bool legacy = size == 20 && version == 1;
        const bool old = size == 32 && version >= 2 && version <= 6;
        const bool perPreset = size >= 33 && version == 7;
        if ((!legacy && !old && !perPreset) ||
            !std::equal(magic.begin(), magic.end(), bytes.begin()) || (bytes[8] & 0xF0) != 0 ||
            (version == 6 && (bytes[30] & ~3u)) ||
            (perPreset && (bytes[19] != 0 || bytes[24] != 0 || bytes[29] != 0 || (bytes[30] & ~1u))) ||
            (version < 6 && std::any_of(bytes.begin() + (legacy ? 12 : 25 + version), bytes.begin() + size - 1,
                                        [](uint8_t value) { return value != 0; }))) {
            return false;
        }
        uint8_t check = 0xA7;
        for (size_t i = 0; i < size - 1; ++i) {
            check ^= bytes[i];
        }
        if (check != bytes[size - 1]) {
            return false;
        }
        StudioPreferences loaded;
        loaded.sortMode = bytes[5];
        loaded.countryFilter = int(bytes[6]) - 1;
        loaded.classFilter = int(bytes[7]) - 1;
        loaded.carLogo = (bytes[8] & 1) != 0;
        loaded.brandLogo = (bytes[8] & 2) != 0;
        loaded.displayModelId = (bytes[8] & 4) != 0;
        loaded.displayModelName = (bytes[8] & 8) != 0;
        for (size_t i = 0; i < loaded.audio.value.size(); ++i) {
            loaded.audio.value[i] = bytes[9 + i];
        }
        if (!legacy) {
            for (unsigned i = 0; i < 4; ++i) {
                loaded.countryMask |= uint32_t(bytes[12 + i]) << (8 * i);
            }
            loaded.classMask = uint32_t(bytes[16]) | (uint32_t(bytes[17]) << 8);
            loaded.unitFormat = bytes[18];
            if (bytes[19] > 1) {
                return false;
            }
            if (bytes[20] > 1 || bytes[21] > 2) { return false; }
            loaded.antialiasing = bytes[22];
            loaded.ambientOcclusion = bytes[23];
            loaded.vehicleReflections = bytes[25];
            loaded.language = bytes[26];
            loaded.backgroundSelection = bytes[4] >= 3 ? bytes[27] : 0;
            loaded.showroomStyle = bytes[4] >= 4 ? bytes[28] : 0;
        }
        loaded.backgroundOverridden = version >= 6 ? (bytes[30] & 1u) != 0
                                                   : loaded.backgroundSelection != DefaultBackground;
        if (!loaded.valid()) { return false; }
        if (perPreset) {
            for (size_t i = 0; i < size - 32; ++i) {
                const uint8_t flags = bytes[31 + i];
                const uint8_t quality = flags & 0x0F;
                if ((flags & 0x70) || (quality > 4 && quality != 0x0F)) { return false; }
                if (i < loaded.presets.size()) {
                    loaded.presets[i] = {quality == 0x0F ? inheritedFloorQuality : quality, (flags & 0x80) == 0};
                }
            }
        } else if (old) {
            const uint8_t floor = bytes[24];
            const uint8_t quality = version >= 5 ? bytes[29] : (floor ? floor : 3);
            if (floor > 4 || quality < 1 || quality > 4) { return false; }
            // Pre-v6 files contain the effective value. Infer a manual choice with the original defaults
            const bool automaticRed = !loaded.backgroundOverridden &&
                !studioBikeSelected(loaded.classMask, loaded.classFilter) &&
                studioCountrySelected(loaded.countryMask, loaded.countryFilter, Country::Japan);
            const bool oldReflective = loaded.showroomStyle == 1 || automaticRed ||
                loaded.backgroundSelection == RedBackground || loaded.backgroundSelection >= StaticBackground;
            const bool manual = version == 6 ? (bytes[30] & 2u) != 0
                : floor != (oldReflective ? quality : 0) || quality != 3;
            const int background = loaded.backgroundOverridden ? loaded.backgroundSelection :
                studioBackgroundForFilters(loaded.countryMask, loaded.countryFilter, loaded.classMask, loaded.classFilter);
            auto& preset = loaded.presets[reflectionPreset(loaded.showroomStyle, background)];
            if (manual) { preset.floorQuality = floor; preset.reflectionQuality = quality; }
            preset.shadows = bytes[19] != 0;
        }
        result = loaded;
        return true;
    }

    inline bool loadBinaryStudioPreferences(const std::filesystem::path& path, StudioPreferences& result)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) { return false; }
        const auto length = input.tellg();
        if (length < 20 || length > 1048576) { return false; }
        std::vector<uint8_t> bytes(size_t(length), 0);
        input.seekg(0);
        if (!input.read(reinterpret_cast<char*>(bytes.data()), length)) { return false; }
        StudioPreferences loaded;
        if (bytes[4] <= 7) {
            if (!decodeLegacyStudioPreferences(bytes, loaded)) { return false; }
            const auto key = presentationContextKey(loaded.countryMask, loaded.countryFilter, loaded.classMask, loaded.classFilter);
            if (key) {
                loaded.contexts.emplace(key, PresentationContext{loaded.backgroundSelection, loaded.backgroundOverridden, loaded.presets});
            }
        } else {
            if (bytes[4] != 8 || bytes.size() < 35) { return false; }
            uint8_t check = 0xA7;
            for (size_t i = 0; i + 1 < bytes.size(); ++i) { check ^= bytes[i]; }
            if (check != bytes.back()) { return false; }
            const size_t count = bytes[31];
            const size_t record = 8 + count;
            if (!count || bytes.size() < 35 + count) { return false; }
            const size_t entries = size_t(bytes[32 + count]) | (size_t(bytes[33 + count]) << 8);
            if (entries > 4096 || bytes.size() != 35 + count + record * entries) { return false; }
            std::vector<uint8_t> legacy(bytes.begin(), bytes.begin() + 31);
            legacy[4] = 7;
            if (legacy[29] > 1) { return false; }
            const bool countryPriority = legacy[29] != 0;
            legacy[29] = 0;
            legacy.insert(legacy.end(), bytes.begin() + 32, bytes.begin() + 32 + count);
            legacy.push_back(0xA7);
            for (size_t i = 0; i + 1 < legacy.size(); ++i) { legacy.back() ^= legacy[i]; }
            if (!decodeLegacyStudioPreferences(legacy, loaded)) { return false; }
            loaded.countryPresentationPriority = countryPriority;
            size_t offset = 34 + count;
            for (size_t i = 0; i < entries; ++i) {
                uint32_t country = 0;
                for (unsigned b = 0; b < 4; ++b) { country |= uint32_t(bytes[offset + b]) << (8 * b); }
                const uint32_t category = uint32_t(bytes[offset + 4]) | (uint32_t(bytes[offset + 5]) << 8);
                const uint64_t key = uint64_t(country) | (uint64_t(category) << 32);
                PresentationContext context;
                context.backgroundSelection = bytes[offset + 6];
                if (bytes[offset + 7] > 1) { return false; }
                context.backgroundOverridden = bytes[offset + 7] != 0;
                for (size_t slot = 0; slot < count; ++slot) {
                    const uint8_t flags = bytes[offset + 8 + slot], quality = flags & 0x0F;
                    if ((flags & 0x70) || (quality > 4 && quality != 0x0F)) { return false; }
                    if (slot < context.presets.size()) {
                        context.presets[slot] = {quality == 0x0F ? inheritedFloorQuality : quality, (flags & 0x80) == 0};
                    }
                }
                if (!loaded.contexts.emplace(key, context).second) { return false; }
                offset += record;
            }
        }
        if (!loaded.valid()) { return false; }
        result = std::move(loaded);
        return true;
    }

inline std::string settingTrim(std::string value)
{
    const auto first=value.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)return {};
    return value.substr(first,value.find_last_not_of(" \t\r\n")-first+1);
}

inline std::string settingFold(std::string value)
{
    for(char& c:value)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
    return value;
}

inline constexpr std::array<const char*,7> presentationLanguageNames{"English","Indonesian","Spanish","Chinese","Japanese","Korean","Russian"};

inline constexpr std::array<const char*,13> presentationClassKeys{
    "Normal","Poor","Rich","Executive","Worker","Heavy","Taxi","Moped","Motorbike","LeisureBoat","WorkBoat","Bicycle","Special"};

inline std::string presentationSection(uint64_t key)
{
    if(!key)return "Main";
    if(key==favoritesPresentationContext)return "Favorites";
    std::string name;
    const auto append=[&](uint32_t mask,const char* prefix,const auto& names){
        if(!mask)return;
        if(!name.empty())name+='.';
        name+=prefix;bool first=true;
        for(size_t i=0;i<std::size(names);++i)if(mask&(1u<<i)){
            if(!first)name+='+';
            const std::string token=names[i];
            name+=token=="Motorbike"?"Motorbikes":token=="Moped"?"Mopeds":token=="Bicycle"?"Bicycles":token;
            first=false;
        }
    };
    append(uint32_t(key),"Countries.",countryKeys);
    append(uint32_t(key>>32),"Classes.",presentationClassKeys);
    return name;
}

inline std::string presentationSlot(size_t slot)
{
    if(!(slot&1))return studioBackgroundNames[slot/2];
    return slot==1?"Prologue":"Slot"+std::to_string(slot);
}

inline bool saveStudioPreferences(const std::filesystem::path& path,const StudioPreferences& preferences)
{
    if(!preferences.valid())return false;
    std::ostringstream text;
    text<<"; GT4 Showroom presentation settings\n; Edit with the showroom closed, then reopen it or reload the script to apply changes\n"
        <<"; Background names: Default, Static, Red, Dark Gray, Brushed Steel, Dark Graphite, Wooden Taupe, Aged Concrete, Weathered Slate, Horizontal Brushed, Cracked Plaster, Vertical Brushed, Midnight Slate, Smoky Steel, Blue Mist.\n"
        <<"; Industrial Garage is the Favorites initial preset and is not listed in the Background menu.\n"
        <<"; Recommended applies to Background. FloorReflections is 0/1; floor quality is Low, Medium, High or Ultra.\n"
        <<"; Country and class sections are independent. Combined sections use Countries.X.Classes.Y.\n"
        <<"; LastFilter decides which preset an untouched combination inherits.\n"
        <<"; Logos: 0=None, 1=Manufacturer, 2=Vehicle name, 3=Both. Style is shared.\n"
        <<"; Only predefined categories and contexts customized in-game are generated.\n"
        <<"; Country/class root floor settings describe their Background in Default style.\n"
        <<"; Prologue and other customized backgrounds use independent Presets.<name> sections.\n"
        <<"; Main root floor settings describe its current Style; a matching Presets section overrides them.\n\n"
        <<"[Settings]\n; File format version - leave at 1\nFormatVersion=1\n; 0=None, 1=Manufacturer only, 2=Vehicle name only, 3=Both\nLogos="<<int(preferences.brandLogo | (preferences.carLogo << 1))
        <<"\n; Vehicle order: 1=Z-A, 2=A-Z, 3=Highest speed, 4=Country, 5=Model ID, 0=A-Z fallback\nSortMode="<<preferences.sortMode<<"\n; Last country filter: -1=All, 0=Unassigned, 1=USA, 2=Japan, 3=Italy, 4=France, 5=UK, 6=Germany, 7=Sweden, 8=China, 9=South Korea, 10=Canada, 11=Australia, 12=Russia, 13=Denmark, 14=Brazil, 15=Indonesia, 16=Malaysia\nCountryFilter="<<preferences.countryFilter<<"\n; Last class filter: -1=All, 0=Normal, 1=Poor, 2=Rich, 3=Executive, 4=Worker, 5=Heavy, 6=Taxi, 7=Mopeds, 8=Motorbikes, 9=Leisure boats, 10=Work boats, 11=Bicycles, 12=Special\nClassFilter="<<preferences.classFilter
        <<"\n; Multi-country filter bitmask - 0 uses CountryFilter; bit N selects country N above\nCountryMask="<<preferences.countryMask<<"\n; Multi-class filter bitmask - 0 uses ClassFilter; bit N selects class N above\nClassMask="<<preferences.classMask
        <<"\n; Country or Class - the last filter changed supplies an untouched combined preset\nLastFilter="<<(preferences.countryPresentationPriority?"Country":"Class")
        <<"\n; 0=Auto by country, 1=Metric, 2=Imperial\nUnitFormat="<<preferences.unitFormat<<"\n; Shared edge smoothing quality: 0=Off, 1=Standard, 2=High, 3=Ultra\nAntialiasing="<<preferences.antialiasing
        <<"\n; Shared ambient occlusion quality: 0=Off, 1=Standard, 2=High, 3=Ultra\nAmbientOcclusion="<<preferences.ambientOcclusion<<"\n; Shared vehicle reflection quality: Off, Low, Medium, High, Ultra - independent of floor reflections\nVehicleReflections="<<std::array<const char*,5>{"Off","Low","Medium","High","Ultra"}[size_t(preferences.vehicleReflections)]
        <<"\n; Interface language: English, Indonesian, Spanish, Chinese, Japanese, Korean, Russian - unavailable packs fall back to English\nLanguage="<<presentationLanguageNames[size_t(preferences.language)]
        <<"\n; 1=Show model IDs next to vehicle names, 0=Hide\nDisplayModelId="<<preferences.displayModelId<<"\n; 1=Show internal model names next to vehicle names, 0=Hide\nDisplayModelName="<<preferences.displayModelName
        <<"\n\n"
        <<"[Audio]\n; Retained master volume (0-100) for compatibility - playback follows GTA audio sliders\nMaster="<<preferences.audio.value[0]<<"\n; Retained music volume (0-100) for compatibility - GTA audio options control playback\nMusic="<<preferences.audio.value[1]<<"\n; Retained effects volume (0-100) for compatibility - GTA audio options control playback\nEffects="<<preferences.audio.value[2]<<"\n";
    static constexpr std::array<const char*,5> quality{"Off","Low","Medium","High","Ultra"};
    const auto controls=[&](const PresentationPreset& preset,size_t slot) {
        const int floor=preset.floorQuality==inheritedFloorQuality?(presentationDefaultFloor(slot)?preset.reflectionQuality:0):preset.floorQuality;
        text<<"; Floor reflections for this preset only: 0=Off, 1=On\nFloorReflections="<<int(floor>0)
            <<"\n; Saved floor reflection quality: Low, Medium, High, Ultra - retained while Off\nReflectionQuality="<<quality[preset.reflectionQuality]
            <<"\n; Vehicle shadows for this preset only: On or Off\nShadows="<<(preset.shadows?"On":"Off")<<"\n";
    };
    const auto section=[&](uint64_t key,const PresentationContext& context,bool configured){
        const auto name=presentationSection(key);
        text<<"\n["<<name<<"]\n";
        if(!key)text<<"; Shared showroom style: Default or Prologue - applies to every country and class\nStyle="<<(preferences.showroomStyle?"Prologue":"Default")<<"\n";
        text<<"; Background for this context; Recommended keeps its original or inherited default\nBackground=";
        if(context.backgroundOverridden)text<<studioBackgroundNames[size_t(context.backgroundSelection)];
        else if(!configured&&key)text<<studioBackgroundNames[size_t(key==favoritesPresentationContext?IndustrialGarageBackground:studioBackgroundForFilters(uint32_t(key),-1,uint32_t(key>>32),-1))];
        else text<<"Recommended";
        text<<"\n";
        const auto active=reflectionPreset(key?0:preferences.showroomStyle,presentationBackground(preferences,key));
        controls(context.presets[active],active);
        for(size_t i=0;i<context.presets.size();++i){
            if(i==active||((i&1)&&i!=1&&samePresentationPreset(context.presets[i],{},i)))continue;
            if(samePresentationPreset(context.presets[i],{},i))continue;
            text<<"\n["<<name<<".Presets."<<presentationSlot(i)<<"]\n";
            controls(context.presets[i],i);
        }
    };
    section(0,{preferences.backgroundSelection,preferences.backgroundOverridden,preferences.presets},true);
    auto contexts=preferences.contexts;
    contexts.try_emplace(favoritesPresentationContext);
    for(const auto country:{Country::Japan,Country::Germany,Country::SouthKorea,Country::UK,Country::Australia,Country::Russia,Country::France,Country::Italy,Country::China,Country::Brazil,Country::Sweden,Country::USA,Country::Denmark,Country::Malaysia,Country::Indonesia})contexts.try_emplace(uint64_t(1u<<unsigned(country)));
    for(unsigned category:{7u,8u,11u})contexts.try_emplace(uint64_t(1u<<category)<<32);
    for(const auto& item:contexts)section(item.first,item.second,preferences.contexts.count(item.first)!=0);
    std::error_code error;std::filesystem::create_directories(path.parent_path(),error);
    if(error)return false;
    auto temporary=path;temporary+=L".tmp";
    bool written=false;{
        std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
        auto content=text.str();
        const auto settingsAt=content.find("[Settings]\n"),mainAt=content.find("\n[Main]\n");
        if(settingsAt!=std::string::npos&&mainAt!=std::string::npos){
            const auto next=content.find("\n[",mainAt+2);
            if(next!=std::string::npos)content=content.substr(0,settingsAt)+content.substr(mainAt+1,next-mainAt-1)+"\n\n"+
                content.substr(settingsAt,mainAt-settingsAt)+content.substr(next);
        }
        if(content.size()>1048576){output.close();DeleteFileW(temporary.c_str());return false;}
        written=bool(output.write(content.data(),std::streamsize(content.size()))&&output.flush());
    }
    if(!written){DeleteFileW(temporary.c_str());return false;}
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        DeleteFileW(temporary.c_str());return false;
    }
    return true;
}

inline bool loadStudioPreferences(const std::filesystem::path& path,StudioPreferences& result)
{
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input)return false;
    const auto length=input.tellg();if(length<1||length>1048576)return false;
    input.seekg(0);std::string contents(size_t(length),'\0');
    if(!input.read(contents.data(),length))return false;
    input.close();
    if(contents.compare(0,4,"GT4P")==0)return loadBinaryStudioPreferences(path,result);
    if(contents.compare(0,3,"\xEF\xBB\xBF")==0)contents.erase(0,3);
    std::map<std::string,std::map<std::string,std::string>> sections;
    std::istringstream lines(contents);std::string line,current;
    while(std::getline(lines,line)){
        line=settingTrim(line);if(line.empty()||line[0]==';'||line[0]=='#'||line.compare(0,2,"//")==0)continue;
        if(line[0]=='['){
            if(line.back()!=']')return false;
            current=settingFold(settingTrim(line.substr(1,line.size()-2)));
            if(current.empty()||sections.count(current))return false;
            sections[current]={};continue;
        }
        const auto split=line.find('=');
        if(current.empty()||split==std::string::npos)return false;
        const auto key=settingFold(settingTrim(line.substr(0,split))),value=settingTrim(line.substr(split+1));
        if(key.empty()||value.empty()||!sections[current].emplace(key,value).second)return false;
    }
    std::map<std::string,std::map<std::string,std::string>> presentationControls;
    const auto canonical=[](std::string name) {
        if(name=="main")return std::string("preset.main");
        if(name=="favorites"||name.rfind("favorites.presets.",0)==0)return "preset."+name;
        if(name.rfind("countries.",0)==0)name="preset.country."+name.substr(10);
        else if(name.rfind("classes.",0)==0)name="preset.class."+name.substr(8);
        const auto mixed=name.find(".classes.");
        if(mixed!=std::string::npos)name.replace(mixed,9,".class.");
        return name;
    };
    if(sections.count("main")||sections.count("settings")){
        auto converted=std::map<std::string,std::map<std::string,std::string>>{};
        converted["showroom"]=sections["settings"];
        if(sections["main"].count("style"))converted["showroom"]["style"]=sections["main"]["style"];
        if(sections.count("audio"))converted["audio"]=sections["audio"];
        for(const auto& item:sections){
            if(item.first!="main"&&item.first!="favorites"&&item.first.rfind("favorites.presets.",0)!=0&&item.first.rfind("main.presets.",0)!=0&&item.first.rfind("countries.",0)!=0&&item.first.rfind("classes.",0)!=0)continue;
            const auto split=item.first.find(".presets.");
            const auto root=canonical(item.first.substr(0,split));
            const auto control=root+(split==std::string::npos?"":"|"+item.first.substr(split+9));
            converted.try_emplace(root);
            for(const auto& value:item.second){
                if(value.first=="style"&&item.first=="main")continue;
                if(value.first=="floorreflections"||value.first=="reflections"||value.first=="reflectionquality"||value.first=="shadows")
                    presentationControls[control][value.first=="floorreflections"?"reflections":value.first]=value.second;
                else if(split==std::string::npos)converted[root][value.first]=value.second;
                else return false;
            }
        }
        sections=std::move(converted);
    }
    if(!sections.count("showroom"))return false;
    const auto integer=[](const std::string& value,int& out){
        try{size_t used=0;const auto number=std::stoll(value,&used,0);if(used!=value.size()||number<INT_MIN||number>INT_MAX)return false;out=int(number);return true;}catch(...){return false;}
    };
    const auto boolean=[](const std::string& value,bool& out){
        const auto v=settingFold(value);
        if(v=="on"||v=="true"||v=="1"){out=true;return true;}
        if(v=="off"||v=="false"||v=="0"){out=false;return true;}
        return false;
    };
    StudioPreferences loaded;
    const auto number=[&](const char* section,const char* key,int& target){
        const auto group=sections.find(section);if(group==sections.end())return true;
        const auto found=group->second.find(key);return found==group->second.end()||integer(found->second,target);
    };
    const auto flag=[&](const char* key,bool& target){
        const auto found=sections["showroom"].find(key);return found==sections["showroom"].end()||boolean(found->second,target);
    };
    if(sections["showroom"].count("vehiclereflections")){
        auto& value=sections["showroom"]["vehiclereflections"];
        const auto name=settingFold(value);
        for(int i=0;i<5;++i)if(name==std::array<const char*,5>{"off","low","medium","high","ultra"}[size_t(i)])value=std::to_string(i);
    }
    int version=1;if(!number("showroom","formatversion",version)||version!=1)return false;
    if(sections["showroom"].count("language")){
        auto& value=sections["showroom"]["language"];const auto name=settingFold(value);
        for(size_t i=0;i<presentationLanguageNames.size();++i)if(name==settingFold(presentationLanguageNames[i]))value=std::to_string(i);
    }
    const auto& global=sections["showroom"];
    if(global.count("logos")){
        int logos=0;if(!integer(global.at("logos"),logos)||logos<0||logos>3)return false;
        sections["showroom"]["brandlogo"]=(logos&1)?"1":"0";
        sections["showroom"]["carlogo"]=(logos&2)?"1":"0";
    }
    const auto style=global.find("style");
    if(style!=global.end()){
        const auto value=settingFold(style->second);
        if(value=="default")loaded.showroomStyle=0;
        else if(value=="prologue")loaded.showroomStyle=1;else return false;
    }
    const auto priority=global.find("lastfilter");
    if(priority!=global.end()){
        const auto value=settingFold(priority->second);
        if(value!="country"&&value!="class")return false;
        loaded.countryPresentationPriority=value=="country";
    }
    if(!number("showroom","sortmode",loaded.sortMode)||!number("showroom","countryfilter",loaded.countryFilter)||
       !number("showroom","classfilter",loaded.classFilter)||!number("showroom","unitformat",loaded.unitFormat)||
       !number("showroom","antialiasing",loaded.antialiasing)||!number("showroom","ambientocclusion",loaded.ambientOcclusion)||
       !number("showroom","vehiclereflections",loaded.vehicleReflections)||!number("showroom","language",loaded.language)||
       !flag("carlogo",loaded.carLogo)||!flag("brandlogo",loaded.brandLogo)||!flag("displaymodelid",loaded.displayModelId)||
       !flag("displaymodelname",loaded.displayModelName)||!number("audio","master",loaded.audio.value[0])||
       !number("audio","music",loaded.audio.value[1])||!number("audio","effects",loaded.audio.value[2]))return false;
    for(const auto item:std::array<std::pair<const char*,uint32_t*>,2>{{{"countrymask",&loaded.countryMask},{"classmask",&loaded.classMask}}}){
        const auto found=global.find(item.first);if(found==global.end())continue;
        try{size_t used=0;const auto value=std::stoull(found->second,&used,0);if(used!=found->second.size()||value>UINT32_MAX)return false;*item.second=uint32_t(value);}catch(...){return false;}
    }
    const auto mask=[](const std::string& list,const auto& names,uint32_t& result){
        std::istringstream parts(list);std::string token;result=0;
        while(std::getline(parts,token,'+')){
            if(token=="motorbikes"||token=="motorcycles")token="motorbike";
            if(token=="mopeds")token="moped";
            if(token=="bicycles"||token=="bikes")token="bicycle";
            bool found=false;for(size_t i=0;i<std::size(names);++i)if(token==settingFold(names[i])){
                if(result&(1u<<i))return false;result|=1u<<i;found=true;break;
            }
            if(!found)return false;
        }
        return result!=0;
    };
    std::map<std::string,uint64_t> sectionKeys;
    for(const auto& item:sections){
        if(item.first.rfind("preset.",0)!=0)continue;
        uint64_t key=0;const auto suffix=item.first.substr(7);
        if(suffix=="favorites")key=favoritesPresentationContext;
        else if(suffix!="main"){
            uint32_t countries=0,classes=0;auto names=suffix;
            if(names.rfind("country.",0)==0){
                const auto split=names.find(".class.");const auto country=names.substr(8,split==std::string::npos?std::string::npos:split-8);
                if(!mask(country,countryKeys,countries))return false;
                if(split!=std::string::npos&&!mask(names.substr(split+7),presentationClassKeys,classes))return false;
            }else if(names.rfind("class.",0)==0){if(!mask(names.substr(6),presentationClassKeys,classes))return false;}
            else return false;
            key=uint64_t(countries)|(uint64_t(classes)<<32);
        }
        sectionKeys[item.first]=key;
        PresentationContext context;bool configured=false;
        if(item.second.count("configured")&&!boolean(item.second.at("configured"),configured))return false;
        for(const auto& value:item.second){
            if(value.first=="configured"){if(!boolean(value.second,configured))return false;continue;}
            if(value.first=="lastbackground") {
                if (!context.backgroundOverridden) {
                    bool found=false; for(int i=0;i<studioBackgroundCount;++i) if(settingFold(value.second)==settingFold(studioBackgroundNames[size_t(i)])) {
                        context.backgroundSelection=i;found=true;break;
                    }
                    if(!found)return false;
                }
                continue;
            }
            if(value.first=="background"){
                const auto name=settingFold(value.second);if(name=="recommended")continue;
                bool found=false;for(int i=0;i<studioBackgroundCount;++i)if(name==settingFold(studioBackgroundNames[size_t(i)])||(i==WarmBackground&&name=="warm")){
                    context.backgroundSelection=i;
                    context.backgroundOverridden=configured||!predefinedPresentationContext(key)||i!=(key==favoritesPresentationContext?IndustrialGarageBackground:studioBackgroundForFilters(uint32_t(key),-1,uint32_t(key>>32),-1));
                    found=true;break;
                }
                if(!found)return false;continue;
            }
            const bool floor=value.first.rfind("floorreflections.",0)==0,shadow=value.first.rfind("shadows.",0)==0;
            if(!floor&&!shadow)return false;
            const auto name=value.first.substr(floor?17:8);size_t slot=reflectionPresetCount;
            for(size_t i=0;i<reflectionPresetCount;++i)if(name==settingFold(presentationSlot(i))){slot=i;break;}
            if(slot==reflectionPresetCount)return false;
            if(shadow){if(!boolean(value.second,context.presets[slot].shadows))return false;}
            else{
                const auto quality=settingFold(value.second);int selected=-1;
                if(quality=="recommended")selected=inheritedFloorQuality;
                else if(quality=="on")selected=3;
                else for(int i=0;i<5;++i)if(quality==std::array<const char*,5>{"off","low","medium","high","ultra"}[size_t(i)])selected=i;
                if(selected<0)return false;context.presets[slot].floorQuality=uint8_t(selected);
                if(selected>0&&selected<=4)context.presets[slot].reflectionQuality=uint8_t(selected);
            }
        }
        if(!key){loaded.backgroundSelection=context.backgroundSelection;loaded.backgroundOverridden=context.backgroundOverridden;loaded.presets=context.presets;}
        else if(configured||context.backgroundOverridden||!predefinedPresentationContext(key)||!samePresentationPresets(context.presets,{})){
            if(!loaded.contexts.emplace(key,context).second)return false;
        }
    }
    for(const auto& item:presentationControls){
        const auto split=item.first.find('|');const auto root=item.first.substr(0,split);
        const auto found=sectionKeys.find(root);if(found==sectionKeys.end())return false;
        const auto key=found->second;
        const auto bg=presentationBackground(loaded,key);
        size_t slot=reflectionPreset(key?0:loaded.showroomStyle,bg);
        if(split!=std::string::npos){
            const auto name=item.first.substr(split+1);slot=reflectionPresetCount;
            for(size_t i=0;i<reflectionPresetCount;++i)if(settingFold(presentationSlot(i))==name){slot=i;break;}
            if(slot==reflectionPresetCount)return false;
        }
        auto& preset=key?loaded.contexts[key].presets[slot]:loaded.presets[slot];
        const auto& controls=item.second;
        if(controls.count("reflectionquality")){
            const auto name=settingFold(controls.at("reflectionquality"));int quality=-1;
            for(int i=1;i<=4;++i)if(name==std::array<const char*,5>{"off","low","medium","high","ultra"}[size_t(i)])quality=i;
            if(quality<0)return false;preset.reflectionQuality=uint8_t(quality);
        }
        if(controls.count("reflections")){
            const auto name=settingFold(controls.at("reflections"));
            if(name=="recommended")preset.floorQuality=inheritedFloorQuality;
            else {bool enabled=false;if(!boolean(name,enabled))return false;preset.floorQuality=enabled?preset.reflectionQuality:0;}
        }
        if(controls.count("shadows")&&!boolean(controls.at("shadows"),preset.shadows))return false;
    }
    for(auto item=loaded.contexts.begin();item!=loaded.contexts.end();){
        bool configured=false;
        for(const auto& section:sectionKeys)if(section.second==item->first&&sections[section.first].count("configured"))
            if(!boolean(sections[section.first].at("configured"),configured))return false;
        if(predefinedPresentationContext(item->first)&&!configured&&!item->second.backgroundOverridden&&samePresentationPresets(item->second.presets,{}))
            item=loaded.contexts.erase(item);
        else ++item;
    }
    if(!loaded.valid())return false;
    result=std::move(loaded);return true;
}

inline bool migrateStudioPreferences(const std::filesystem::path& ini,const std::filesystem::path& legacy,StudioPreferences& result)
{
    std::error_code error;
    if(std::filesystem::exists(ini,error))return loadStudioPreferences(ini,result);
    if (!std::filesystem::exists(legacy,error)) {
        return saveStudioPreferences(ini,result) && loadStudioPreferences(ini,result);
    }
    StudioPreferences saved;
    if(!loadBinaryStudioPreferences(legacy,saved)||!saveStudioPreferences(ini,saved))return false;
    StudioPreferences verified;
    if(!loadStudioPreferences(ini,verified)||verified!=saved)return false;
    auto backup=legacy;backup+=L".migrated";
    if(!std::filesystem::exists(backup,error))MoveFileW(legacy.c_str(),backup.c_str());
    result=std::move(verified);return true;
}
}

