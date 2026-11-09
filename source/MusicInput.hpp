#pragma once
#include "AssetPaths.hpp"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <memory>
#include <new>
#include <vector>
#include <array>
#include <fstream>

namespace gt4
{
    template <class T> struct ReleaseMedia
    {
        void operator()(T* value) const
        {
            value->Release();
        }
    };

    template <class Clip>
    bool readMp3(const std::filesystem::path& file, size_t limit, Clip& clip)
    {
        struct Libraries
        {
            HMODULE platform = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            HMODULE readers = LoadLibraryExW(L"mfreadwrite.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

            ~Libraries()
            {
                if (readers) {
                    FreeLibrary(readers);
                }
                if (platform) {
                    FreeLibrary(platform);
                }
            }
        } libraries;
        if (!libraries.platform || !libraries.readers) {
            return false;
        }

        const auto start = reinterpret_cast<decltype(&MFStartup)>(GetProcAddress(libraries.platform, "MFStartup"));
        const auto stop = reinterpret_cast<decltype(&MFShutdown)>(GetProcAddress(libraries.platform, "MFShutdown"));
        const auto createType =
            reinterpret_cast<decltype(&MFCreateMediaType)>(GetProcAddress(libraries.platform, "MFCreateMediaType"));
        const auto toWave = reinterpret_cast<decltype(&MFCreateWaveFormatExFromMFMediaType)>(
            GetProcAddress(libraries.platform, "MFCreateWaveFormatExFromMFMediaType"));
        using CreateByteStream = HRESULT(WINAPI*)(IStream*, IMFByteStream**);
        const auto createByteStream =
            reinterpret_cast<CreateByteStream>(GetProcAddress(libraries.platform, "MFCreateMFByteStreamOnStream"));
        const auto createReader = reinterpret_cast<decltype(&MFCreateSourceReaderFromByteStream)>(
            GetProcAddress(libraries.readers, "MFCreateSourceReaderFromByteStream"));
        if (!start || !stop || !createType || !toWave || !createByteStream || !createReader) {
            return false;
        }

        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
            return false;
        }
        struct Runtime
        {
            bool uninitialize;
            bool started;
            decltype(&MFShutdown) stop;

            ~Runtime()
            {
                if (started) {
                    stop();
                }
                if (uninitialize) {
                    CoUninitialize();
                }
            }
        } runtime{SUCCEEDED(com), SUCCEEDED(start(MF_VERSION, MFSTARTUP_FULL)), stop};
        if (!runtime.started) {
            return false;
        }

        std::ifstream input(assetPath(file), std::ios::binary | std::ios::ate);
        const auto fileSize = input ? input.tellg() : std::streampos(-1);
        if (fileSize <= 0 || uint64_t(fileSize) > std::min(limit, size_t(64 * 1024 * 1024))) {
            return false;
        }
        IStream* streamRaw = nullptr;
        if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &streamRaw))) {
            return false;
        }
        const std::unique_ptr<IStream, ReleaseMedia<IStream>> source(streamRaw);
        ULARGE_INTEGER length{};
        length.QuadPart = uint64_t(fileSize);
        if (FAILED(source->SetSize(length))) {
            return false;
        }
        input.seekg(0);
        std::array<char, 64 * 1024> block{};
        size_t remaining = size_t(fileSize);
        while (remaining) {
            const ULONG size = ULONG(std::min(remaining, block.size()));
            ULONG written = 0;
            if (!input.read(block.data(), size) || FAILED(source->Write(block.data(), size, &written)) ||
                written != size) {
                return false;
            }
            remaining -= size;
        }
        if (FAILED(source->Seek(LARGE_INTEGER{}, STREAM_SEEK_SET, nullptr))) {
            return false;
        }
        IMFByteStream* byteStreamRaw = nullptr;
        if (FAILED(createByteStream(source.get(), &byteStreamRaw))) {
            return false;
        }
        const std::unique_ptr<IMFByteStream, ReleaseMedia<IMFByteStream>> byteStream(byteStreamRaw);
        IMFSourceReader* readerRaw = nullptr;
        if (FAILED(createReader(byteStream.get(), nullptr, &readerRaw))) {
            return false;
        }
        const std::unique_ptr<IMFSourceReader, ReleaseMedia<IMFSourceReader>> reader(readerRaw);
        IMFMediaType* typeRaw = nullptr;
        if (FAILED(createType(&typeRaw))) {
            return false;
        }
        const std::unique_ptr<IMFMediaType, ReleaseMedia<IMFMediaType>> type(typeRaw);
        constexpr DWORD stream = MF_SOURCE_READER_FIRST_AUDIO_STREAM;
        if (FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
            FAILED(type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
            FAILED(reader->SetCurrentMediaType(stream, nullptr, type.get()))) {
            return false;
        }
        IMFMediaType* actualRaw = nullptr;
        if (FAILED(reader->GetCurrentMediaType(stream, &actualRaw))) {
            return false;
        }
        const std::unique_ptr<IMFMediaType, ReleaseMedia<IMFMediaType>> actual(actualRaw);
        WAVEFORMATEX* waveRaw = nullptr;
        UINT32 waveSize = 0;
        if (FAILED(toWave(actual.get(), &waveRaw, &waveSize, MFWaveFormatExConvertFlag_Normal)) ||
            !waveRaw || waveSize < sizeof(WAVEFORMATEX)) {
            CoTaskMemFree(waveRaw);
            return false;
        }
        const WAVEFORMATEX format = *waveRaw;
        CoTaskMemFree(waveRaw);
        if (format.wFormatTag != WAVE_FORMAT_PCM || format.wBitsPerSample != 16 ||
            (format.nChannels != 1 && format.nChannels != 2) || format.nSamplesPerSec < 8000 ||
            format.nSamplesPerSec > 96000 || format.nBlockAlign != format.nChannels * 2) {
            return false;
        }
        std::vector<char> bytes;
        unsigned emptyReads = 0;
        for (;;) {
            DWORD flags = 0;
            IMFSample* sampleRaw = nullptr;
            const HRESULT result = reader->ReadSample(stream, 0, nullptr, &flags, nullptr, &sampleRaw);
            const std::unique_ptr<IMFSample, ReleaseMedia<IMFSample>> sample(sampleRaw);
            if (FAILED(result) || (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))) {
                return false;
            }
            if (sample) {
                IMFMediaBuffer* bufferRaw = nullptr;
                if (FAILED(sample->ConvertToContiguousBuffer(&bufferRaw))) {
                    return false;
                }
                const std::unique_ptr<IMFMediaBuffer, ReleaseMedia<IMFMediaBuffer>> buffer(bufferRaw);
                BYTE* data = nullptr;
                DWORD size = 0;
                if (FAILED(buffer->Lock(&data, nullptr, &size))) {
                    return false;
                }
                bool stored = size <= limit - bytes.size();
                if (stored && size) {
                    try {
                        bytes.insert(bytes.end(), data, data + size);
                    } catch (const std::bad_alloc&) {
                        stored = false;
                    }
                }
                buffer->Unlock();
                if (!stored) {
                    return false;
                }
                if (size) {
                    emptyReads = 0;
                }
            }
            if (++emptyReads > 256) {
                return false;
            }
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
                break;
            }
        }
        if (bytes.empty() || bytes.size() % format.nBlockAlign) {
            return false;
        }
        clip.format = format;
        clip.bytes = std::move(bytes);
        return true;
    }
}
