#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

namespace gt4
{
    inline std::string utf8Text(const std::wstring& value)
    {
        if (value.empty()) {
            return {};
        }
        const int size = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) {
            return {};
        }
        std::string text(size_t(size), '\0');
        WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), text.data(), size, nullptr, nullptr);
        return text;
    }

    inline std::filesystem::path assetPath(const std::filesystem::path& path)
    {
        std::wstring value = path.wstring();
        std::replace(value.begin(), value.end(), L'/', L'\\');
        if (value.size() < MAX_PATH || value.compare(0, 4, L"\\\\?\\") == 0) {
            return value;
        }
        if (!path.is_absolute()) {
            std::error_code error;
            const auto absolute = std::filesystem::absolute(path, error);
            if (error) {
                return value;
            }
            value = absolute.wstring();
            std::replace(value.begin(), value.end(), L'/', L'\\');
        }
        if (value.compare(0, 2, L"\\\\") == 0) {
            return L"\\\\?\\UNC\\" + value.substr(2);
        }
        return L"\\\\?\\" + value;
    }

    inline std::filesystem::path moduleDirectory(HMODULE module, DWORD* error = nullptr)
    {
        std::array<wchar_t, 32768> path{};
        SetLastError(ERROR_SUCCESS);
        const DWORD length = GetModuleFileNameW(module, path.data(), DWORD(path.size()));
        if (!length || length >= path.size()) {
            if (error) {
                *error = length ? ERROR_INSUFFICIENT_BUFFER : GetLastError();
            }
            return {};
        }
        if (error) {
            *error = ERROR_SUCCESS;
        }
        std::wstring value(path.data(), length);
        if (value.compare(0, 8, L"\\\\?\\UNC\\") == 0) {
            value = L"\\\\" + value.substr(8);
        } else if (value.compare(0, 4, L"\\\\?\\") == 0) {
            value.erase(0, 4);
        }
        return std::filesystem::path(value).parent_path();
    }
}
