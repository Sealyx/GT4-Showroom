#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "Countries.hpp"
#include "AudioLevels.hpp"
#include "StudioPreferences.hpp"
#include "CinematicVehicle.hpp"
#include "VehicleViewer.hpp"
#include "CustomizationState.hpp"

namespace gt4
{
    struct UIRect
    {
        float x = 0, y = 0, w = 0, h = 0;

        bool contains(float a, float b) const
        {
            return a >= x && b >= y && a < x + w && b < y + h;
        }
    };
    enum class StudioAction
    {
        None,
        SortAscending,
        SortDescending,
        SortClass,
        SortSpeed,
        SortCountry,
        SortModelId,
        Favorites,
        AllCars,
        RefreshFavorites,
        Cinematic,
        FilterCountry,
        FilterClass,
        Shadows,
        CarLogo,
        BrandLogo,
        Extra1,
        Extra2,
        VehFuncsExtra,
        DisplayModelId,
        DisplayModelName,
        Hotspots,
        ViewerEnter,
        ViewerIdle,
        ViewerScene,
        ViewerColor,
        ViewerFloor,
        ViewerPalette,
        ViewerReflections,
        ViewerShadows,
        ViewerPause,
        EnterTransFender
    };

    inline int extraActionGroup(StudioAction action)
    {
        switch (action) {
            case StudioAction::Extra1:
                return 0;
            case StudioAction::Extra2:
                return 1;
            case StudioAction::VehFuncsExtra:
                return 2;
            default:
                return -1;
        }
    }

    inline StudioAction extraGroupAction(size_t group)
    {
        static constexpr std::array<StudioAction, 3> actions{
            {StudioAction::Extra1, StudioAction::Extra2, StudioAction::VehFuncsExtra}};
        return group < actions.size() ? actions[group] : StudioAction::None;
    }

    struct ViewTransition
    {
        StudioAction action = StudioAction::None;
        int phase = 0;
        double began = 0;
        bool black = false;

        bool active() const
        {
            return phase != 0;
        }

        bool request(StudioAction a, double now)
        {
            if (active()) {
                return false;
            }
            action = a;
            phase = 1;
            began = now;
            black = false;
            return true;
        }

        double fadeDuration() const
        {
            return .16;
        }

        float alpha(double now) const
        {
            const float t = float((now - began) / fadeDuration());
            const float x = std::clamp(t, 0.f, 1.f), v = x * x * (3 - 2 * x);
            return phase == 1 ? v : phase == 2 ? 1.f : phase == 3 ? 1 - v : 0;
        }

        void step(double now)
        {
            if (phase == 1 && now - began >= fadeDuration()) {
                phase = 2;
            } else if (phase == 3 && now - began >= fadeDuration()) {
                phase = 0;
                action = StudioAction::None;
                black = false;
            }
        }

        void presented(double now)
        {
            if (phase == 2 || (phase == 1 && now - began >= fadeDuration())) {
                black = true;
            }
        }

        bool ready(double) const
        {
            return phase == 2 && black;
        }

        void applied(double now)
        {
            phase = 3;
            began = now;
        }
    };
    enum class MenuBranch
    {
        None,
        Sort,
        Countries,
        Classes,
        Shadows,
        Logos,
        BrandLogo,
        CarLogo,
        Extras,
        Extra1,
        Extra2,
        VehFuncsExtra,
        Display,
        DisplayModelId,
        DisplayModelName,
        Hotspots,
        ViewerIdle,
        ViewerScene,
        ViewerColor,
        ViewerFloor,
        ViewerPalette,
        ViewerReflections,
        ViewerShadows,
        ViewerPause
    };

    inline int extraBranchGroup(MenuBranch branch)
    {
        switch (branch) {
            case MenuBranch::Extra1:
                return 0;
            case MenuBranch::Extra2:
                return 1;
            case MenuBranch::VehFuncsExtra:
                return 2;
            default:
                return -1;
        }
    }

