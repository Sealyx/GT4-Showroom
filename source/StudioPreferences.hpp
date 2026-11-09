#pragma once
#include "AudioLevels.hpp"
#include "Countries.hpp"

namespace gt4
{
    struct StudioPreferences
    {
        int sortMode = 5, countryFilter = -1, classFilter = -1;
        bool carLogo = false, brandLogo = false, displayModelId = false, displayModelName = false;
        // legacy bytes keep existing settings readable; playback uses gta's sliders.
        AudioLevels audio;

        bool valid() const
        {
            return sortMode >= 0 && sortMode <= 5 && countryFilter >= -1 && countryFilter < int(countryCount) &&
                   classFilter >= -1 && classFilter <= 12 &&
                   std::all_of(
                       audio.value.begin(),
                       audio.value.end(),
                       [](int value) {
                           return value >= 0 && value <= 100;
                       });
        }

        bool operator==(const StudioPreferences& other) const
        {
            return sortMode == other.sortMode && countryFilter == other.countryFilter &&
                   classFilter == other.classFilter && carLogo == other.carLogo && brandLogo == other.brandLogo &&
                   displayModelId == other.displayModelId && displayModelName == other.displayModelName &&
                   audio == other.audio;
        }

        bool operator!=(const StudioPreferences& other) const
        {
            return !(*this == other);
        }
    };

    inline bool loadStudioPreferences(const std::filesystem::path& path, StudioPreferences& result)
    {
        std::ifstream input(path, std::ios::binary);
        std::array<uint8_t, 20> bytes{};
        if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) ||
            input.peek() != std::char_traits<char>::eof()) {
            return false;
        }
        const std::array<uint8_t, 5> magic{{'G', 'T', '4', 'P', 1}};
        if (!std::equal(magic.begin(), magic.end(), bytes.begin()) || (bytes[8] & 0xF0) != 0 ||
            std::any_of(bytes.begin() + 12, bytes.begin() + 19, [](uint8_t value) {
                return value != 0;
            })) {
            return false;
        }
        uint8_t check = 0xA7;
        for (size_t i = 0; i < bytes.size() - 1; ++i) {
            check ^= bytes[i];
        }
        if (check != bytes.back()) {
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
        if (!loaded.valid()) {
            return false;
        }
        result = loaded;
        return true;
    }

    inline bool saveStudioPreferences(const std::filesystem::path& path, const StudioPreferences& preferences)
    {
        if (!preferences.valid()) {
            return false;
        }
        std::array<uint8_t, 20> bytes{{'G', 'T', '4', 'P', 1}};
        bytes[5] = uint8_t(preferences.sortMode);
        bytes[6] = uint8_t(preferences.countryFilter + 1);
        bytes[7] = uint8_t(preferences.classFilter + 1);
        bytes[8] = uint8_t(preferences.carLogo | (preferences.brandLogo << 1) | (preferences.displayModelId << 2) |
                           (preferences.displayModelName << 3));
        for (size_t i = 0; i < preferences.audio.value.size(); ++i) {
            bytes[9 + i] = uint8_t(preferences.audio.value[i]);
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
