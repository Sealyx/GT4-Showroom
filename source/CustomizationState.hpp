#pragma once
#include "TuningPlan.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace gt4
// customization state for customization
// i combined front, rear, skirts, bonnet into one for easier navigation
// no you don't get hydraulics, stero and nitrous 

{
    inline const char* customizationSlotName(int slot)
    {
        switch (slot) {
            case 0:
                return "Hood";
            case 1:
                return "Hood vents";
            case 2:
                return "Right hood vent";
            case 6:
                return "Spoiler";
            case 8:
                return "Side skirts";
            case 9:
                return "Right side skirt";
            case 10:
                return "Front bull bar";
            case 11:
                return "Rear bull bar";
            case 12:
                return "Headlights";
            case 14:
                return "Roof";
            case 15:
                return "Nitrous";
            case 16:
                return "Hydraulics";
            case 17:
                return "Stereo";
            case 33:
                return "Chassis";
            case 34:
                return "Wheels";
            case 44:
                return "Front bumper";
            case 45:
                return "Rear bumper";
            case 51:
                return "Exhaust";
            case 52:
                return "Accessories 1";
            case 53:
                return "Accessories 2";
            case 54:
                return "Accessories 3";
            default:
                return nullptr;
        }
    }

    struct CustomizationPart
    {
        int id = -1, slot = -1, linked = -1;
        std::string label;
        bool compatible = true;
    };

    struct CustomizationOption
    {
        std::string label;
        std::vector<int> parts, slots;
    };

    struct CustomizationGroup
    {
        std::string label;
        int slot = -1, choice = 0;
        std::vector<CustomizationOption> options;
    };

    using CustomizationAppearance = std::array<int, 12>;

    struct CustomizationCaptureRetry
    {
        uint64_t revision = 0, nextAttempt = 0;
        CustomizationAppearance appearance{};
        unsigned attempts = 0;

        bool allow(uint64_t selectedRevision, const CustomizationAppearance& selectedAppearance, uint64_t now)
        {
            if (selectedRevision != revision || selectedAppearance != appearance) {
                revision = selectedRevision;
                appearance = selectedAppearance;
                attempts = 0;
                nextAttempt = 0;
            }
            if (attempts >= 3 || now < nextAttempt) {
                return false;
            }
            ++attempts;
            nextAttempt = now + 250;
            return true;
        }

        bool exhausted() const
        {
            return attempts >= 3;
        }
    };

    struct CustomizationState
    {
        static constexpr size_t maxParts = 1024, maxOptions = 512;
        int model = -1;
        uint64_t generation = 0, revision = 0;
        std::vector<CustomizationGroup> groups;

        void clear()
        {
            model = -1;
            groups.clear();
            ++generation;
            ++revision;
        }

        void begin(int id, const std::vector<CustomizationPart>& parts)
        {
            clear();
            if (id < 0) {
                return;
            }
            model = id;
            std::map<int, const CustomizationPart*> available;
            for (const auto& part : parts) {
                if (available.size() >= maxParts) {
                    break;
                }
                if (part.id >= 0 && customizationSlotName(part.slot)) {
                    available.emplace(part.id, &part);
                }
            }
            std::map<int, CustomizationGroup> slots;
            std::set<std::vector<int>> seen;
            for (const auto& item : available) {
                const auto& part = *item.second;
                if (!part.compatible || seen.size() >= maxOptions) {
                    continue;
                }
                CustomizationOption option;
                option.label = part.label;
                if (option.label.empty()) {
                    option.label = customizationSlotName(part.slot);
                }
                option.parts.push_back(part.id);
                option.slots.push_back(part.slot);
                if (part.linked >= 0) {
                    const auto linked = available.find(part.linked);
                    if (part.linked == part.id || linked == available.end() ||
                        (linked->second->linked >= 0 && linked->second->linked != part.id)) {
                        continue;
                    }
                    option.parts.push_back(linked->first);
                    option.slots.push_back(linked->second->slot);
                }
                std::sort(option.parts.begin(), option.parts.end());
                std::sort(option.slots.begin(), option.slots.end());
                option.slots.erase(std::unique(option.slots.begin(), option.slots.end()), option.slots.end());
                if (!seen.insert(option.parts).second) {
                    continue;
                }
                const int slot = option.slots.front();
                auto& group = slots[slot];
                if (group.options.empty()) {
                    group.slot = slot;
                    group.label = customizationSlotName(slot);
                    group.options.push_back({"Stock", {}, {}});
                }
                if (option.slots == std::vector<int>({10, 11})) {
                    group.label = "Bull bars";
                }
                group.options.push_back(std::move(option));
            }
            for (auto& group : slots) {
                groups.push_back(std::move(group.second));
            }
        }

        bool select(size_t row, int option)
        {
            if (row >= groups.size() || option < 0 || size_t(option) >= groups[row].options.size()) {
                return false;
            }
            auto& group = groups[row];
            if (group.choice == option) {
                return true;
            }
            const auto& wanted = group.options[size_t(option)].slots;
            for (size_t i = 0; i < groups.size(); ++i) {
                auto& other = groups[i];
                if (i == row || other.choice <= 0) {
                    continue;
                }
                const auto& occupied = other.options[size_t(other.choice)].slots;
                if (std::any_of(wanted.begin(), wanted.end(), [&](int slot) {
                        return std::find(occupied.begin(), occupied.end(), slot) != occupied.end();
                    })) {
                    other.choice = 0;
                }
            }
            group.choice = option;
            ++revision;
            return true;
        }

        bool hasSelection() const noexcept
        {
            return std::any_of(groups.begin(), groups.end(), [](const CustomizationGroup& group) {
                return group.choice > 0;
            });
        }

        std::vector<int> selectedParts() const
        {
            std::vector<int> parts;
            for (const auto& group : groups) {
                const auto& selected = group.options[size_t(group.choice)].parts;
                for (int id : selected) {
                    if (std::find(parts.begin(), parts.end(), id) == parts.end()) {
                        parts.push_back(id);
                    }
                }
            }
            return parts;
        }
    };
}