    struct MenuItem
    {
        std::string label;
        StudioAction action = StudioAction::None;
        int value = -1;
        MenuBranch child = MenuBranch::None;
        bool checked = false;
    };

    struct CustomizationMenuRow
    {
        std::string label;
        std::vector<std::string> options;
        std::vector<int> partModels;
        int choice = 0, slot = -1;
    };

    inline constexpr size_t classCount = 13;
    inline constexpr const char* classNames[] = {"Normal",
                                                 "Poor",
                                                 "Rich",
                                                 "Executive",
                                                 "Worker",
                                                 "Heavy",
                                                 "Taxi",
                                                 "Moped",
                                                 "Motorbike",
                                                 "Leisure Boat",
                                                 "Work Boat",
                                                 "Bicycle",
                                                 "Special"};

    inline int classIndex(int value)
    {
        return value >= 0 && value < 12 ? value : 12;
    }

    enum class SettingsView
    {
        Showroom,
        Cinematic,
        VehicleViewer
    };

    struct ContextMenu
    {
        bool monitorPicker = false, monitorHover = false, monitorHeaderHover = false, monitorKeyboard = false;
        int monitorDragLevel = -1, monitorHelpLevel = -1, monitorHelpIndex = -1, monitorHoverLevel = -1,
            monitorHoverIndex = -1, monitorFooter = -1, monitorFooterLevel = 0;
        float monitorDragX = 0, monitorDragY = 0;
        std::array<float, 3> monitorX{}, monitorY{};
        std::array<bool, 3> monitorPlaced{};
        double monitorChanged = 0;
        int closeResult = 0, sliderDrag = -1;
        bool sliderChanged = false;
        bool hotspots = true;
        ViewerSettings viewerSettings;
        SettingsView settingsView = SettingsView::Showroom;
        bool viewerPaused = false;
        bool customizationEnabled = false;
        int customizationModel = -1;
        uint64_t customizationGeneration = 0, customizationRevision = 0;
        std::vector<CustomizationMenuRow> customizations;
        bool open = false, sub = false, countries = false, countryFocus = false;
        int row = 0, sortRow = 0, countryRow = 0, choice = -1, focusLevel = 0;
        float x = .5f, y = .5f;
        double began = 0;
        mutable std::array<int, 3> monitorFirst{};
        std::vector<int> available{-1}, classes{-1};
        std::array<int, countryCount> counts{};
        std::array<int, classCount> classCounts{};
        std::array<std::vector<std::string>, 3> extras;
        std::array<int, 3> extraChoice{{0, 0, 0}};
        bool shadows = true, carLogo = false, brandLogo = false,
             displayModelId = false, displayModelName = false;
        int order = 0, countryFilter = -1, classFilter = -1;

        void sync(int o,
                  int country,
                  int cls,
                  bool shadow,
                  bool car,
                  bool brand,
                  bool showId = false,
                  bool showName = false)
        {
            order = o;
            countryFilter = country;
            classFilter = cls;
            shadows = shadow;
            carLogo = car;
            brandLogo = brand;
            displayModelId = showId;
            displayModelName = showName;
        }

        void setCountries(const std::array<int, countryCount>& c)
        {
            if (counts == c) {
                return;
            }
            const int focused =
                thirdBranch() == MenuBranch::Countries && countryRow >= 0 && size_t(countryRow) < available.size()
                    ? available[size_t(countryRow)]
                    : countryFilter;
            counts = c;
            available = {-1};
            for (size_t i = 0; i < countryCount; ++i) {
                if (c[i]) {
                    available.push_back(int(i));
                }
            }
            std::sort(available.begin() + 1, available.end(), [&](int a, int b) {
                if (c[size_t(a)] != c[size_t(b)]) {
                    return c[size_t(a)] > c[size_t(b)];
                }
                const std::string an = a ? countryLabel(Country(a)) : "Unassigned",
                                  bn = b ? countryLabel(Country(b)) : "Unassigned";
                return an < bn;
            });
            if (thirdBranch() == MenuBranch::Countries) {
                const auto i = std::find(available.begin(), available.end(), focused);
                countryRow = i == available.end() ? 0 : int(i - available.begin());
            }
        }

