#pragma once
#include "StudioBackgrounds.hpp"
#include "AudioLevels.hpp"
#include "Countries.hpp"

namespace gt4
{
    struct PresentationPreset
    {
        uint8_t floorQuality = inheritedFloorQuality;
        bool shadows = true;

        bool operator==(const PresentationPreset& other) const
        {
            return floorQuality == other.floorQuality && shadows == other.shadows;
        }
    };

    struct StudioPreferences
    {
        int sortMode = 5, countryFilter = -1, classFilter = -1;
        uint32_t countryMask = 0, classMask = 0;
        int backgroundSelection = 0, showroomStyle = 0;
        int unitFormat = 0, japanBackground = 0, bikeBackground = 0;
        int antialiasing = 2, ambientOcclusion = 2;
        int vehicleReflections = 3;
        bool backgroundOverridden = false;
        std::array<PresentationPreset, reflectionPresetCount> presets{};
        int language = 0;
        bool carLogo = false, brandLogo = false, displayModelId = true, displayModelName = true;
        // bytes keep existing settings readable. playback uses gta's sliders
        AudioLevels audio;

        bool valid() const
        {
            return showroomStyle >= 0 && showroomStyle < studioStyleCount && backgroundSelection >= 0 && backgroundSelection < studioBackgroundCount && sortMode >= 0 && sortMode <= 5 && countryFilter >= -1 && countryFilter < int(countryCount) &&
                   classFilter >= -1 && classFilter <= 12 && unitFormat >= 0 && unitFormat <= 2 &&
                   japanBackground >= 0 && japanBackground <= 1 && bikeBackground >= 0 && bikeBackground <= 2 &&
                   antialiasing >= 0 && antialiasing <= 3 && ambientOcclusion >= 0 && ambientOcclusion <= 3 &&
                   vehicleReflections >= 0 && vehicleReflections <= 4 &&
                   std::all_of(presets.begin(), presets.end(), [](const PresentationPreset& preset) {
                       return preset.floorQuality <= 4 || preset.floorQuality == inheritedFloorQuality;
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
            return showroomStyle == other.showroomStyle && backgroundSelection == other.backgroundSelection &&
                   backgroundOverridden == other.backgroundOverridden &&
                   presets == other.presets && sortMode == other.sortMode && countryFilter == other.countryFilter &&
                   classFilter == other.classFilter && carLogo == other.carLogo && brandLogo == other.brandLogo &&
                   displayModelId == other.displayModelId && displayModelName == other.displayModelName &&
                   audio == other.audio && countryMask == other.countryMask && classMask == other.classMask &&
                   unitFormat == other.unitFormat &&
                   japanBackground == other.japanBackground && bikeBackground == other.bikeBackground &&
                   antialiasing == other.antialiasing && ambientOcclusion == other.ambientOcclusion &&
                   vehicleReflections == other.vehicleReflections && language == other.language;
        }

        bool operator!=(const StudioPreferences& other) const
        {
            return !(*this == other);
        }
    };

    inline bool loadStudioPreferences(const std::filesystem::path& path, StudioPreferences& result)
    {
        std::ifstream input(path, std::ios::binary);
        std::array<uint8_t, 32 + 255> bytes{};
        input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        const size_t size = size_t(input.gcount());
        const std::array<uint8_t, 4> magic{{'G', 'T', '4', 'P'}};
        const uint8_t version = bytes[4];
        const bool legacy = size == 20 && version == 1;
        const bool old = size == 32 && version >= 2 && version <= 6;
        const bool perPreset = size >= 33 && version == 7;
        if ((!legacy && !old && !perPreset) || input.peek() != std::char_traits<char>::eof() ||
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
            loaded.japanBackground = bytes[20];
            loaded.bikeBackground = bytes[21];
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
            // Pre-v6 files contain the effective value. Infer a manual choice with the original defaults.
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
            if (manual) { preset.floorQuality = floor; }
            preset.shadows = bytes[19] != 0;
        }
        result = loaded;
        return true;
    }

    inline bool saveStudioPreferences(const std::filesystem::path& path, const StudioPreferences& preferences)
    {
        if (!preferences.valid()) {
            return false;
        }
        std::array<uint8_t, 32 + reflectionPresetCount> bytes{{'G', 'T', '4', 'P', 7}};
        bytes[5] = uint8_t(preferences.sortMode);
        bytes[6] = uint8_t(preferences.countryFilter + 1);
        bytes[7] = uint8_t(preferences.classFilter + 1);
        bytes[8] = uint8_t(preferences.carLogo | (preferences.brandLogo << 1) | (preferences.displayModelId << 2) |
                           (preferences.displayModelName << 3));
        for (size_t i = 0; i < preferences.audio.value.size(); ++i) {
            bytes[9 + i] = uint8_t(preferences.audio.value[i]);
        }
        for (unsigned i = 0; i < 4; ++i) {
            bytes[12 + i] = uint8_t(preferences.countryMask >> (8 * i));
        }
        bytes[16] = uint8_t(preferences.classMask);
        bytes[17] = uint8_t(preferences.classMask >> 8);
        bytes[18] = uint8_t(preferences.unitFormat);
        bytes[20] = uint8_t(preferences.japanBackground);
        bytes[21] = uint8_t(preferences.bikeBackground);
        bytes[22] = uint8_t(preferences.antialiasing);
        bytes[23] = uint8_t(preferences.ambientOcclusion);
        bytes[25] = uint8_t(preferences.vehicleReflections);
        bytes[26] = uint8_t(preferences.language);
        bytes[27] = uint8_t(preferences.backgroundSelection);
        bytes[28] = uint8_t(preferences.showroomStyle);
        bytes[30] = uint8_t(preferences.backgroundOverridden);
        for (size_t i = 0; i < preferences.presets.size(); ++i) {
            const auto& preset = preferences.presets[i];
            const uint8_t quality = preset.floorQuality == inheritedFloorQuality ? 0x0F : preset.floorQuality;
            bytes[31 + i] = uint8_t(quality | (preset.shadows ? 0 : 0x80));
        }
        bytes.back() = 0xA7;
        for (size_t i = 0; i < bytes.size() - 1; ++i) {
            bytes.back() ^= bytes[i];
        }
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return false;
        }
        auto temporary = path;
        temporary += L".tmp";
        bool written = false;
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            written = bool(output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()) && output.flush());
        }
        if (!written) {
            DeleteFileW(temporary.c_str());
            return false;
        }
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(temporary.c_str());
            return false;
        }
        return true;
    }
}
