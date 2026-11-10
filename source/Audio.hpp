#pragma once
#include "AssetPaths.hpp"
#include "AudioLevels.hpp"
#include "MusicInput.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "Showroom.hpp"
#include <mmsystem.h>
#include <dsound.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <new>
#include <string_view>

namespace gt4
{
    enum class PcmReadError { None, Open, FileSize, Riff, Bounds, Format, Data, Read, Unsupported, Memory };

    inline const char* pcmReadErrorName(PcmReadError error)
    {
        static const char* names[] = {"none",         "open",       "file-size", "riff-header",        "chunk-bounds",
                                      "format-chunk", "data-chunk", "read",      "unsupported-format", "memory"};
        return names[size_t(error)];
    }

    struct AudioFault
    {
        const char* stage = "";
        std::wstring file;
        HRESULT code = S_OK;
        PcmReadError decoder = PcmReadError::None;
    };

    struct PcmClip
    {
        WAVEFORMATEX format{};
        std::vector<char> bytes;
    };

    inline bool readPcm(const std::filesystem::path& name, size_t limit, PcmClip& clip, PcmReadError* error = nullptr)
    {
        clip = {};
        if (error) {
            *error = PcmReadError::None;
        }
        auto fail = [&](PcmReadError reason) {
            if (error) {
                *error = reason;
            }
            return false;
        };
        std::ifstream f(assetPath(name), std::ios::binary | std::ios::ate);
        if (!f) {
            return fail(PcmReadError::Open);
        }
        const auto length = f.tellg();
        if (length < 12 || uint64_t(length) > limit + 65536) {
            return fail(PcmReadError::FileSize);
        }
        f.seekg(0);
        char id[4]{};
        uint32_t size = 0;
        f.read(id, 4);
        if (std::memcmp(id, "RIFF", 4)) {
            return fail(PcmReadError::Riff);
        }
        f.read(reinterpret_cast<char*>(&size), 4);
        const uint64_t end = uint64_t(size) + 8;
        f.read(id, 4);
        if (!f || std::memcmp(id, "WAVE", 4) || end > uint64_t(length) || end < 12) {
            return fail(PcmReadError::Riff);
        }
        bool fmt = false, data = false;
        while (f && uint64_t(f.tellg()) + 8 <= end) {
            f.read(id, 4);
            f.read(reinterpret_cast<char*>(&size), 4);
            if (!f) {
                return fail(PcmReadError::Read);
            }
            const uint64_t offset = uint64_t(f.tellg()), next = offset + size + (size & 1);
            if (next > end) {
                return fail(PcmReadError::Bounds);
            }
            if (!std::memcmp(id, "fmt ", 4)) {
                if (fmt || size < 16 || size > 40) {
                    return fail(PcmReadError::Format);
                }
                f.read(reinterpret_cast<char*>(&clip.format), std::min(size_t(size), sizeof(WAVEFORMATEX)));
                fmt = true;
            } else if (!std::memcmp(id, "data", 4)) {
                if (data || !size || size > limit) {
                    return fail(PcmReadError::Data);
                }
                try {
                    clip.bytes.resize(size);
                } catch (const std::bad_alloc&) {
                    return fail(PcmReadError::Memory);
                }
                f.read(clip.bytes.data(), size);
                data = true;
            }
            if (!f) {
                return fail(PcmReadError::Read);
            }
            f.seekg(std::streamoff(next));
        }
        const auto& w = clip.format;
        const bool valid = fmt && data && w.wFormatTag == WAVE_FORMAT_PCM && (w.nChannels == 1 || w.nChannels == 2) &&
                           (w.wBitsPerSample == 8 || w.wBitsPerSample == 16) && w.nSamplesPerSec >= 8000 &&
                           w.nSamplesPerSec <= 96000 && w.nBlockAlign == w.nChannels * w.wBitsPerSample / 8 &&
                           w.nAvgBytesPerSec == w.nSamplesPerSec * w.nBlockAlign &&
                           clip.bytes.size() % w.nBlockAlign == 0;
        return valid || fail(PcmReadError::Unsupported);
    }