        void setClasses(const std::array<int, classCount>& c)
        {
            if (classCounts == c) {
                return;
            }
            const int focused =
                thirdBranch() == MenuBranch::Classes && countryRow >= 0 && size_t(countryRow) < classes.size()
                    ? classes[size_t(countryRow)]
                    : classFilter;
            classCounts = c;
            classes = {-1};
            for (size_t i = 0; i < classCount; ++i) {
                if (c[i]) {
                    classes.push_back(int(i));
                }
            }
            std::sort(classes.begin() + 1, classes.end(), [&](int a, int b) {
                return c[size_t(a)] != c[size_t(b)] ? c[size_t(a)] > c[size_t(b)]
                                                    : std::string(classNames[size_t(a)]) < classNames[size_t(b)];
            });
            if (thirdBranch() == MenuBranch::Classes) {
                const auto i = std::find(classes.begin(), classes.end(), focused);
                countryRow = i == classes.end() ? 0 : int(i - classes.begin());
            }
        }

        void setExtras(const std::vector<std::string>& first,
                       const std::vector<std::string>& second,
                       const std::vector<std::string>& vehfuncs)
        {
            if (extras[0] == first && extras[1] == second && extras[2] == vehfuncs) {
                return;
            }
            extras[0] = first;
            extras[1] = second;
            extras[2] = vehfuncs;
            row = std::min(row, int(rootItems().size()) - 1);
        }

        void setCustomization(const CustomizationState& source, bool enabled)
        {
            customizationEnabled = enabled && source.model >= 0 && source.groups.size() <= 32;
            if (source.model != customizationModel || source.generation != customizationGeneration) {
                customizations.clear();
                if (source.groups.size() <= 32) {
                    customizations.reserve(source.groups.size());
                    for (const auto& group : source.groups) {
                        CustomizationMenuRow menu;
                        menu.label = group.label;
                        menu.choice = group.choice;
                        menu.slot = group.slot;
                        menu.options.reserve(group.options.size());
                        for (const auto& option : group.options) {
                            menu.options.push_back(option.label);
                            menu.partModels.push_back(option.parts.empty() ? -1 : option.parts.front());
                        }
                        customizations.push_back(std::move(menu));
                    }
                }
                customizationModel = source.model;
                customizationGeneration = source.generation;
                customizationRevision = source.revision;
            } else if (source.revision != customizationRevision && source.groups.size() == customizations.size()) {
                for (size_t i = 0; i < customizations.size(); ++i) {
                    customizations[i].choice = source.groups[i].choice;
                }
                customizationRevision = source.revision;
            }
        }

        std::vector<MenuItem> rootItems() const
        {
            if (settingsView == SettingsView::VehicleViewer) {
                std::vector<MenuItem> rows{{"Idle", StudioAction::None, -1, MenuBranch::ViewerIdle},
                                           {"Background", StudioAction::None, -1, MenuBranch::ViewerScene}};
                if (viewerSettings.scene != ViewerScene::Custom) {
                    rows.push_back({"Color", StudioAction::None, -1, MenuBranch::ViewerColor});
                    rows.push_back({"Floor", StudioAction::None, -1, MenuBranch::ViewerFloor});
                    rows.push_back({"Lighting", StudioAction::None, -1, MenuBranch::ViewerPalette});
                    rows.push_back({"Reflections", StudioAction::None, -1, MenuBranch::ViewerReflections});
                }
                rows.push_back({"Shadows", StudioAction::None, -1, MenuBranch::ViewerShadows});
                rows.push_back({"Rotation", StudioAction::None, -1, MenuBranch::ViewerPause});
                rows.push_back({"Hotspots", StudioAction::None, -1, MenuBranch::Hotspots});
                if (customizationEnabled) {
                    rows.push_back({"Customization", StudioAction::EnterTransFender});
                }
                return rows;
            }
            if (settingsView == SettingsView::Cinematic) {
                std::vector<MenuItem> rows{{"Hotspots", StudioAction::None, -1, MenuBranch::Hotspots},
                                           {"Shadows", StudioAction::None, -1, MenuBranch::Shadows}};
                if (customizationEnabled) {
                    rows.push_back({"Customization", StudioAction::EnterTransFender});
                }
                rows.push_back({"Vehicle Viewer", StudioAction::ViewerEnter});
                return rows;
            }
            std::vector<MenuItem> rows{{"Sort by", StudioAction::None, -1, MenuBranch::Sort},
                                       {"Display", StudioAction::None, -1, MenuBranch::Display},
                                       {"Logos", StudioAction::None, -1, MenuBranch::Logos},
                                       {"Shadows", StudioAction::None, -1, MenuBranch::Shadows}};
            if (!extras[0].empty() || !extras[1].empty() || !extras[2].empty()) {
                rows.push_back({"Extras", StudioAction::None, -1, MenuBranch::Extras});
            }
            rows.push_back({"Cinematic", StudioAction::Cinematic});
            rows.push_back({"Vehicle Viewer", StudioAction::ViewerEnter});
            return rows;
        }

