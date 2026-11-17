#pragma once
#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "LanguageKeys.hpp"

namespace gt4
{
    inline bool validLanguageText(const std::string &text)
    {
        return !text.empty() && text.size() <= 4096 &&
               std::none_of(text.begin(), text.end(), [](unsigned char c) { return c < 32 || c == 127; }) &&
               MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0) > 0;
    }

    inline std::wstring wideLanguageText(const std::string &text)
    {
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (count <= 0)
        {
            return {};
        }
        std::wstring result(size_t(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), count);
        return result;
    }

    inline std::string languageKey(const std::string &text)
    {
        uint32_t hash = 2166136261u;
        for (unsigned char value : text)
        {
            hash = (hash ^ value) * 16777619u;
        }
        char key[10]{};
        std::snprintf(key, sizeof(key), "T%08lX", static_cast<unsigned long>(hash));
        return key;
    }

    struct Localization
    {
        static constexpr std::array<const char *, 7> folders{"english",  "indonesian", "spanish", "chinese",
                                                             "japanese", "korean",     "russian"};
        static constexpr std::array<const char *, 7> names{"English",
                                                           "Bahasa Indonesia",
                                                           "Espa\xC3\xB1ol",
                                                           "\xE4\xB8\xAD\xE6\x96\x87",
                                                           "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E",
                                                           "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4",
                                                           "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9"};
        std::array<std::unordered_map<std::string, std::string>, 7> packs;
        std::array<bool, 7> ready{};
        std::unordered_map<std::string, std::string> cache;
        std::unordered_set<std::string> protectedNames;
        std::vector<std::string> fragments;
        int selected = 0;

        Localization()
        {
            ready[0] = true;
        }

        const wchar_t *family() const
        {
            if (selected == 3)
            {
                return L"Microsoft YaHei";
            }
            if (selected == 4)
            {
                return L"Yu Gothic";
            }
            if (selected == 5)
            {
                return L"Malgun Gothic";
            }
            return L"Segoe UI";
        }

        // a partial translation must never silently become a partly translated menu
        bool readPack(const std::filesystem::path &folder, std::unordered_map<std::string, std::string> &output)
        {
            std::unordered_map<std::string, std::string> values;
            std::error_code error;
            size_t bytes = 0, files = 0;
            for (const auto &entry : std::filesystem::directory_iterator(folder, error))
            {
                if (error || !entry.is_regular_file(error) || entry.path().extension() != L".fxt")
                {
                    continue;
                }
                const auto size = entry.file_size(error);
                if (error || size > 2 * 1024 * 1024 || (bytes += size) > 2 * 1024 * 1024)
                {
                    return false;
                }
                std::ifstream input(entry.path(), std::ios::binary);
                std::string line;
                ++files;
                while (std::getline(input, line))
                {
                    if (line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
                    {
                        line.erase(0, 3);
                    }
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (line.empty() || line[0] == '#')
                    {
                        continue;
                    }
                    const size_t split = line.find_first_of(" \t");
                    if (split == std::string::npos || split != 9)
                    {
                        return false;
                    }
                    const size_t first = line.find_first_not_of(" \t", split);
                    const auto value = first == std::string::npos ? std::string{} : line.substr(first);
                    if (!validLanguageText(value) || !values.emplace(line.substr(0, split), value).second)
                    {
                        return false;
                    }
                }
                if (!input.eof())
                {
                    return false;
                }
            }
            if (error || !files || values.size() != languageSourceStrings.size())
            {
                return false;
            }
            std::unordered_map<std::string, std::string> result;
            for (const auto *source : languageSourceStrings)
            {
                const auto found = values.find(languageKey(source));
                if (found == values.end())
                {
                    return false;
                }
                result.emplace(source, found->second);
            }
            output = std::move(result);
            return true;
        }

        void load(const std::filesystem::path &root)
        {
            cache.clear();
            fragments.clear();
            packs = {};
            ready = {};
            for (size_t i = 0; i < folders.size(); ++i)
            {
                ready[i] = readPack(root / folders[i], packs[i]);
            }
            // english remains a safe built-in fallback if a package is missing or damaged
            ready[0] = true;
            for (const auto *source : languageSourceStrings)
            {
                fragments.emplace_back(source);
            }
            std::sort(fragments.begin(), fragments.end(),
                      [](const auto &a, const auto &b) { return a.size() > b.size(); });
            select(selected);
        }

        bool select(int value)
        {
            if (value < 0 || value >= int(ready.size()) || !ready[size_t(value)])
            {
                selected = 0;
                cache.clear();
                return false;
            }
            selected = value;
            cache.clear();
            return true;
        }

        const std::string &text(const std::string &source)
        {
            const auto &pack = packs[size_t(selected)];
            if (pack.empty() || protectedNames.count(source))
            {
                return source;
            }
            const auto exact = pack.find(source);
            if (exact != pack.end())
            {
                return exact->second;
            }
            const auto cached = cache.find(source);
            if (cached != cache.end())
            {
                return cached->second;
            }
            std::string result;
            for (size_t offset = 0; offset < source.size();)
            {
                bool matched = false;
                for (const auto &fragment : fragments)
                {
                    const size_t end = offset + fragment.size();
                    if (end > source.size() || source.compare(offset, fragment.size(), fragment) != 0 ||
                        (offset && std::isalnum(static_cast<unsigned char>(source[offset - 1]))) ||
                        (end < source.size() && std::isalnum(static_cast<unsigned char>(source[end]))))
                    {
                        continue;
                    }
                    const auto translated = pack.find(fragment);
                    if (translated != pack.end())
                    {
                        result += translated->second;
                        offset = end;
                        matched = true;
                        break;
                    }
                }
                if (!matched)
                {
                    result += source[offset++];
                }
            }
            if (result == source)
            {
                return source;
            }
            if (cache.size() >= 512)
            {
                cache.clear();
            }
            return cache.emplace(source, std::move(result)).first->second;
        }
    };

    inline Localization localization;
    inline const std::string &localized(const std::string &source)
    {
        return localization.text(source);
    }
} // namespace gt4