    class Audio
    {
        static constexpr std::array<const wchar_t*, 9> effectFiles{
            {L"move.wav", L"accept.wav", L"back.wav", L"open.wav", L"spawn.wav", L"error.wav", L"slider.wav",
             L"wash-spin.wav", L"wash-complete.wav"}};

        struct Voice
        {
            IDirectSoundBuffer* buffer = nullptr;
            uint64_t serial = 0;
        };

        IDirectSound8* ds = nullptr;
        IDirectSoundBuffer* bed = nullptr;
        IDirectSoundBuffer* shopBed = nullptr;
        bool shopMusic = false, shopBedTried = false;
        IDirectSoundBuffer* prize = nullptr;
        bool prizeActive = false, prizePlaying = false, prizeResume = false;
        float prizeGain = 0;
        IDirectSoundBuffer* failure = nullptr;
        bool failureActive = false, failurePlaying = false, failureResume = false;
        float failureGain = 0;
        std::array<IDirectSoundBuffer*, effectFiles.size()> clips{};
        std::array<Voice, 16> voices{};
        uint64_t serial = 0, previous = 0;
        bool musicStarted = false, bedPlaying = false;
        float musicGain = 0;
        std::array<AudioFault, 16> faults{};
        size_t faultsStored = 0, faultsReported = 0;

        void fault(const char* stage, std::wstring_view file, HRESULT code, PcmReadError decoder = PcmReadError::None)
        {
            for (size_t i = 0; i < faultsStored; ++i) {
                const auto& previous = faults[i];
                if (previous.code == code && previous.decoder == decoder && std::strcmp(previous.stage, stage) == 0 &&
                    std::wstring_view(previous.file) == file) {
                    return;
                }
            }
            if (faultsStored < faults.size()) {
                auto& next = faults[faultsStored++];
                next.stage = stage;
                next.code = code;
                next.decoder = decoder;
                try {
                    next.file.assign(file.data(), file.size());
                } catch (const std::bad_alloc&) {
                    next.file.clear();
                }
            }
        }

        void releaseVoices()
        {
            for (auto& v : voices) {
                if (v.buffer) {
                    v.buffer->Stop();
                    v.buffer->Release();
                    v = {};
                }
            }
        }

        IDirectSoundBuffer* load(const std::wstring& name, size_t limit, bool music = false)
        {
            PcmClip c;
            PcmReadError decoder = PcmReadError::None;
            const bool mp3 = std::filesystem::path(name).extension() == L".mp3";
            const bool decoded = mp3 ? readMp3(name, limit, c) : readPcm(name, limit, c, &decoder);
            if (!decoded) {
                HRESULT code = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                if (decoder == PcmReadError::Memory) {
                    code = E_OUTOFMEMORY;
                }
                if (decoder == PcmReadError::Open) {
                    const auto path = assetPath(name);
                    code = GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES
                               ? HRESULT_FROM_WIN32(GetLastError())
                               : E_FAIL;
                }
                fault(mp3 ? "mp3-decode" : "wave-load", name, code, decoder);
                return nullptr;
            }
            DSBUFFERDESC desc{};
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;
            if (music) {
                desc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
            }
            desc.dwBufferBytes = DWORD(c.bytes.size());
            desc.lpwfxFormat = &c.format;
            IDirectSoundBuffer* b = nullptr;
            HRESULT result = ds->CreateSoundBuffer(&desc, &b, nullptr);
            if (FAILED(result)) {
                fault("buffer-create", name, result);
                return nullptr;
            }
            void *p1 = nullptr, *p2 = nullptr;
            DWORD n1 = 0, n2 = 0;
            result = b->Lock(0, desc.dwBufferBytes, &p1, &n1, &p2, &n2, 0);
            if (FAILED(result)) {
                fault("buffer-lock", name, result);
                b->Release();
                return nullptr;
            }
            std::memcpy(p1, c.bytes.data(), n1);
            if (n2) {
                std::memcpy(p2, c.bytes.data() + n1, n2);
            }
            result = b->Unlock(p1, n1, p2, n2);
            if (FAILED(result)) {
                fault("buffer-unlock", name, result);
                b->Release();
                return nullptr;
            }
            result = b->SetVolume(DSBVOLUME_MIN);
            if (FAILED(result)) {
                fault("buffer-volume", name, result);
                b->Release();
                return nullptr;
            }
            return b;
        }