        std::vector<MenuItem> branchItems(MenuBranch branch) const
        {
            switch (branch) {
                case MenuBranch::Sort:
                    return {{"Ascending", StudioAction::SortAscending, 0, MenuBranch::None, order == 0},
                            {"Descending", StudioAction::SortDescending, 1, MenuBranch::None, order == 1},
                            {"Class", StudioAction::None, -1, MenuBranch::Classes, classFilter >= 0},
                            {"Top Speed", StudioAction::SortSpeed, 3, MenuBranch::None, order == 3},
                            {"Countries", StudioAction::None, -1, MenuBranch::Countries, countryFilter >= 0},
                            {"Model ID", StudioAction::SortModelId, 5, MenuBranch::None, order == 5}};
                case MenuBranch::Countries: {
                    std::vector<MenuItem> r;
                    for (int c : available) {
                        std::string label = c < 0 ? "All Countries" : c == 0 ? "Unassigned" : countryLabel(Country(c));
                        if (c >= 0) {
                            label += "  (" + std::to_string(counts[size_t(c)]) + ")";
                        }
                        r.push_back({label, StudioAction::FilterCountry, c, MenuBranch::None, countryFilter == c});
                    }
                    return r;
                }
                case MenuBranch::Classes: {
                    std::vector<MenuItem> r;
                    for (int c : classes) {
                        std::string label = c < 0 ? "All Classes" : classNames[size_t(c)];
                        if (c >= 0) {
                            label += "  (" + std::to_string(classCounts[size_t(c)]) + ")";
                        }
                        r.push_back({label, StudioAction::FilterClass, c, MenuBranch::None, classFilter == c});
                    }
                    return r;
                }
                case MenuBranch::Hotspots:
                    return {{"On", StudioAction::Hotspots, 1, MenuBranch::None, hotspots},
                            {"Off", StudioAction::Hotspots, 0, MenuBranch::None, !hotspots}};
                case MenuBranch::ViewerIdle:
                    return {{"On", StudioAction::ViewerIdle, 1, MenuBranch::None, viewerSettings.idleEnabled},
                            {"Off", StudioAction::ViewerIdle, 0, MenuBranch::None, !viewerSettings.idleEnabled}};
                case MenuBranch::ViewerScene: {
                    std::vector<MenuItem> rows;
                    for (size_t i = 0; i < viewerSceneNames.size(); ++i) {
                        if (!viewerSceneNames[i]) {
                            continue;
                        }
                        rows.push_back({viewerSceneNames[i], StudioAction::ViewerScene, int(i), MenuBranch::None,
                                        int(viewerSettings.scene) == int(i)});
                    }
                    return rows;
                }
                case MenuBranch::ViewerColor:
                case MenuBranch::ViewerFloor: {
                    const bool wall = branch == MenuBranch::ViewerColor;
                    const auto action = wall ? StudioAction::ViewerColor : StudioAction::ViewerFloor;
                    const int selection = wall ? viewerSettings.wallColor : viewerSettings.floorColor;
                    std::vector<MenuItem> rows;
                    rows.reserve(size_t(viewerWhite + 2));
                    const auto addColor = [&](int id) {
                        rows.push_back({viewerSurfaceName(id), action, id, MenuBranch::None, selection == id});
                    };
                    addColor(-1);
                    addColor(viewerBlack);
                    for (int i = 0; i < viewerBlack; ++i) {
                        addColor(i);
                        if (std::string_view(viewerPalettes[size_t(i)].name) == "White") {
                            addColor(viewerWhite);
                        }
                    }
                    return rows;
                }
                case MenuBranch::ViewerPalette: {
                    std::vector<MenuItem> rows;
                    for (size_t i = 0; i < viewerPalettes.size(); ++i) {
                        rows.push_back({viewerPalettes[i].name, StudioAction::ViewerPalette, int(i), MenuBranch::None,
                                        viewerSettings.palette == int(i)});
                    }
                    return rows;
                }
                case MenuBranch::ViewerReflections:
                    return {{"On", StudioAction::ViewerReflections, 1, MenuBranch::None, viewerSettings.reflections},
                            {"Off", StudioAction::ViewerReflections, 0, MenuBranch::None, !viewerSettings.reflections}};
                case MenuBranch::ViewerShadows:
                    return {{"On", StudioAction::ViewerShadows, 1, MenuBranch::None, viewerSettings.shadows},
                            {"Off", StudioAction::ViewerShadows, 0, MenuBranch::None, !viewerSettings.shadows}};
                case MenuBranch::ViewerPause:
                    return {{"Playing", StudioAction::ViewerPause, 0, MenuBranch::None, !viewerPaused},
                            {"Paused", StudioAction::ViewerPause, 1, MenuBranch::None, viewerPaused}};
                case MenuBranch::Shadows:
                    return {{"On", StudioAction::Shadows, 1, MenuBranch::None, shadows},
                            {"Off", StudioAction::Shadows, 0, MenuBranch::None, !shadows}};
                case MenuBranch::Logos:
                    return {{"Manufacturer", StudioAction::None, -1, MenuBranch::BrandLogo, brandLogo},
                            {"Car name", StudioAction::None, -1, MenuBranch::CarLogo, carLogo}};
                case MenuBranch::BrandLogo:
                    return {{"On", StudioAction::BrandLogo, 1, MenuBranch::None, brandLogo},
                            {"Off", StudioAction::BrandLogo, 0, MenuBranch::None, !brandLogo}};
                case MenuBranch::CarLogo:
                    return {{"On", StudioAction::CarLogo, 1, MenuBranch::None, carLogo},
                            {"Off", StudioAction::CarLogo, 0, MenuBranch::None, !carLogo}};
                case MenuBranch::Display:
                    return {{"Model ID", StudioAction::None, -1, MenuBranch::DisplayModelId, displayModelId},
                            {"Model name", StudioAction::None, -1, MenuBranch::DisplayModelName, displayModelName}};
                case MenuBranch::DisplayModelId:
                    return {{"On", StudioAction::DisplayModelId, 1, MenuBranch::None, displayModelId},
                            {"Off", StudioAction::DisplayModelId, 0, MenuBranch::None, !displayModelId}};
                case MenuBranch::DisplayModelName:
                    return {{"On", StudioAction::DisplayModelName, 1, MenuBranch::None, displayModelName},
                            {"Off", StudioAction::DisplayModelName, 0, MenuBranch::None, !displayModelName}};
                case MenuBranch::Extras: {
                    std::vector<MenuItem> r;
                    if (!extras[0].empty()) {
                        r.push_back({"Extras 1", StudioAction::Extra1, extraChoice[0]});
                    }
                    if (!extras[1].empty()) {
                        r.push_back({"Extras 2", StudioAction::Extra2, extraChoice[1]});
                    }
                    if (!extras[2].empty()) {
                        r.push_back({"VehFuncs", StudioAction::VehFuncsExtra, extraChoice[2]});
                    }
                    return r;
                }
                case MenuBranch::Extra1:
                case MenuBranch::Extra2:
                case MenuBranch::VehFuncsExtra: {
                    const int group = extraBranchGroup(branch);
                    std::vector<MenuItem> r;
                    for (size_t i = 0; i < extras[size_t(group)].size(); ++i) {
                        r.push_back({extras[size_t(group)][i],
                                     extraGroupAction(size_t(group)),
                                     int(i),
                                     MenuBranch::None,
                                     extraChoice[size_t(group)] == int(i)});
                    }
                    return r;
                }
                default:
                    return {};
            }
        }

