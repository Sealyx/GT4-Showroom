#pragma once
#include <d3d9.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <limits>
#include <vector>

namespace gt4
{
    inline unsigned previewTextureMaximumEdge = 0;

    struct PreviewTextureScope
    {
        unsigned previous = previewTextureMaximumEdge;

        explicit PreviewTextureScope(unsigned maximumEdge)
        {
            previewTextureMaximumEdge = maximumEdge;
        }

        PreviewTextureScope(const PreviewTextureScope&) = delete;
        PreviewTextureScope& operator=(const PreviewTextureScope&) = delete;

        ~PreviewTextureScope()
        {
            previewTextureMaximumEdge = previous;
        }
    };

    struct TextureRows
    {
        size_t bytes = 0, count = 0;
    };

    inline TextureRows textureRows(D3DFORMAT format, UINT width, UINT height)
    {
        if (!width || !height || width > 16384 || height > 16384) {
            return {};
        }
        if (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 || format == D3DFMT_DXT3 ||
            format == D3DFMT_DXT4 || format == D3DFMT_DXT5) {
            return {size_t((width + 3) / 4) * (format == D3DFMT_DXT1 ? 8 : 16), (height + 3) / 4};
        }
        size_t bytes = 0;
        switch (format) {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_A8B8G8R8:
            case D3DFMT_X8B8G8R8:
            case D3DFMT_A2R10G10B10:
                bytes = 4;
                break;
            case D3DFMT_R8G8B8:
                bytes = 3;
                break;
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_A4R4G4B4:
            case D3DFMT_A8L8:
                bytes = 2;
                break;
            case D3DFMT_A8:
            case D3DFMT_L8:
                bytes = 1;
                break;
            default:
                return {};
        }
        return {size_t(width) * bytes, height};
    }

    // AddRef preserves an object, not its pixels; previews need their own texture contents
    template <class Device, class Texture>
    Texture* copyPreviewTexture(Device* device, Texture* source, size_t& bytes, unsigned maximumEdge = 0,
                                size_t byteBudget = std::numeric_limits<size_t>::max())
    {
        bytes = 0;
        D3DSURFACE_DESC first{};
        if (!device || !source || FAILED(source->GetLevelDesc(0, &first))) {
            return nullptr;
        }
        UINT levels = source->GetLevelCount();
        UINT firstLevel = 0;
        if (levels > 16) {
            return nullptr;
        }
        const auto requiredBytes = [&]() {
            size_t total = 0;
            for (UINT level = firstLevel; level < levels; ++level) {
                const UINT shift = level - firstLevel;
                const auto rows = textureRows(first.Format, std::max(1u, first.Width >> shift),
                                              std::max(1u, first.Height >> shift));
                total += rows.bytes * rows.count;
            }
            return total;
        };
        while (firstLevel + 1 < levels &&
               ((maximumEdge && std::max(first.Width, first.Height) > maximumEdge) || requiredBytes() > byteBudget)) {
            ++firstLevel;
            if (FAILED(source->GetLevelDesc(firstLevel, &first))) {
                return nullptr;
            }
        }
        if (requiredBytes() > byteBudget) {
            return nullptr;
        }
        levels -= firstLevel;
        const auto rows = textureRows(first.Format, first.Width, first.Height);
        if (!rows.bytes || !levels || levels > 16) {
            return nullptr;
        }
        Texture* result = nullptr;
        if (SUCCEEDED(device->CreateTexture(first.Width, first.Height, levels, 0, first.Format,
                                           D3DPOOL_MANAGED, &result, nullptr)) && result) {
            bool copied = true;
            for (UINT level = 0; level < levels; ++level) {
                D3DSURFACE_DESC desc{};
                D3DLOCKED_RECT input{}, output{};
                if (FAILED(source->GetLevelDesc(level + firstLevel, &desc)) || desc.Format != first.Format ||
                    desc.Width != std::max(1u, first.Width >> level) ||
                    desc.Height != std::max(1u, first.Height >> level)) {
                    copied = false;
                    break;
                }
                const auto shape = textureRows(desc.Format, desc.Width, desc.Height);
                if (!shape.bytes || FAILED(source->LockRect(level + firstLevel, &input, nullptr, D3DLOCK_READONLY))) {
                    copied = false;
                    break;
                }
                const bool locked = SUCCEEDED(result->LockRect(level, &output, nullptr, 0));
                copied = locked && input.pBits && output.pBits && input.Pitch >= 0 && output.Pitch >= 0 &&
                         size_t(input.Pitch) >= shape.bytes && size_t(output.Pitch) >= shape.bytes;
                if (copied) {
                    for (size_t row = 0; row < shape.count; ++row) {
                        std::memcpy(static_cast<uint8_t*>(output.pBits) + row * size_t(output.Pitch),
                                    static_cast<const uint8_t*>(input.pBits) + row * size_t(input.Pitch), shape.bytes);
                    }
                    bytes += shape.bytes * shape.count;
                }
                if (locked) {
                    result->UnlockRect(level);
                }
                source->UnlockRect(level + firstLevel);
                if (!copied) {
                    break;
                }
            }
            if (copied) {
                return result;
            }
            result->Release();
            result = nullptr;
            bytes = 0;
        }
        if (result) {
            result->Release();
            result = nullptr;
        }
        // dirt/composite textures can be render targets and cannot be CPU-locked
        if (levels != 1 || FAILED(device->CreateTexture(first.Width, first.Height, 1, D3DUSAGE_RENDERTARGET,
                                                       first.Format, D3DPOOL_DEFAULT, &result, nullptr)) || !result) {
            return nullptr;
        }
        IDirect3DSurface9 *input = nullptr, *output = nullptr;
        const bool copied = SUCCEEDED(source->GetSurfaceLevel(firstLevel, &input)) && input &&
                            SUCCEEDED(result->GetSurfaceLevel(0, &output)) && output &&
                            SUCCEEDED(device->StretchRect(input, nullptr, output, nullptr, D3DTEXF_NONE));
        if (input) {
            input->Release();
        }
        if (output) {
            output->Release();
        }
        if (!copied) {
            result->Release();
            return nullptr;
        }
        bytes = rows.bytes * rows.count;
        return result;
    }