        IDirectSoundBuffer* activeBed() const
        {
            return shopMusic ? shopBed : bed;
        }

        void playBed()
        {
            auto* active = activeBed();
            if (!active) {
                return;
            }
            if (!shopMusic && !musicStarted) {
                const HRESULT result = active->SetCurrentPosition(0);
                if (FAILED(result)) {
                    fault("music-position", L"music.wav", result);
                    return;
                }
                musicStarted = true;
            }
            active->SetVolume(DSBVOLUME_MIN);
            const HRESULT result = active->Play(0, 0, DSBPLAY_LOOPING);
            bedPlaying = SUCCEEDED(result);
            if (FAILED(result)) {
                fault("music-play", shopMusic ? L"transfender.mp3" : L"music.wav", result);
            }
            musicGain = 0;
        }

        float transitionGain = 1.f;
        unsigned gameEffectsVolume = 64, gameRadioVolume = 64;

        LONG effectVolume() const
        {
            return mixedVolume(masterVolume, cueVolume, gameEffectsVolume, transitionGain);
        }

        void updateEffectVolume()
        {
            const LONG level = effectVolume();
            for (auto& voice : voices) {
                if (voice.buffer) {
                    voice.buffer->SetVolume(level);
                }
            }
        }

      public:
        ~Audio()
        {
            close();
        }

        Audio() = default;
        Audio(const Audio&) = delete;
        Audio& operator=(const Audio&) = delete;
        bool enabled = true, running = false, focused = true;

		// game owns the volume now. saved showroom trims stay out of playback

        unsigned bedVolume = 100, cueVolume = 100, masterVolume = 100;

        const AudioFault* nextFault()
        {
            return faultsReported < faultsStored ? &faults[faultsReported++] : nullptr;
        }

        size_t diagnosticCount() const
        {
            return faultsStored;
        }

        void setTransitionGain(float gain)
        {
            gain = std::clamp(gain, 0.f, 1.f);
            if (gain != transitionGain) {
                transitionGain = gain;
                updateEffectVolume();
            }
        }

        void setGameLevels(unsigned effects, unsigned music)
        {
            const unsigned nextEffects = std::min(effects, 64u);
            gameRadioVolume = std::min(music, 64u);
            if (gameEffectsVolume != nextEffects) {
                gameEffectsVolume = nextEffects;
                updateEffectVolume();
            }
        }

        void setLevels(AudioLevels levels)
        {
            levels.normalize();
            if (masterVolume == unsigned(levels.value[0]) && bedVolume == unsigned(levels.value[1]) &&
                cueVolume == unsigned(levels.value[2])) {
                return;
            }
            masterVolume = unsigned(levels.value[0]);
            bedVolume = unsigned(levels.value[1]);
            cueVolume = unsigned(levels.value[2]);
            updateEffectVolume();
        }

        static LONG mixedVolume(unsigned master, unsigned bus, unsigned native, float gain)
        {
            return volume(float(std::min(master, 100u) * std::min(bus, 100u)) / 100.f *
                          (float(std::min(native, 64u)) / 64.f) * gain);
        }

        static LONG volume(float percent)
        {
            percent = std::clamp(percent, 0.f, 100.f);
            return percent > 0 ? LONG(std::max(-10000., 2000 * std::log10(double(percent) / 100))) : DSBVOLUME_MIN;
        }