        MenuBranch childBranch() const
        {
            const auto r = rootItems();
            return row >= 0 && size_t(row) < r.size() ? r[size_t(row)].child : MenuBranch::None;
        }

        MenuBranch thirdBranch() const
        {
            if (!sub) {
                return MenuBranch::None;
            }
            const auto branch = childBranch();
            if ((branch == MenuBranch::Logos || branch == MenuBranch::Extras || branch == MenuBranch::Display) &&
                focus() == 0) {
                return MenuBranch::None;
            }
            const auto r = branchItems(branch);
            return sortRow >= 0 && size_t(sortRow) < r.size() ? r[size_t(sortRow)].child : MenuBranch::None;
        }

        std::vector<MenuItem> items(int level) const
        {
            return level == 0 ? rootItems() : branchItems(level == 1 ? childBranch() : thirdBranch());
        }

        std::array<int, 3> previewExtras() const
        {
            auto selected = extraChoice;
            if (open && focus() == 2) {
                const auto branch = thirdBranch();
                const int branchGroup = extraBranchGroup(branch);
                if (branchGroup >= 0) {
                    const size_t group = size_t(branchGroup);
                    if (countryRow >= 0 && size_t(countryRow) < extras[group].size()) {
                        selected[group] = countryRow;
                    }
                }
            }
            return selected;
        }

