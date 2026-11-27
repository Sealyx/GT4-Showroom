#pragma once
#include <filesystem>
#include <string>
namespace gt4 {
inline constexpr int maximumMusicTrack = 28;
inline std::filesystem::path musicTrackFolder;
inline bool musicTrackAvailable(int track) {
    std::error_code error;
    return track>=1 && track<=maximumMusicTrack && std::filesystem::is_regular_file(
        musicTrackFolder/(L"track-"+std::to_wstring(track)+L".wav"),error);
}
inline int validatedMusicTrack(int track) { return musicTrackAvailable(track)?track:1; }
}