        void init(HWND window, const std::wstring& folder, bool studio = true)
        {
            if (!enabled) {
                return;
            }
            if (!ds) {
                if (!window || !IsWindow(window)) {
                    fault("cooperative-window", folder, E_HANDLE);
                    return;
                }
                HRESULT result = DirectSoundCreate8(nullptr, &ds, nullptr);
                if (FAILED(result)) {
                    fault("device-create", folder, result);
                    return;
                }
                result = ds->SetCooperativeLevel(window, DSSCL_NORMAL);
                if (FAILED(result)) {
                    close();
                    fault("cooperative-level", folder, result);
                    return;
                }
            }
            if (studio) {
                if (!bed) {
                    constexpr size_t limit = 64 * 1024 * 1024;
                    const auto mp3 = folder + L"music.mp3";
                    if (GetFileAttributesW(assetPath(mp3).c_str()) != INVALID_FILE_ATTRIBUTES) {
                        bed = load(mp3, limit, true);
                    }
                    if (!bed) {
                        bed = load(folder + L"music.wav", limit, true);
                    }
                }
                if (!prize) {
                    prize = load(folder + L"prize.wav", 64 * 1024 * 1024);
                }
            }
            for (size_t i = 0; i < clips.size(); ++i) {
                if (!clips[i]) {
                    clips[i] = load(folder + effectFiles[i], 2 * 1024 * 1024);
                }
            }
            previous = GetTickCount64();
        }

        void initFailure(HWND window, const std::wstring& folder)
        {
            init(window, folder, false);
            if (ds && !failure) {
                failure = load(folder + L"failure.wav", 64 * 1024 * 1024);
            }
        }

        void startFailure()
        {
            if (!enabled || !failure) {
                return;
            }
            running = false;
            prizeActive = false;
            if (auto* active = activeBed()) {
                active->Stop();
            }
            bedPlaying = false;
            if (prize) {
                prize->Stop();
            }
            prizePlaying = false;
            releaseVoices();
            failureActive = true;
            failureResume = false;
            failureGain = 0;
            failure->Stop();
            failure->SetCurrentPosition(0);
            failure->SetVolume(DSBVOLUME_MIN);
            failurePlaying = false;
            if (focused) {
                const HRESULT result = failure->Play(0, 0, DSBPLAY_LOOPING);
                failurePlaying = SUCCEEDED(result);
                if (FAILED(result)) {
                    fault("failure-play", L"failure.wav", result);
                }
            }
        }

        void endFailure()
        {
            failureActive = failurePlaying = failureResume = false;
            failureGain = 0;
            if (failure) {
                failure->Stop();
            }
        }

        bool failureReady() const
        {
            return failure != nullptr;
        }

        void start()
        {
            if (!enabled) {
                return;
            }
            endFailure();
            running = true;
            focused = true;
            playBed();
        }

        void stopBed()
        {
            running = false;
            prizeActive = false;
        }

        void startPrize()
        {
            running = false;
            if (auto* active = activeBed()) {
                active->Stop();
            }
            bedPlaying = false;
            musicGain = 0;
            releaseVoices();
            prizeActive = true;
            prizeGain = 0;
            prizeResume = false;
            if (prize) {
                prize->Stop();
                prize->SetCurrentPosition(0);
                prize->SetVolume(DSBVOLUME_MIN);
                prizePlaying = false;
                if (enabled && focused) {
                    const HRESULT result = prize->Play(0, 0, 0);
                    prizePlaying = SUCCEEDED(result);
                    if (FAILED(result)) {
                        fault("prize-play", L"prize.wav", result);
                    }
                }
            }
        }

        void endPrize(bool resume)
        {
            prizeActive = false;
            prizeResume = false;
            if (prize) {
                prize->Stop();
            }
            prizePlaying = false;
            prizeGain = 0;
            if (resume) {
                running = true;
                if (focused && enabled) {
                    playBed();
                }
            }
        }

        bool prizeReady() const
        {
            return prize != nullptr;
        }

