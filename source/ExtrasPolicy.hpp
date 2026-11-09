#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace gt4
{
    inline std::array<std::vector<int>, 2> nativeExtraDomains(const std::array<bool, 6>& present, uint32_t rules)
    {
        std::array<std::vector<int>, 2> domains;
        for (bool raining : {false, true}) {
            std::array<unsigned, 2> rule{{(rules >> 12) & 15, (rules >> 28) & 15}};
            std::array<bool, 2> active{};
            std::array<std::vector<int>, 2> allowed;
            std::array<bool, 6> reserved{};
            for (size_t slot = 0; slot < 2; ++slot) {
                active[slot] = rule[slot] >= 1 && rule[slot] <= 4 && (rule[slot] != 2 || raining);
                if (!active[slot]) {
                    continue;
                }
                if (rule[slot] == 4) {
                    for (size_t i = 0; i < present.size(); ++i) {
                        if (present[i]) {
                            allowed[slot].push_back(int(i));
                        }
                    }
                }
                for (unsigned index = 0; index < 3; ++index) {
                    const unsigned component = (rules >> (unsigned(slot) * 16 + index * 4)) & 15;
                    if (component < present.size() && present[component]) {
                        reserved[component] = true;
                        if (rule[slot] != 4) {
                            allowed[slot].push_back(int(component));
                        }
                    }
                }
            }
            std::vector<int> unreserved;
            if (!(active[0] && rule[0] == 4)) {
                for (size_t i = 0; i < present.size(); ++i) {
                    if (present[i] && !reserved[i]) {
                        unreserved.push_back(int(i));
                    }
                }
            }
            if (!active[0]) {
                allowed[0] = unreserved;
            }
            if (!active[1] && active[0]) {
                allowed[1] = unreserved;
            }
            for (size_t slot = 0; slot < domains.size(); ++slot) {
                domains[slot].insert(domains[slot].end(), allowed[slot].begin(), allowed[slot].end());
            }
        }
        for (auto& domain : domains) {
            std::sort(domain.begin(), domain.end());
            domain.erase(std::unique(domain.begin(), domain.end()), domain.end());
        }
        return domains;
    }

    inline std::string extraOutcomeKey(std::vector<std::string> parts)
    {
        if (parts.empty() || parts.size() > 8192) {
            return {};
        }
        size_t bytes = 0;
        for (const auto& p : parts) {
            if (p.size() > 2 * 1024 * 1024 - bytes) {
                return {};
            }
            bytes += p.size();
        }
        std::sort(parts.begin(), parts.end());
        uint64_t a = 14695981039346656037ull, b = 0x9E3779B97F4A7C15ull;
        for (const auto& p : parts) {
            for (unsigned char c : p) {
                a = (a ^ c) * 1099511628211ull;
                b ^= uint64_t(c) + 0x9E3779B97F4A7C15ull + (b << 6) + (b >> 2);
            }
            a = (a ^ 255) * 1099511628211ull;
            b ^= p.size() + 0x517CC1B727220A95ull + (b << 6) + (b >> 2);
        }
        return std::to_string(parts.size()) + ":" + std::to_string(a) + ":" + std::to_string(b);
    }

    struct VehFuncsVariant
    {
        int seed = 1;
        std::string signature;
    };

    struct VehFuncsCatalogue
    {
        static constexpr size_t maxVariants = 64, maxSamples = 192, quietSamples = 48;
        std::vector<VehFuncsVariant> variants;
        std::array<int8_t, 2> components{{-2, -2}};
        size_t samples = 0, quiet = 0;
        bool stopped = false;

        bool add(int seed, std::string signature, std::array<int8_t, 2> pair)
        {
            if (seed <= 0 || signature.empty() || stopped) {
                return false;
            }
            if (!variants.empty() && pair != components) {
                stopped = true;
                return false;
            }
            ++samples;
            for (const auto& v : variants) {
                if (v.signature == signature) {
                    ++quiet;
                    finish();
                    return false;
                }
            }
            if (variants.empty()) {
                components = pair;
            }
            variants.push_back({seed, std::move(signature)});
            quiet = 0;
            finish();
            return true;
        }

        void finish()
        {
            stopped = samples >= maxSamples || quiet >= quietSamples || variants.size() >= maxVariants;
        }

        int seed(size_t option, int fallback) const
        {
            return option < variants.size() ? variants[option].seed : fallback;
        }

        std::vector<std::string> labels() const
        {
            std::vector<std::string> out{"Default"};
            for (size_t i = 1; i < variants.size(); ++i) {
                out.push_back("Extras " + std::to_string(i));
            }
            return out;
        }
    };

    inline int variantProbeSeed(int model, size_t sample)
    {
        uint32_t x = uint32_t(model) ^ uint32_t(sample + 1) * 0x9E3779B9u;
        x ^= x >> 16;
        x *= 0x7FEB352Du;
        x ^= x >> 15;
        x *= 0x846CA68Bu;
        x ^= x >> 16;
        return int((x & 0x7FFFFFFFu) | 1u);
    }
}