        int current(int level) const
        {
            return level == 0 ? row : level == 1 ? sortRow : countryRow;
        }

        int depth() const
        {
            return !open ? 0 : !sub ? 1 : thirdBranch() == MenuBranch::None ? 2 : 3;
        }

        int focus() const
        {
            return countryFocus ? 2 : focusLevel;
        }

        void path()
        {
            sub = childBranch() != MenuBranch::None;
            const auto r = items(1);
            sortRow = r.empty() ? 0 : std::clamp(sortRow, 0, int(r.size()) - 1);
            countries = thirdBranch() == MenuBranch::Countries;
            const auto t = items(2);
            countryRow = t.empty() ? 0 : std::clamp(countryRow, 0, int(t.size()) - 1);
            if (t.empty()) {
                countryFocus = false;
            }
        }

        void toggle(float px, float py, double now)
        {
            monitorToggle(px, py, now);
        }

        int windowStart(int level, float h) const
        {
            return monitorStart(level, h);
        }

        std::array<UIRect, 3> panels(float w, float h) const
        {
            return monitorPanels(w, h);
        }

        UIRect panel(int level, float w, float h) const
        {
            return monitorPanels(w, h)[size_t(level)];
        }

        UIRect root(float w, float h) const
        {
            return panel(0, w, h);
        }

        UIRect child(float w, float h) const
        {
            return panel(1, w, h);
        }

        UIRect grandchild(float w, float h) const
        {
            return panel(2, w, h);
        }

        void hover(float px, float py, float w, float h)
        {
            monitorPoint(px, py, w, h);
        }

        void move(int dx, int dy)
        {
            monitorMove(dx, dy);
        }

        StudioAction activate(bool)
        {
            return monitorActivate();
        }

        StudioAction click(float px, float py, float w, float h, bool)
        {
            return monitorClick(px, py, w, h);
        }

        bool monitorSubWindow() const
        {
            const auto b = childBranch();
            return b == MenuBranch::Logos || b == MenuBranch::Extras || b == MenuBranch::Display;
        }

