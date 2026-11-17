#pragma once
#include <string>
#include <cctype>
#include <cstdint>
#include <charconv>
#include <vector>

namespace gt4
{
    enum class SearchKind
    {
        Any,
        Name,
        Id
    };

    inline std::string trimName(const std::string& s)
    {
        const auto a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
        return a == std::string::npos ? std::string{} : s.substr(a, b - a + 1);
    }

    inline std::string lowerName(std::string s)
    {
        for (auto& c : s) {
            if (c >= 'A' && c <= 'Z') {
                c = char(c + 32);
            }
        }
        return s;
    }

    inline uint32_t modelNameKey(const std::string& name)
    {
        uint32_t key = 0xFFFFFFFF;
        for (unsigned char c : name) {
            if (c >= 'a' && c <= 'z') {
                c = uint8_t(c - 32);
            }
            key ^= c;
            for (int bit = 0; bit < 8; ++bit) {
                key = (key >> 1) ^ ((key & 1) ? 0xEDB88320u : 0);
            }
        }
        return key;
    }

    inline bool decimalModelId(const std::string& text, int& id)
    {
        if (text.empty()) {
            return false;
        }
        for (char c : text) {
            if (c < '0' || c > '9') {
                return false;
            }
        }
        const auto r = std::from_chars(text.data(), text.data() + text.size(), id);
        return r.ec == std::errc{} && r.ptr == text.data() + text.size() && id >= 0 && id < 1000000;
    }

    template <class E> bool sameModelName(const E& e, const std::string& name)
    {
        return (e.modelKey && e.modelKey == modelNameKey(name)) || lowerName(e.name) == lowerName(name);
    }

    // keep exact names first, then cycle partial matches without retaining native pointers
    template <class E>
    size_t findModel(const std::vector<E>& entries, const std::string& raw, int previousModel = -1,
                     SearchKind kind = SearchKind::Any)
    {
        const auto query = trimName(raw);
        if (query.empty()) {
            return entries.size();
        }
        int id = -1;
        const bool numeric = decimalModelId(query, id);
        bool onlyDigits = true;
        for (char c : query) {
            onlyDigits = onlyDigits && c >= '0' && c <= '9';
        }
        if (kind == SearchKind::Id || (kind == SearchKind::Any && (numeric || onlyDigits))) {
            for (size_t i = 0; i < entries.size(); ++i) {
                if (numeric && entries[i].id == id) {
                    return i;
                }
            }
            return entries.size();
        }
        const auto text = lowerName(query);
        const auto rank = [&](const E& entry) {
            const auto display = lowerName(entry.name);
            const auto model = lowerName(entry.modelName);
            if (sameModelName(entry, query) || model == text) {
                return 0;
            }
            if (display.rfind(text, 0) == 0 || model.rfind(text, 0) == 0) {
                return 1;
            }
            if (display.find(text) != std::string::npos || model.find(text) != std::string::npos) {
                return 2;
            }
            return -1;
        };
        size_t first = entries.size();
        bool afterPrevious = previousModel < 0;
        for (int priority = 0; priority < 3; ++priority) {
            for (size_t i = 0; i < entries.size(); ++i) {
                if (rank(entries[i]) != priority) {
                    continue;
                }
                if (first == entries.size()) {
                    first = i;
                }
                if (afterPrevious) {
                    return i;
                }
                afterPrevious = entries[i].id == previousModel;
            }
        }
        return first;
    }
}