        void setShopMusic(bool on, const std::wstring& folder = {})
        {
            if (on == shopMusic) {
                return;
            }
            if (on && ds && !shopBedTried) {
                shopBedTried = true;
                shopBed = load(folder + L"transfender.mp3", 64 * 1024 * 1024);
            }
            if (auto* active = activeBed()) {
                active->Stop();
            }
            shopMusic = on;
            bedPlaying = false;
            musicGain = 0;
            if (running && focused && enabled) {
                playBed();
            }
        }

        void update()
        {
            const uint64_t tick = GetTickCount64();
            const float dt = previous ? std::min(float(tick - previous) / 1000.f, .1f) : 0;
            previous = tick;
            const float target = running && focused && enabled ? 1.f : 0.f;
            musicGain += std::clamp(target - musicGain, -dt / .15f, dt / .15f);
            if (auto* active = activeBed(); active && bedPlaying) {
                const HRESULT result = active->SetVolume(
                    mixedVolume(masterVolume, bedVolume, gameRadioVolume, musicGain * transitionGain));
                if (FAILED(result)) {
                    fault("music-volume", shopMusic ? L"transfender.mp3" : L"music.wav", result);
                }
                if (musicGain <= 0 && target == 0) {
                    active->Stop();
                    bedPlaying = false;
                }
            }
            const float prizeTarget = prizeActive && focused && enabled ? 1.f : 0.f;
            prizeGain += std::clamp(prizeTarget - prizeGain, -dt / .12f, dt / .22f);
            if (prize && prizePlaying) {
                prize->SetVolume(mixedVolume(masterVolume, bedVolume, gameRadioVolume, prizeGain));
                DWORD flags = 0;
                const HRESULT result = prize->GetStatus(&flags);
                if (FAILED(result)) {
                    fault("prize-status", L"prize.wav", result);
                    prizePlaying = false;
                } else if (!(flags & DSBSTATUS_PLAYING)) {
                    prizePlaying = false;
                } else if (prizeGain <= 0 && prizeTarget == 0) {
                    prize->Stop();
                    prizePlaying = false;
                }
            }
            const float failTarget = failureActive && focused && enabled ? 1.f : 0.f;
            failureGain += std::clamp(failTarget - failureGain, -dt / .15f, dt / .08f);
            if (failure && failurePlaying) {
                failure->SetVolume(mixedVolume(masterVolume, bedVolume, gameRadioVolume, failureGain));
            }
            for (auto& v : voices) {
                if (v.buffer) {
                    DWORD flags = 0;
                    if (FAILED(v.buffer->GetStatus(&flags)) || !(flags & DSBSTATUS_PLAYING)) {
                        v.buffer->Release();
                        v = {};
                    }
                }
            }
        }

        void focus(bool on)
        {
            if (focused == on) {
                return;
            }
            focused = on;
            if (!on) {
                if (auto* active = activeBed()) {
                    active->Stop();
                }
                bedPlaying = false;
                musicGain = 0;
                prizeResume = prizePlaying;
                if (prize) {
                    prize->Stop();
                }
                prizePlaying = false;
                prizeGain = 0;
                failureResume = failurePlaying;
                if (failure) {
                    failure->Stop();
                }
                failurePlaying = false;
                failureGain = 0;
                releaseVoices();
            } else if (failureActive && failureResume && enabled && failure) {
                const HRESULT result = failure->Play(0, 0, DSBPLAY_LOOPING);
                failurePlaying = SUCCEEDED(result);
                if (FAILED(result)) {
                    fault("failure-resume", L"failure.wav", result);
                }
                failureResume = false;
            } else if (prizeActive && prizeResume && enabled && prize) {
                const HRESULT result = prize->Play(0, 0, 0);
                prizePlaying = SUCCEEDED(result);
                if (FAILED(result)) {
                    fault("prize-resume", L"prize.wav", result);
                }
                prizeResume = false;
            } else if (running && enabled) {
                playBed();
            }
        }

