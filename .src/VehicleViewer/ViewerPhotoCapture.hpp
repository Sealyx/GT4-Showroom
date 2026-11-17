#pragma once
#include <windows.h>
#include <d3d9.h>
#include <wincodec.h>
#include <filesystem>
#include <vector>
#include <memory>
#include <cstring>
#include <cwchar>
#include <cstdint>

namespace gt4
{
    struct PhotoRelease
    {
        template <class T> void operator()(T* value) const
        {
            if (value) {
                value->Release();
            }
        }
    };

    // encode before creating the file so a failed codec cannot leave an empty photo behind
    inline bool writeViewerPhoto(const std::vector<uint8_t>& pixels,
                                 UINT width,
                                 UINT height,
                                 const std::filesystem::path& folder,
                                 std::filesystem::path& saved)
    {
        saved.clear();
        if (!width || !height || uint64_t(width) * height > 64000000 || pixels.size() != uint64_t(width) * height * 4) {
            return false;
        }
        const UINT stride = width * 4;

        struct Apartment
        {
            HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

            ~Apartment()
            {
                if (SUCCEEDED(result)) {
                    CoUninitialize();
                }
            }
        } apartment;

        if (FAILED(apartment.result) && apartment.result != RPC_E_CHANGED_MODE) {
            return false;
        }
        IWICImagingFactory* factoryRaw = nullptr;
        if (FAILED(
                CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factoryRaw)))) {
            return false;
        }
        std::unique_ptr<IWICImagingFactory, PhotoRelease> factory(factoryRaw);
        IStream* streamRaw = nullptr;
        if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &streamRaw))) {
            return false;
        }
        std::unique_ptr<IStream, PhotoRelease> stream(streamRaw);
        IWICBitmapEncoder* encoderRaw = nullptr;
        if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoderRaw))) {
            return false;
        }
        std::unique_ptr<IWICBitmapEncoder, PhotoRelease> encoder(encoderRaw);
        if (FAILED(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache))) {
            return false;
        }
        IWICBitmapFrameEncode* frameRaw = nullptr;
        if (FAILED(encoder->CreateNewFrame(&frameRaw, nullptr))) {
            return false;
        }
        std::unique_ptr<IWICBitmapFrameEncode, PhotoRelease> frame(frameRaw);
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(frame->Initialize(nullptr)) || FAILED(frame->SetSize(width, height)) ||
            FAILED(frame->SetPixelFormat(&format)) || !IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA) ||
            FAILED(frame->WritePixels(height, stride, UINT(pixels.size()), const_cast<uint8_t*>(pixels.data()))) ||
            FAILED(frame->Commit()) || FAILED(encoder->Commit())) {
            return false;
        }
        STATSTG stats{};
        HGLOBAL memory = nullptr;
        if (FAILED(stream->Stat(&stats, STATFLAG_NONAME)) || stats.cbSize.HighPart || !stats.cbSize.LowPart ||
            FAILED(GetHGlobalFromStream(stream.get(), &memory))) {
            return false;
        }
        std::error_code error;
        std::filesystem::create_directories(folder, error);
        if (error) {
            return false;
        }
        SYSTEMTIME date{};
        GetLocalTime(&date);
        wchar_t stem[100]{};
        std::swprintf(stem,
                      std::size(stem),
                      L"GT4Showroom-%04u-%02u-%02u-%02u-%02u-%02u-%03u",
                      date.wYear,
                      date.wMonth,
                      date.wDay,
                      date.wHour,
                      date.wMinute,
                      date.wSecond,
                      date.wMilliseconds);
        const auto* bytes = static_cast<const uint8_t*>(GlobalLock(memory));
        if (!bytes) {
            return false;
        }

        struct Unlock
        {
            HGLOBAL memory;

            ~Unlock()
            {
                GlobalUnlock(memory);
            }
        } unlock{memory};

        // exclusive creation also protects photos taken within the same millisecond
        for (unsigned suffix = 0; suffix < 1000; ++suffix) {
            const auto file = folder / (std::wstring(stem) + (suffix ? L"-" + std::to_wstring(suffix) : L"") + L".png");
            HANDLE handle =
                CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == INVALID_HANDLE_VALUE) {
                if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) {
                    continue;
                }
                break;
            }
            DWORD written = 0;
            const bool complete =
                WriteFile(handle, bytes, stats.cbSize.LowPart, &written, nullptr) && written == stats.cbSize.LowPart;
            const bool closed = CloseHandle(handle) != FALSE;
            if (complete && closed) {
                saved = file;
            } else {
                DeleteFileW(file.c_str());
            }
            break;
        }
        return !saved.empty();
    }

    // save our private studio target, never the game's backbuffer or another menu's overlay
    inline bool saveViewerPhoto(IDirect3DDevice9* device,
                                IDirect3DTexture9* texture,
                                const ViewerPhoto& photo,
                                const std::filesystem::path& folder,
                                std::filesystem::path& saved)
    {
        saved.clear();
        if (!device || !texture) {
            return false;
        }
        D3DSURFACE_DESC desc{};
        if (FAILED(texture->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height ||
            (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8) ||
            uint64_t(desc.Width) * desc.Height > 64000000) {
            return false;
        }
        IDirect3DSurface9* sourceRaw = nullptr;
        if (FAILED(texture->GetSurfaceLevel(0, &sourceRaw))) {
            return false;
        }
        std::unique_ptr<IDirect3DSurface9, PhotoRelease> source(sourceRaw);
        IDirect3DSurface9* readRaw = nullptr;
        if (FAILED(device->CreateOffscreenPlainSurface(
                desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &readRaw, nullptr))) {
            return false;
        }
        std::unique_ptr<IDirect3DSurface9, PhotoRelease> readback(readRaw);
        if (FAILED(device->GetRenderTargetData(source.get(), readback.get()))) {
            return false;
        }
        const auto crop = photo.crop(desc.Width, desc.Height);
        const UINT stride = crop.width * 4;
        std::vector<uint8_t> pixels(size_t(stride) * crop.height);
        D3DLOCKED_RECT locked{};
        if (FAILED(readback->LockRect(&locked, nullptr, D3DLOCK_READONLY))) {
            return false;
        }
        const bool valid = locked.pBits && locked.Pitch >= int(desc.Width * 4);
        if (valid) {
            for (UINT y = 0; y < crop.height; ++y) {
                const auto* row =
                    static_cast<const uint8_t*>(locked.pBits) + size_t(y + crop.y) * locked.Pitch + crop.x * 4;
                auto* destination = pixels.data() + size_t(y) * stride;
                std::memcpy(destination, row, stride);
                for (UINT x = 0; x < crop.width; ++x) {
                    destination[x * 4 + 3] = 255;
                }
            }
        }
        const HRESULT unlocked = readback->UnlockRect();
        if (!valid || FAILED(unlocked)) {
            return false;
        }

        return writeViewerPhoto(pixels, crop.width, crop.height, folder, saved);
    }
}