        bool monitorSelectorWindow() const
        {
            return sub && focus() == 2 && !monitorSubWindow();
        }

        void monitorToggle(float px, float py, double now);
        int monitorStart(int level, float h, float listHeight = 0) const;
        std::array<UIRect, 3> monitorPanels(float w, float h) const;
        UIRect monitorField(int level, int index, float w, float h) const;
        UIRect monitorDropdown(float w, float h) const;
        UIRect monitorButton(int level, int which, float w, float h) const;
        UIRect monitorListRow(UIRect list, int level, int index, float h) const;
        UIRect monitorSlider(int index, float w, float h) const;
        bool monitorSlide(float px, float py, float w, float h, bool down, bool pressed);

        bool sliderWindow() const
        {
            return childBranch() == MenuBranch::Extras;
        }

        int extraGroup(int index) const
        {
            const auto rows = items(1);
            return index >= 0 && size_t(index) < rows.size() ? extraActionGroup(rows[size_t(index)].action) : -1;
        }

        int sliderMaximum(int index) const
        {
            const int group = extraGroup(index);
            return group >= 0 ? std::max(0, int(extras[size_t(group)].size()) - 1) : 0;
        }

        int sliderValue(int index) const
        {
            const int group = extraGroup(index);
            return group >= 0 ? extraChoice[size_t(group)] : 0;
        }

        bool setSliderValue(int index, int value)
        {
            value = std::clamp(value, 0, sliderMaximum(index));
            const int group = extraGroup(index);
            int* target = group >= 0 ? &extraChoice[size_t(group)] : nullptr;
            if (!target || *target == value) {
                return false;
            }
            *target = value;
            sliderChanged = true;
            return true;
        }

        bool fieldOpensWindow(int level, int index) const;
        int audibleTarget() const;

        void finish(int result)
        {
            closeResult = result;
            open = sub = countries = countryFocus = monitorPicker = false;
            sliderDrag = -1;
        }

        bool monitorFieldSelected(int level, int index, bool single = false) const;
        bool monitorButtonSelected(int level, int which) const;
        UIRect pointerTarget(float w, float h) const;
        std::string monitorValue(int level, int index) const;
        std::string monitorTitle(int level) const;
        std::string help() const;
        void monitorPoint(float px, float py, float w, float h);
        void monitorMove(int dx, int dy);
        void monitorBack();
        StudioAction monitorActivate();
        StudioAction monitorClick(float px, float py, float w, float h);
        bool monitorDrag(float px, float py, float w, float h, bool down, bool pressed);

        void selectedAction()
        {
            monitorPicker = false;
            monitorFooter = -1;
            if (!sub) {
                countryFocus = false;
                focusLevel = 0;
                return;
            }
            if (focus() == 2) {
                countryFocus = false;
                focusLevel = monitorSubWindow() ? 1 : 0;
                sub = monitorSubWindow();
            }
        }
    };

#include "MonitorSettings.inl"

    struct CinematicView
    {
        bool active = false, paused = false, grabbed = false, dragged = false;
        float mix = 0, yaw = 0, pitch = .23f, zoom = 1, zoomTarget = 1;
        double began = 0, lightHeld = 0, lightBegan = 0;
        float grabX = 0, grabY = 0;

        float angle(double now) const
        {
            return yaw + (!paused ? float(now - began) * .314159265f : 0.f);
        }

        double lightTime(double now) const
        {
            return lightHeld + (!paused ? now - lightBegan : 0.);
        }

        void enter(double now, float angle)
        {
            active = true;
            paused = grabbed = dragged = false;
            yaw = angle;
            began = lightBegan = now;
            lightHeld = now;
            pitch = .23f;
            zoomTarget = 1.10f;
        }

        void leave()
        {
            active = grabbed = false;
            zoomTarget = 1;
        }

        void toggle(double now)
        {
            yaw = angle(now);
            lightHeld = lightTime(now);
            began = lightBegan = now;
            paused = !paused;
        }