        bool musicReady() const
        {
            return activeBed() != nullptr;
        }

        bool effectsReady() const
        {
            return std::all_of(clips.begin(), clips.end(), [](auto* b) {
                return b != nullptr;
            });
        }

        bool ready() const
        {
            return musicReady() && effectsReady();
        }

        size_t voiceCount() const
        {
            return size_t(std::count_if(voices.begin(), voices.end(), [](const Voice& v) {
                return v.buffer != nullptr;
            }));
        }

        bool voicesMuted() const
        {
            for (const auto& v : voices) {
                if (v.buffer) {
                    LONG level = 0;
                    if (FAILED(v.buffer->GetVolume(&level)) || level != DSBVOLUME_MIN) {
                        return false;
                    }
                }
            }
            return true;
        }

        bool cue(Cue cue)
        {
            if (!enabled || !focused || !ds || cue == Cue::None) {
                return false;
            }
            update();
            size_t index = 1;
            switch (cue) {
            case Cue::Move:
                index = 0;
                break;
            case Cue::Slider:
                index = 6;
                break;
            case Cue::Back:
                index = 2;
                break;
            case Cue::Open:
                index = 3;
                break;
            case Cue::Spawn:
                index = 4;
                break;
            case Cue::Error:
                index = 5;
                break;
            case Cue::WashSpin:
                index = 7;
                break;
            case Cue::WashComplete:
                index = 8;
                break;
            default:
                break;
            }
            if (!clips[index]) {
                return false;
            }
            IDirectSoundBuffer* b = nullptr;
            HRESULT result = ds->DuplicateSoundBuffer(clips[index], &b);
            if (FAILED(result)) {
                fault("effect-duplicate", effectFiles[index], result);
                return false;
            }

            const LONG level = effectVolume();
            b->SetVolume(level == DSBVOLUME_MIN ? DSBVOLUME_MIN + 1 : DSBVOLUME_MIN);
            result = b->SetVolume(level);
            if (FAILED(result)) {
                fault("effect-volume", effectFiles[index], result);
                b->Release();
                return false;
            }
            result = b->SetCurrentPosition(0);
            if (FAILED(result)) {
                fault("effect-position", effectFiles[index], result);
                b->Release();
                return false;
            }
            result = b->Play(0, 0, 0);
            if (FAILED(result)) {
                fault("effect-play", effectFiles[index], result);
                b->Release();
                return false;
            }
            auto it = std::find_if(voices.begin(), voices.end(), [](const Voice& v) {
                return v.buffer == nullptr;
            });
            if (it == voices.end()) {
                it = std::min_element(voices.begin(), voices.end(), [](const Voice& a, const Voice& b) {
                    return a.serial < b.serial;
                });
                it->buffer->Stop();
                it->buffer->Release();
            }
            *it = {b, ++serial};
            return true;
        }

        void stop()
        {
            endFailure();
            prizeActive = false;
            prizeResume = false;
            prizePlaying = false;
            prizeGain = 0;
            if (prize) {
                prize->Stop();
            }
            running = false;
            if (auto* active = activeBed()) {
                active->Stop();
            }
            bedPlaying = false;
            shopMusic = false;
            musicGain = 0;
            releaseVoices();
        }

        void close()
        {
            stop();
            if (failure) {
                failure->Release();
            }
            failure = nullptr;
            if (bed) {
                bed->Release();
            }
            bed = nullptr;
            if (shopBed) {
                shopBed->Release();
                shopBed = nullptr;
            }
            shopBedTried = false;
            if (prize) {
                prize->Release();
            }
            prize = nullptr;
            for (auto*& b : clips) {
                if (b) {
                    b->Release();
                }
                b = nullptr;
            }
            if (ds) {
                ds->Release();
            }
            ds = nullptr;
            musicStarted = false;
            previous = 0;
            faults = {};
            faultsStored = faultsReported = 0;
        }
    };
}