    // the native plate atlas stays locked until shutdown; copy its readable rows without touching that lock
    template <class Device, class Texture>
    Texture* copyLockedPreviewTexture(Device* device, Texture* source, const uint8_t* pixels, size_t stride,
                                     size_t& bytes, size_t byteBudget)
    {
        bytes = 0;
        D3DSURFACE_DESC desc{};
        if (!device || !source || !pixels || !source->GetLevelCount() || source->GetLevelCount() > 16 ||
            FAILED(source->GetLevelDesc(0, &desc))) {
            return nullptr;
        }
        const auto rows = textureRows(desc.Format, desc.Width, desc.Height);
        if (desc.Format != D3DFMT_A8R8G8B8 || !rows.bytes || stride < rows.bytes) {
            return nullptr;
        }
        const unsigned levels = source->GetLevelCount();
        size_t required = 0;
        for (unsigned level = 0; level < levels; ++level) {
            required += size_t(std::max(1u, desc.Width >> level)) * std::max(1u, desc.Height >> level) * 4;
        }
        if (required > byteBudget) {
            return nullptr;
        }
        Texture* result = nullptr;
        if (FAILED(device->CreateTexture(desc.Width, desc.Height, levels, 0, desc.Format, D3DPOOL_MANAGED,
                                          &result, nullptr)) || !result) {
            return nullptr;
        }
        unsigned width = desc.Width, height = desc.Height;
        std::vector<uint8_t> current(size_t(width) * height * 4);
        for (unsigned row = 0; row < height; ++row) {
            std::memcpy(current.data() + size_t(row) * width * 4, pixels + size_t(row) * stride, size_t(width) * 4);
        }
        for (unsigned level = 0; level < levels; ++level) {
            D3DLOCKED_RECT output{};
            const bool locked = SUCCEEDED(result->LockRect(level, &output, nullptr, 0));
            const bool valid = locked && output.pBits && output.Pitch >= 0 && size_t(output.Pitch) >= width * 4;
            if (valid) {
                for (unsigned row = 0; row < height; ++row) {
                    std::memcpy(static_cast<uint8_t*>(output.pBits) + size_t(row) * size_t(output.Pitch),
                                current.data() + size_t(row) * width * 4, size_t(width) * 4);
                }
            }
            if (locked) {
                result->UnlockRect(level);
            }
            if (!valid) {
                result->Release();
                return nullptr;
            }
            if (level + 1 == levels) {
                break;
            }
            const unsigned nextWidth = std::max(1u, width / 2), nextHeight = std::max(1u, height / 2);
            std::vector<uint8_t> next(size_t(nextWidth) * nextHeight * 4);
            for (unsigned y = 0; y < nextHeight; ++y) {
                for (unsigned x = 0; x < nextWidth; ++x) {
                    for (unsigned channel = 0; channel < 4; ++channel) {
                        unsigned total = 0;
                        for (unsigned dy = 0; dy < 2; ++dy) {
                            for (unsigned dx = 0; dx < 2; ++dx) {
                                const unsigned sx = std::min(width - 1, x * 2 + dx);
                                const unsigned sy = std::min(height - 1, y * 2 + dy);
                                total += current[(size_t(sy) * width + sx) * 4 + channel];
                            }
                        }
                        next[(size_t(y) * nextWidth + x) * 4 + channel] = uint8_t((total + 2) / 4);
                    }
                }
            }
            current = std::move(next);
            width = nextWidth;
            height = nextHeight;
        }
        bytes = required;
        return result;
    }

    struct TextureSnapshots
    {
        static constexpr size_t budget = 64u * 1024 * 1024;
        static constexpr size_t perTextureBudget = 16u * 1024 * 1024;
        std::map<IDirect3DTexture9*, IDirect3DTexture9*> copies;
        size_t bytes = 0;
        unsigned maximumEdge = previewTextureMaximumEdge;

        TextureSnapshots() = default;
        TextureSnapshots(const TextureSnapshots&) = delete;
        TextureSnapshots& operator=(const TextureSnapshots&) = delete;

        ~TextureSnapshots()
        {
            for (auto& copy : copies) {
                if (copy.second) {
                    copy.second->Release();
                }
            }
        }

        IDirect3DTexture9* get(IDirect3DDevice9* device, IDirect3DTexture9* source,
                              const uint8_t* lockedPixels = nullptr, size_t lockedStride = 0)
        {
            if (!source) {
                return nullptr;
            }
            const auto existing = copies.find(source);
            if (existing != copies.end()) {
                if (existing->second) {
                    existing->second->AddRef();
                }
                return existing->second;
            }
            size_t copiedBytes = 0;
            // reuse native mip levels before allocating; one oversized mod must not consume the process address space
            const size_t remaining = std::min(perTextureBudget, budget - bytes);
            auto* copy = lockedPixels
                ? copyLockedPreviewTexture(device, source, lockedPixels, lockedStride, copiedBytes, remaining)
                : copyPreviewTexture(device, source, copiedBytes, maximumEdge, remaining);
            if (!copy) {
                copies.emplace(source, nullptr);
                return nullptr;
            }
            copies.emplace(source, copy);
            bytes += copiedBytes;
            copy->AddRef();
            return copy;
        }
    };
}