        void orbit(float dx, float dy, double now)
        {
            if (std::abs(dx) + std::abs(dy) < .0001f) {
                return;
            }
            yaw = angle(now) + dx;
            lightHeld = lightTime(now);
            began = lightBegan = now;
            paused = true;
            pitch = std::clamp(pitch + dy, .03f, 1.15f);
        }

        void wheel(float v)
        {
            zoomTarget = std::clamp(zoomTarget * std::exp(v * .10f), .55f, 2.4f);
        }

        void step(double dt)
        {
            const float t = float(1 - std::exp(-dt * 12));
            mix += ((active ? 1.f : 0.f) - mix) * t;
            zoom += (zoomTarget - zoom) * t;
        }
    };

    struct ControllerKeyboard
    {
        bool open = false;
        int selected = 0;

        static std::string key(int i)
        {
            static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            return i < 36    ? chars.substr(size_t(i), 1)
                   : i == 36 ? "SPACE"
                   : i == 37 ? "DEL"
                   : i == 38 ? "FIND"
                             : "CLOSE";
        }

        void move(int dx, int dy)
        {
            selected = ((selected / 10 + dy + 4) % 4) * 10 + (selected % 10 + dx + 10) % 10;
        }

        UIRect box(float w, float h, int i) const
        {
            const float s = h / 720.f, cw = 48 * s, ch = 37 * s;
            return {(w - 10 * cw) * .5f + (i % 10) * cw, h * .68f + (i / 10) * ch, cw - 3 * s, ch - 3 * s};
        }

        int type(std::string& query)
        {
            if (selected == 38) {
                return 1;
            }
            if (selected == 39) {
                return -1;
            }
            if (selected == 37) {
                if (!query.empty()) {
                    query.pop_back();
                }
            } else if (query.size() < 48) {
                query += selected == 36 ? " " : key(selected);
            }
            return 0;
        }
    };

    struct StudioUI
    {
        ContextMenu menu;
        CinematicView cinematic, menuCinematic;
        CinematicVehicleState vehicle, menuVehicle;
        AudioLevels audioLevels;
        StudioPreferences menuPreferences;
        std::array<int, 3> menuExtras{{0, 0, 0}};
        bool menuShadows = true;
        bool menuSnapshot = false;
        ControllerKeyboard keyboard;
        ViewTransition transition;
        bool favoritesOnly = false, controllerUsed = false;
        int sortMode = 5, countryFilter = -1, classFilter = -1;
        bool shadows = true, carLogo = false, brandLogo = false, displayModelId = false, displayModelName = false;
        double pointerMoved = 0;
        bool dock = false;

        StudioPreferences preferences() const
        {
            StudioPreferences result;
            result.sortMode = sortMode;
            result.countryFilter = countryFilter;
            result.classFilter = classFilter;
            result.carLogo = carLogo;
            result.brandLogo = brandLogo;
            result.displayModelId = displayModelId;
            result.displayModelName = displayModelName;
            result.audio = audioLevels;
            return result;
        }

        void applyPreferences(const StudioPreferences& preferences)
        {
            sortMode = preferences.sortMode;
            countryFilter = preferences.countryFilter;
            classFilter = preferences.classFilter;
            carLogo = preferences.carLogo;
            brandLogo = preferences.brandLogo;
            displayModelId = preferences.displayModelId;
            displayModelName = preferences.displayModelName;
            audioLevels = preferences.audio;
        }

        bool countryNotice(Country country, bool searching, bool searchResult) const
        {
            return country == Country::Unknown && !cinematic.active && !searching && !searchResult;
        }

        bool modal() const
        {
            return menu.open || keyboard.open;
        }

        void mouse(double now, float dx, float dy)
        {
            if (std::abs(dx) + std::abs(dy) > .03f) {
                pointerMoved = now;
                dock = false;
                controllerUsed = false;
            }
        }

        void resetTransient()
        {
            menu.open = false;
            keyboard.open = false;
            cinematic.leave();
            vehicle.reset();
            menuSnapshot = false;
            transition = {};
            dock = false;
        }
    };
}
