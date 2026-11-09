#pragma once
#include "AssetPaths.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <new>
#include <string>
#include <vector>

namespace gt4
{
    // load the bundled face privately so another mod or windows font installation cannot replace it.
    class PrivateFont
    {
        HANDLE resource = nullptr;
        std::wstring selected = L"Arial", actual;
        std::array<uint8_t, 54> head{};
        const char* result = "not-loaded";
        DWORD error = ERROR_SUCCESS;

        static uint32_t bigEndian(const uint8_t* value)
        {
            return uint32_t(value[0]) << 24 | uint32_t(value[1]) << 16 | uint32_t(value[2]) << 8 | value[3];
        }

        bool readHead(const std::vector<uint8_t>& bytes)
        {
            if (bytes.size() < 12) {
                return false;
            }
            const size_t count = size_t(bytes[4]) << 8 | bytes[5];
            if (!count || count > (bytes.size() - 12) / 16) {
                return false;
            }
            for (size_t i = 0; i < count; ++i) {
                const uint8_t* table = bytes.data() + 12 + i * 16;
                if (std::memcmp(table, "head", 4)) {
                    continue;
                }
                const size_t offset = bigEndian(table + 8), length = bigEndian(table + 12);
                if (length < head.size() || offset > bytes.size() || length > bytes.size() - offset) {
                    return false;
                }
                std::memcpy(head.data(), bytes.data() + offset, head.size());
                return true;
            }
            return false;
        }

        static HFONT makeFont(const wchar_t* family, int em)
        {
            return CreateFontW(-em,
                               0,
                               0,
                               0,
                               FW_BOLD,
                               FALSE,
                               FALSE,
                               FALSE,
                               ANSI_CHARSET,
                               OUT_TT_ONLY_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY,
                               DEFAULT_PITCH,
                               family);
        }

        void releaseResource()
        {
            if (resource) {
                RemoveFontMemResourceEx(resource);
                resource = nullptr;
            }
        }

    public:
        ~PrivateFont()
        {
            unload();
        }

        PrivateFont() = default;
        PrivateFont(const PrivateFont&) = delete;
        PrivateFont& operator=(const PrivateFont&) = delete;

        bool load(const std::wstring& path, const std::wstring& family)
        {
            unload();
            const auto native = assetPath(path);
            const DWORD attributes = GetFileAttributesW(native.c_str());
            if (path.empty() || family.empty() || attributes == INVALID_FILE_ATTRIBUTES ||
                (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                result = "font-file-unavailable";
                error = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_INVALID_PARAMETER;
                return false;
            }
            std::ifstream input(native, std::ios::binary | std::ios::ate);
            const auto size = input ? input.tellg() : std::streampos(-1);
            if (size < 12 || uint64_t(size) > 16 * 1024 * 1024) {
                result = "font-file-size-or-read";
                return false;
            }
            std::vector<uint8_t> bytes;
            try {
                bytes.resize(static_cast<size_t>(size));
            } catch (const std::bad_alloc&) {
                result = "font-allocation-failed";
                error = ERROR_NOT_ENOUGH_MEMORY;
                return false;
            }
            input.seekg(0);
            if (!input.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(bytes.size())) || !readHead(bytes)) {
                result = "font-data-invalid";
                error = ERROR_BAD_FORMAT;
                return false;
            }
            DWORD count = 0;
            resource = AddFontMemResourceEx(bytes.data(), DWORD(bytes.size()), nullptr, &count);
            if (!resource || !count) {
                error = GetLastError();
                result = "font-registration-failed";
                releaseResource();
                return false;
            }
            selected = family;
            HDC dc = CreateCompatibleDC(nullptr);
            if (!dc) {
                result = "font-dc-failed";
                releaseResource();
                selected = L"Arial";
                return false;
            }
            HFONT font = createFont(dc, 29);
            if (font) {
                DeleteObject(font);
            }
            DeleteDC(dc);
            return loaded();
        }

        HFONT createFont(HDC dc, int em)
        {
            HFONT font = makeFont(selected.c_str(), em);
            if (!font) {
                result = "font-object-failed";
                releaseResource();
                selected = L"Arial";
                return makeFont(selected.c_str(), em);
            }
            const auto previous = SelectObject(dc, font);
            if (!previous || previous == HGDI_ERROR) {
                result = "font-selection-failed";
                DeleteObject(font);
                releaseResource();
                selected = L"Arial";
                return makeFont(selected.c_str(), em);
            }
            wchar_t family[128]{};
            const int characters = GetTextFaceW(dc, 128, family);
            actual = characters > 0 ? family : L"";
            std::array<uint8_t, 54> realized{};
            const DWORD table = DWORD('h') | DWORD('e') << 8 | DWORD('a') << 16 | DWORD('d') << 24;
            const bool faceMatches = characters > 0 && _wcsicmp(family, selected.c_str()) == 0;
            const bool dataMatches =
                !resource || (GetFontData(dc, table, 0, realized.data(), DWORD(realized.size())) == realized.size() &&
                              realized == head);
            SelectObject(dc, previous);
            if (resource && (!faceMatches || !dataMatches)) {
                result = faceMatches ? "font-data-substituted" : "font-family-substituted";
                DeleteObject(font);
                releaseResource();
                selected = L"Arial";
                return makeFont(selected.c_str(), em);
            }
            if (resource) {
                result = "ready";
            }
            return font;
        }

        void unload()
        {
            releaseResource();
            selected = L"Arial";
            actual.clear();
            head = {};
            result = "not-loaded";
            error = ERROR_SUCCESS;
        }

        const wchar_t* face() const
        {
            return selected.c_str();
        }

        const wchar_t* resolvedFace() const
        {
            return actual.c_str();
        }

        const char* status() const
        {
            return result;
        }

        DWORD systemError() const
        {
            return error;
        }

        std::string diagnostic() const
        {
            return std::string(result) + "; win32=" + std::to_string(error) + "; family=" + utf8Text(selected) +
                   "; realized=" + utf8Text(actual);
        }

        bool loaded() const
        {
            return resource && std::strcmp(result, "ready") == 0;
        }
    };
}
