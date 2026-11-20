#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include "ModelNames.hpp"
#include "PreviewMotion.hpp"
#include "Countries.hpp"
#include "StudioUI.hpp"
#include "VehicleViewer.hpp"
#include "TransFender.hpp"
#include "ViewerPhoto.hpp"
#include "SearchPanel.hpp"
#include "LicensePlate.hpp"
#include "InformationPanel.hpp"

namespace gt4
// main showroom controller
// mostly to maintain main controls, favorites, car's classes, ids, etc
{
    constexpr float pi = 3.14159265358979323846f;

    struct V3
    {
        float x = 0, y = 0, z = 0;
    };

    inline V3 operator+(V3 a, V3 b)
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    inline V3 operator-(V3 a, V3 b)
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    inline V3 operator*(V3 a, float s)
    {
        return {a.x * s, a.y * s, a.z * s};
    }

    inline float dot(V3 a, V3 b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline V3 cross(V3 a, V3 b)
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    inline V3 unit(V3 a)
    {
        const float n = std::sqrt(dot(a, a));
        return n > .00001f ? a * (1.f / n) : V3{0, 0, 1};
    }

    inline float ease(float x)
    {
        x = std::clamp(x, 0.f, 1.f);
        return x * x * (3 - 2 * x);
    }

    struct Paint
    {
        uint8_t ids[4] = {0, 0, 0, 0};
        std::string name;
        uint32_t rgb[4] = {0x94999C, 0x94999C, 0x94999C, 0x94999C};
    };

    inline const char* classLabel(int value)
    {
        static const char* labels[] = {"NORMAL",
                                       "POOR",
                                       "RICH",
                                       "EXECUTIVE",
                                       "WORKER",
                                       "HEAVY",
                                       "TAXI",
                                       "MOPED",
                                       "MOTORBIKE",
                                       "LEISURE BOAT",
                                       "WORK BOAT",
                                       "BICYCLE"};
        return value >= 0 && value < 12 ? labels[value] : "SPECIAL";
    }

    struct Entry
    {
        Country country = Country::Unknown;
        int id = 0, kind = 0, classId = 0;
        uint32_t modelKey = 0;
        float topSpeed = -1, handlingMass = 0;
        unsigned handlingGears = 0;
        char handlingDrive = '?';
        bool handlingValid = false, customInformation = false;
        std::string category = "NORMAL", name, modelName, brand = "SAN ANDREAS", title, year, power, torque,
                    information;
        std::vector<Paint> paints;
        bool favorite = false;
    };

    inline bool twoWheelerPreview(const Entry& entry)
    {
        const int category = classIndex(entry.classId);
        return entry.kind == 9 || entry.kind == 10 || category == 7 || category == 8 || category == 11;
    }

    struct Livery
    {
        int index = -1, txd = -1;
        std::shared_ptr<void> texture;
        float focusU = .5f, focusV = .5f;
        std::shared_ptr<void> appearanceTexture{};
    };
    enum class Page
    {
        Cars,
        Paint,
        Livery,
        Extras,
        Prize
    };

    inline const char* showroomControlHint(Page page, bool controller, bool liveries, bool extras = false, bool colors = true)
    {
        if (page == Page::Extras) {
            return controller ? "D-PAD Adjust Extras    CROSS Continue    CIRCLE Back"
                              : "ARROWS Adjust Extras    ENTER Continue    BACKSPACE Back";
        }
        if (controller) {
            if (page == Page::Cars) {
                return "CROSS Paint    SQUARE Favorite    TRIANGLE Favorites    SELECT Menu    L3 Search    SQUARE+TRIANGLE Info    L1+R1 Hide UI";
            }
            if (page == Page::Livery) {
                if (!colors) { return "CROSS Continue    CIRCLE Vehicles    SELECT Menu    L1+R1 Hide UI"; }
                if (extras) { return "CROSS Extras    CIRCLE Colors    SELECT Menu    L1+R1 Hide UI"; }
                return "CROSS Spawn    CIRCLE Colors    SELECT Menu    L1+R1 Hide UI";
            }
            if (!liveries && extras) { return "CROSS Extras    CIRCLE Vehicles    SELECT Menu    L1+R1 Hide UI"; }
            return liveries ? "CROSS Liveries    CIRCLE Vehicles    SELECT Menu    L1+R1 Hide UI"
                            : "CROSS Spawn    CIRCLE Vehicles    SELECT Menu    L1+R1 Hide UI";
        }
        if (page == Page::Cars) {
            return "ENTER Paint    UP Favorite    F Favorites    RMB Menu    ` Search    I Info    H Hide UI";
        }
        if (page == Page::Livery) {
            if (!colors) { return "ENTER Continue    BACKSPACE Vehicles    RMB Menu    H Hide UI"; }
            if (extras) { return "ENTER Extras    BACKSPACE Colors    RMB Menu    H Hide UI"; }
            return "ENTER Spawn    BACKSPACE Colors    RMB Menu    H Hide UI";
        }
        if (!liveries && extras) { return "ENTER Extras    BACKSPACE Vehicles    RMB Menu    H Hide UI"; }
        return liveries ? "ENTER Liveries    BACKSPACE Vehicles    RMB Menu    H Hide UI"
                        : "ENTER Spawn    BACKSPACE Vehicles    RMB Menu    H Hide UI";
    }
    enum class Cue
    {
        None,
        Move,
        Accept,
        Back,
        Favorite,
        Open,
        Spawn,
        Error,
        Slider,
        WashSpin,
        WashComplete
    };

    enum class SpawnReview
    {
        None,
        Initial,
        Cinematic,
        Viewer,
        LiveryWarning,
        ExtrasWarning,
        ShopExit,
        ShopReset
    };

    enum class ReviewChoice
    {
        Cinematic,
        Exit,
        Spawn,
        Dismiss,
        BackToShowroom
    };

    inline UIRect spawnReviewPanel(float width, float height)
    {
        const float scale = height / 720.f;
        const float panelWidth = std::min(560 * scale, width * .88f);
        return {(width - panelWidth) * .5f, height * .37f, panelWidth, 156 * scale};
    }

    inline UIRect spawnReviewButton(float width, float height, int index, int count)
    {
        const auto panel = spawnReviewPanel(width, height);
        const float scale = height / 720.f;
        const float buttonWidth = panel.w * (count == 3 ? .284f : .365f);
        const float spacing = panel.w * (count == 3 ? .308f : .525f);
        const float left = panel.x + panel.w * .055f + spacing * index;
        return {left, panel.y + 114 * scale, buttonWidth, 25 * scale};
    }

    enum class Tutorial { None, Cinematic, CinematicWithCustomization, CustomizationIntro, Viewer, Customization };

    inline unsigned tutorialTopics(Tutorial tutorial)
    {
        switch (tutorial) {
        case Tutorial::Cinematic: return 1u << 0;
        case Tutorial::CinematicWithCustomization: return (1u << 0) | (1u << 1);
        case Tutorial::CustomizationIntro: return 1u << 1;
        case Tutorial::Viewer: return 1u << 2;
        case Tutorial::Customization: return 1u << 3;
        default: return 0;
        }
    }

    struct Model
    {
        StudioUI ui;
        VehicleViewer viewer;
        CinematicView beforeViewer;
        VehicleViewer menuViewer;
        TransFender shop;
        ViewerPhoto photo;
        bool informationOpen = false;
        bool tutorialsEnabled = false;
        unsigned tutorialsSeen = 0;
        Tutorial tutorial = Tutorial::None;
        ReviewMotion tutorialMotion;
        bool tutorialFocus = true, shopTutorialOffered = false;
        Page extrasFrom = Page::Paint;
        Page lastCompletedSelection = Page::Cars;

        const char* cinematicBackLabel() const
        {
            if (postSelectionCinematic) {
                switch (lastCompletedSelection) {
                case Page::Paint: return "Back to Colors";
                case Page::Livery: return "Back to Liveries";
                case Page::Extras: return "Back to Extras";
                default: break;
                }
            }
            return "Back to Showroom";
        }
        bool extrasResolved = true, selectionAdvancePending = false, skipSelectionExtras = false;
        bool selectionHasColors = true;

        bool appearanceResetPending = false, extrasResetPending = false;
        bool twoWheelerSelected = false;
        void resetVehicleAppearance()
        {
            paint = livery = 0;
            ui.menu.extraChoice = {};
            whitePaintRequested = false;
            lastCompletedSelection = Page::Cars;
            appearanceResetPending = extrasResetPending = true;
        }

        bool usesLivery() const
        {
            return page == Page::Livery || (page == Page::Paint && livery > 0) ||
                   (page == Page::Extras && extrasFrom == Page::Livery);
        }
        bool hasExtras() const
        {
            return std::any_of(ui.menu.extras.begin(), ui.menu.extras.end(),
                               [](const auto& group) { return group.size() > 1; });
        }

        void openReview(SpawnReview next)
        {
            dragging = false;
            spawnReview = next;
            review = {clock};
            pendingReviewChoice = -1;
            reviewChoice = 0;
        }

        void reviewSelectedAppearance()
        {
            if (!extrasResolved) {
                selectionAdvancePending = true;
                return;
            }
            selectionAdvancePending = false;
            openReview(hasExtras() ? SpawnReview::ExtrasWarning : SpawnReview::Initial);
            cue = Cue::Accept;
        }

        void setSelectionPage(Page next)
        {
            if (next == Page::Cars) { resetVehicleAppearance(); }
            page = next;
            stripChanged = clock;
            ui.menu.selectionExtras = next == Page::Extras;
            ui.menu.sliderDrag = -1;
            if (ui.menu.selectionExtras) {
                ui.menu.began = clock;
                ui.menu.sortRow = 0;
                ui.menu.focusLevel = 1;
                ui.menu.monitorKeyboard = true;
                ui.menu.monitorFooter = -1;
                ui.menu.monitorHoverLevel = ui.menu.monitorHoverIndex = -1;
            }
        }

        bool tutorialOpen() const { return tutorial != Tutorial::None; }

        void showTutorial(Tutorial candidate)
        {
            const unsigned unseen = tutorialTopics(candidate) & ~tutorialsSeen;
            if (tutorialsEnabled && !informationOpen && !tutorialOpen() && unseen != 0) {
                tutorial = candidate;
                tutorialMotion = {clock};
                tutorialFocus = true;
            }
        }

        unsigned closeTutorial(bool acknowledge)
        {
            if (!tutorialOpen() || tutorialMotion.leaving >= 0.) { return 0; }
            tutorialMotion.dismiss(clock);
            const unsigned newlySeen = acknowledge ? tutorialTopics(tutorial) & ~tutorialsSeen : 0;
            tutorialsSeen |= newlySeen;
            cue = Cue::Back;
            return newlySeen;
        }
        int informationChoice = 3;
        ReviewMotion informationMotion;
        bool shopExitArmed = false, shopReturnNavigation = false;
        bool viewerSnapshot = false, postSelectionCinematic = false, appearanceReady = true;
        double viewerBegan = 0;
        SpawnReview spawnReview = SpawnReview::None;
        ReviewMotion review;
        bool plateOpen = false, plateEditing = false;
        int plateRow = 3, plateHover = -1;
        int plateAction = -1;
        SpawnReview plateReturn = SpawnReview::None;
        ReviewMotion plateMotion;
        std::string plate;
        int pendingReviewChoice = -1;
        bool whitePaintRequested = false;
        int whitePaintModel = -1;
        int shopResetGroup = -1;
        size_t whitePaintIndex = 0;

        void clearWhitePaint(std::vector<Entry>& cars)
        {
            for (auto& entry : cars) {
                if (entry.id == whitePaintModel && entry.paints.size() == whitePaintIndex + 1) {
                    entry.paints.pop_back();
                    if (paint == whitePaintIndex) {
                        paint = 0;
                    }
                    break;
                }
            }
            whitePaintModel = -1;
            whitePaintRequested = false;
        }

        void syncWhitePaint(std::vector<Entry>& cars, uint32_t white)
        {
            if (selected >= cars.size()) {
                clearWhitePaint(cars);
                return;
            }
            if (whitePaintModel >= 0 && cars[selected].id != whitePaintModel) {
                clearWhitePaint(cars);
            }
            if (!whitePaintRequested) {
                return;
            }
            whitePaintRequested = false;
            auto& entry = cars[selected];
            Paint color = entry.paints.empty() ? Paint{} : entry.paints[std::min(paint, entry.paints.size() - 1)];
            for (size_t channel = 0; channel < 2; ++channel) {
                color.ids[channel] = 1;
                color.rgb[channel] = white;
            }
            color.name = "White";
            if (whitePaintModel == entry.id && whitePaintIndex < entry.paints.size()) {
                entry.paints[whitePaintIndex] = color;
            } else {
                whitePaintModel = entry.id;
                whitePaintIndex = entry.paints.size();
                entry.paints.push_back(color);
            }
            paint = whitePaintIndex;
        }

        float reviewReveal() const
        {
            return review.reveal(clock);
        }

        float reviewOffset(float height) const
        {
            return review.offset(clock, height);
        }
        int reviewChoice = 0;
        std::vector<Livery> liveries;
        size_t livery = 0;
        int liveriesModel = -1;

        float scrollTarget() const
        {
            return float(std::clamp(int(selected) - 4, 0, std::max(0, int(count) - 8)));
        }

        SecretUnlocksConfig secretUnlocks;
        MenuFade fade;
        bool enthusia = false;
        double motionBegan = 0;
        PrizePlan prize;
        double prizeBegan = 0;
        size_t beforePrize = 0;
        int beforePrizeModel = -1;
        bool prizeReady = false;
        std::mt19937 random{5489u};

        void leavePrize()
        {
            if (page == Page::Prize) {
                selected = beforePrize;
                setSelectionPage(Page::Cars);
                changed = motionBegan = clock;
                scroll = scrollTarget();
                prize = {};
                prizeReady = false;
                status.clear();
            }
        }

        bool busy() const
        {
            return fade.action != CloseAction::None || ui.transition.active() || viewer.transitioning() ||
                   (page == Page::Paint && clock - colorsReturned < .20);
        }

        void recovered()
        {
            fade.reset();
            spawn = false;
            skipSelectionExtras = false;
            opened = clock;
        }

        void presented()
        {
            fade.presented(clock);
            ui.transition.presented(clock);
            viewer.presented(clock);
        }

        void rememberViewerCamera()
        {
            if (!viewerSnapshot && (viewer.active || viewer.targetActive)) {
                beforeViewer = ui.cinematic;
                viewerBegan = clock;
                viewerSnapshot = true;
            }
        }

        bool hotspotsEnabled() const
        {
            return ui.menu.allowHotspots && (viewer.active ? viewer.hotspots : ui.vehicle.hotspots);
        }

        float entryAudioGain() const
        {
            const float viewerFade = viewer.entryAudio ? viewer.opacity(clock) : 0.f;
            return 1.f - std::max(viewerFade, shop.fade(clock));
        }

        bool startCinematic()
        {
            if (!open || !count || busy() || ui.cinematic.active || page == Page::Prize) {
                return false;
            }
            ui.cinematic.enter(clock, enthusia ? enthusiaPose(clock - motionBegan).yaw : heading() + pi);
            appearanceReady = !postSelectionCinematic;
            ui.menu.selectionExtras = false;
            const bool needsCinematic = (tutorialTopics(Tutorial::Cinematic) & ~tutorialsSeen) != 0;
            const bool needsIntro = postSelectionCinematic &&
                                    (tutorialTopics(Tutorial::CustomizationIntro) & ~tutorialsSeen) != 0;
            if (needsIntro) {
                showTutorial(needsCinematic ? Tutorial::CinematicWithCustomization : Tutorial::CustomizationIntro);
            } else if (needsCinematic) {
                showTutorial(Tutorial::Cinematic);
            }
            viewer.lastActivity = clock;
            return true;
        }

        bool startViewer(bool automatic = false)
        {
            if (!open || !count || busy() || spawnReview != SpawnReview::None || page == Page::Prize ||
                (automatic && !ui.cinematic.active) || (postSelectionCinematic && !appearanceReady)) {
                return false;
            }
            if (!viewer.request(true, automatic, clock)) {
                return false;
            }
            if (ui.cinematic.active) {
                rememberViewerCamera();
            }
            return true;
        }

        void requestSpawn()
        {
            spawnReview = SpawnReview::None;
            spawn = true;
            dragging = false;
            fade.request(CloseAction::Spawn, clock, blackAlpha());
            cue = Cue::Spawn;
        }

        void beginPlate(SpawnReview previous)
        {
            plateReturn = previous;
            plateOpen = true;
            plateEditing = true;
            plateRow = 0;
            plateHover = plateAction = -1;
            plate.clear();
            plateMotion = previous == SpawnReview::None ? ReviewMotion{clock} : review;
            ui.keyboard.open = false;
            dragging = false;
            cue = Cue::Open;
        }

        bool plateBackNavigates(bool controller) const
        {
            return !plateEditing || controller || (plate.empty() && !ui.keyboard.open);
        }

        void finishPlate(bool confirm)
        {
            if (!plateOpen || plateAction >= 0) {
                return;
            }
            if (!confirm) {
                plateOpen = false;
                skipSelectionExtras = false;
                spawnReview = plateReturn;
                review = plateMotion;
                reviewChoice = 2;
                pendingReviewChoice = -1;
                ui.keyboard.open = false;
                cue = Cue::Back;
                return;
            }
            plateAction = 1;
            plateMotion.dismiss(clock);
            ui.keyboard.open = false;
            cue = confirm ? Cue::Accept : Cue::Back;
        }

        void returnToCarSelection(Page destination = Page::Cars)
        {
            shop = {};
            shopExitArmed = shopReturnNavigation = false;
            photo = {};
            shopResetGroup = -1;
            viewer.reset(clock);
            viewerSnapshot = false;
            postSelectionCinematic = false;
            appearanceReady = true;
            ui.cinematic.leave();
            ui.vehicle.doorTargets.fill(0);
            ui.vehicle.doors.fill(0);
            ui.vehicle.focus = -1;
            ui.vehicle.centerWheels();
            setSelectionPage(destination);
            if (destination == Page::Paint) { colorsReturned = clock; }
            selectionAdvancePending = skipSelectionExtras = false;
            tutorial = Tutorial::None;
            paintMix = 0;
            dragging = false;
            status.clear();
            stripChanged = clock;
        }

        void returnFromCustomization()
        {
            shop.request(ShopPage::Closed, clock);
            shopExitArmed = false;
            shopReturnNavigation = true;
        }

        int reviewButtons() const
        {
            if (spawnReview == SpawnReview::LiveryWarning) {
                return 2;
            }
            return spawnReview == SpawnReview::Initial || spawnReview == SpawnReview::ExtrasWarning || spawnReview == SpawnReview::ShopExit ||
                           spawnReview == SpawnReview::ShopReset || postSelectionCinematic
                       ? 3
                       : 2;
        }

        void chooseSpawnReview(ReviewChoice choice)
        {
            if (spawnReview == SpawnReview::None || busy() || (choice == ReviewChoice::Spawn && reviewButtons() != 3)) {
                return;
            }
            if (choice == ReviewChoice::Spawn && spawnReview != SpawnReview::ShopReset) {
                finishSpawnReview(choice);
                return;
            }
            if (pendingReviewChoice < 0) {
                pendingReviewChoice = int(choice);
                review.dismiss(clock);
            }
        }

        void finishSpawnReview(ReviewChoice choice)
        {
            pendingReviewChoice = -1;
            const auto review = spawnReview;
            spawnReview = SpawnReview::None;
            if (choice == ReviewChoice::Dismiss) {
                shopExitArmed = review == SpawnReview::ShopExit;
                return;
            }
            if (review == SpawnReview::Cinematic &&
                (choice == ReviewChoice::Cinematic || choice == ReviewChoice::BackToShowroom)) {
                const auto destination = choice == ReviewChoice::Cinematic && postSelectionCinematic
                    ? lastCompletedSelection : Page::Cars;
                returnToCarSelection(destination);
                cue = Cue::Back;
                return;
            }
            if (review == SpawnReview::ShopReset) {
                if (choice != ReviewChoice::Exit) {
                    shopResetGroup = choice == ReviewChoice::Cinematic ? 0 : 1;
                }
                cue = choice == ReviewChoice::Exit ? Cue::Back : Cue::Accept;
                return;
            }
            if (review == SpawnReview::ExtrasWarning) {
                if (choice == ReviewChoice::Cinematic) {
                    extrasFrom = page;
                    setSelectionPage(Page::Extras);
                    cue = Cue::Accept;
                } else if (choice == ReviewChoice::Spawn) {
                    skipSelectionExtras = true;
                    beginPlate(review);
                } else {
                    cue = Cue::Back;
                }
                return;
            }
            if (review == SpawnReview::LiveryWarning) {
                whitePaintRequested = choice == ReviewChoice::Exit;
                page = Page::Livery;
                stripChanged = clock;
                cue = Cue::Accept;
                return;
            }
            if (review == SpawnReview::ShopExit && choice == ReviewChoice::Cinematic) {
                returnFromCustomization();
                cue = Cue::Back;
                return;
            }
            if (choice == ReviewChoice::Exit) {
                shopExitArmed = review == SpawnReview::ShopExit;
                cue = Cue::Back;
                return;
            }
            if (choice == ReviewChoice::Spawn) {
                beginPlate(review);
                return;
            }
            if (review == SpawnReview::Viewer) {
                viewer.request(false, false, clock);
            } else if (review == SpawnReview::Initial) {
                postSelectionCinematic = true;
                startCinematic();
            }
            dragging = false;
            cue = Cue::Accept;
        }

        bool open = false, spawn = false, dragging = false;
        float dragGrip = 0;
        Page page = Page::Cars;
        size_t selected = 0, paint = 0;
        int beforeFavoritesModel = -1;
        double clock = 0, changed = 0, opened = 0, stripChanged = 0;
        double classClick = -10, heavyUntil = -1;
        double colorsReturned = -10;
        bool cursorPressed = false;
        float cursorPressMix = 0;
        float prologueMix = 0;
        std::array<float, studioBackgroundCount> backgroundBlend{{1}};
        float scroll = 0, paintMix = 0;
        size_t count = 0;
        float pointerX = .5f, pointerY = .79f;
        bool pointerVisible = true;
        Cue cue = Cue::None;
        std::string status;
        bool searching = false, searchFailed = false;
        std::string query, searchNotice;
        std::string lastSearch;
        int lastSearchModel = -1;
        bool searchById = false, lastSearchById = false, searchPicker = false, searchEditing = true;
        int searchRow = 1, searchHover = -1, searchResultId = -1, searchModeCursor = 0;
        std::string searchResultName;
        double searchUntil = 0;
        double searchBegan = -1, searchClosed = -1;
        float searchCloseAlpha = 0;
        bool searchKeyboard = false;

        float searchOffset(float scale) const
        {
            if (searching) {
                return (1 - searchOpacity()) * 14 * scale;
            }
            const float start = (1 - searchCloseAlpha) * 14 * scale;
            const float amount = VehicleViewer::ease(float((clock - searchClosed) / .16));
            return start - amount * (start + 14 * scale);
        }

        float searchNoticeOpacity() const
        {
            const float entry = VehicleViewer::ease(float((clock - searchUntil + 2.8) / .16));
            const float leave = VehicleViewer::ease(float((searchUntil - clock) / .16));
            return std::min(entry, leave);
        }

        float searchOpacity() const
        {
            const double elapsed = clock - (searching ? searchBegan : searchClosed);
            if (elapsed < 0 || (!searching && searchClosed < 0)) {
                return 0;
            }
            const float phase = VehicleViewer::ease(float(elapsed / .16));
            return searching ? phase : searchCloseAlpha * (1 - phase);
        }

        void closeSearch()
        {
            if (!searching) {
                return;
            }
            searchCloseAlpha = searchOpacity();
            searchClosed = clock;
            searchKeyboard = ui.keyboard.open;
            searching = false;
            ui.keyboard.open = false;
            cue = Cue::Back;
        }

        bool canBeginSearch() const
        {
            return open && page == Page::Cars && !busy() && !ui.modal() &&
                   !ui.cinematic.active && ui.cinematic.mix <= .001f && !viewer.active &&
                   !viewer.targetActive && !shop.open() && !photo.active && !informationOpen && !tutorialOpen() &&
                   !plateOpen && spawnReview == SpawnReview::None && !searching;
        }

        bool beginSearch()
        {
            if (!canBeginSearch()) {
                return false;
            }
            searching = true;
            searchBegan = clock;
            searchClosed = -1;
            searchKeyboard = false;
            searchPicker = false;
            searchEditing = true;
            searchRow = 1;
            searchHover = searchResultId = -1;
            searchResultName.clear();
            cue = Cue::Open;
            query.clear();
            searchNotice.clear();
            searchFailed = false;
            dragging = false;
            spawn = false;
            return true;
        }

        void submitSearch(const std::vector<Entry>& cars)
        {
            if (busy()) {
                return;
            }
            const auto command = lowerName(trimName(query));
            if (command != lastSearch || searchById != lastSearchById) {
                lastSearchModel = -1;
            }
            lastSearch = command;
            lastSearchById = searchById;
            searchResultId = -1;
            searchResultName.clear();
            if (command == "realbig") {
                const auto next = rollPrize(cars, random, secretUnlocks);
                if (!next) {
                    searchFailed = true;
                    searchNotice = "REALBIG: NO MATCHING MODELS";
                    searchUntil = clock + 2.8;
                    cue = Cue::Error;
                    return;
                }
                if (page != Page::Prize) {
                    beforePrize = selected;
                    beforePrizeModel = cars[selected].id;
                }
                prize = next;
                for (size_t i = 0; i < cars.size(); ++i) {
                    if (cars[i].id == prize.id) {
                        selected = i;
                        break;
                    }
                }
                page = Page::Prize;
                paint = size_t(prize.paint);
                prizeBegan = clock;
                opened = clock;
                prizeReady = false;
                status.clear();
                closeSearch();
                searchNotice.clear();
                dragging = false;
                cue = Cue::None;
                return;
            }
            leavePrize();
            if (command == "enthusia" || command == "gt4") {
                enthusia = command == "enthusia";
                motionBegan = clock;
                closeSearch();
                searchFailed = false;
                searchNotice = enthusia ? "ENTHUSIA PREVIEW" : "GT4 PREVIEW";
                searchUntil = clock + 2.8;
                cue = Cue::Accept;
                return;
            }
            const auto kind = searchById ? SearchKind::Id : SearchKind::Name;
            const auto index = findModel(cars, query, lastSearchModel, kind);
            searchFailed = index == cars.size();
            searchUntil = clock + 2.8;
            if (searchFailed) {
                lastSearchModel = -1;
                searchNotice = "MODEL NOT FOUND";
                cue = Cue::Error;
            } else {
                lastSearchModel = cars[index].id;
                searchResultId = cars[index].id;
                searchResultName = cars[index].name;
                select(index, cars);
                closeSearch();
                searchNotice = "MODEL FOUND: " + cars[index].name + "  /  " + std::to_string(cars[index].id);
                cue = Cue::Back;
            }
        }

        void begin(size_t total)
        {
            plateOpen = plateEditing = false;
            plate.clear();
            plateAction = -1;
            if (!total) {
                return;
            }
            count = total;
            shop = {};
            shopExitArmed = shopReturnNavigation = false;
            photo = {};
            shopResetGroup = -1;
            beforeFavoritesModel = -1;
            ui.resetTransient();
            backgroundBlend.fill(0.f);
            backgroundBlend[size_t(ui.backgroundStyle())] = 1.f;
            prologueMix = float(ui.showroomStyle == 1);
            viewer.reset(clock);
            viewerSnapshot = postSelectionCinematic = false;
            spawnReview = SpawnReview::None;
            appearanceReady = true;
            ui.pointerMoved = clock;
            livery = 0;
            liveries.clear();
            liveriesModel = -1;
            fade.reset();
            prize = {};
            prizeReady = false;
            motionBegan = clock;
            open = true;
            searching = false;
            searchBegan = searchClosed = -1;
            searchCloseAlpha = 0;
            query.clear();
            searchNotice.clear();
            lastSearch.clear();
            lastSearchModel = -1;
            searchById = lastSearchById = searchPicker = false;
            searchEditing = true;
            searchRow = 1;
            searchHover = searchResultId = -1;
            searchResultName.clear();
            spawn = false;
            dragging = false;
            setSelectionPage(Page::Cars);
            selected = std::min(selected, count - 1);
            opened = changed = clock;
            informationOpen = false;
            tutorial = Tutorial::None;
            shopTutorialOffered = false;
            ui.menu.selectionExtras = false;
            selectionAdvancePending = skipSelectionExtras = false;
            stripChanged = clock;
            scroll = scrollTarget();
            pointerX = .5f;
            pointerY = .79f;
            paintMix = 0;
            status.clear();
            cue = Cue::Open;
        }

        void step(double seconds)
        {
            seconds = std::isfinite(seconds) ? std::clamp(seconds, 0., 1.) : 0;
            clock += seconds;
            if (tutorialOpen() && tutorialMotion.finished(clock)) {
                tutorial = Tutorial::None;
                viewer.lastActivity = clock;
            }
            if (informationOpen && informationMotion.finished(clock)) {
                informationOpen = false;
            }
            if (plateOpen && plateAction >= 0 && plateMotion.finished(clock)) {
                plateOpen = false;
                if (plateAction == 1) {
                    if (plateReturn == SpawnReview::ShopExit) {
                        shop.page = shop.destination = ShopPage::Closed;
                        shop.transitionStart = -1.;
                    }
                    requestSpawn();
                } else {
                    skipSelectionExtras = false;
                    spawnReview = plateReturn;
                    review = {clock};
                    reviewChoice = 2;
                    pendingReviewChoice = -1;
                }
                plateAction = -1;
            }
            if (spawnReview != SpawnReview::None && pendingReviewChoice >= 0 && review.finished(clock)) {
                finishSpawnReview(static_cast<ReviewChoice>(pendingReviewChoice));
            }
            const bool washing = shop.washStart >= 0.;
            shop.step(clock);
            if (!shop.open()) {
                shopTutorialOffered = false;
            } else if (!shopTutorialOffered && shop.page == ShopPage::Menu && !shop.locked()) {
                shopTutorialOffered = true;
                showTutorial(Tutorial::Customization);
            }
            if (washing && shop.washStart < 0.) {
                cue = Cue::WashComplete;
            }
            ui.cinematic.step(seconds);
            cursorPressMix += (float(cursorPressed) - cursorPressMix) * float(1 - std::exp(-seconds * 35));
            prologueMix += (float(ui.showroomStyle == 1) - prologueMix) *
                           float(1 - std::exp(-seconds * 6));
            if (std::abs(prologueMix - float(ui.showroomStyle == 1)) < .001f) {
                prologueMix = float(ui.showroomStyle == 1);
            }
            const int style = ui.backgroundStyle();
            for (int i = 0; i < studioBackgroundCount; ++i) {
                backgroundBlend[size_t(i)] += (float(i == style) - backgroundBlend[size_t(i)]) *
                                             float(1 - std::exp(-seconds * 7));
            }
            ui.vehicle.step(seconds);
            ui.transition.step(clock);
            const bool viewerWasReady = viewer.active && !viewer.transitioning();
            viewer.step(clock,
                        open && ui.cinematic.active && !ui.modal() && !shop.open() && !photo.active && !searching && !tutorialOpen() &&
                            spawnReview == SpawnReview::None && !plateOpen && fade.action == CloseAction::None &&
                            !ui.transition.active() && (!postSelectionCinematic || appearanceReady));
            if (!viewerWasReady && viewer.active && viewer.targetActive && !viewer.transitioning()) {
                showTutorial(Tutorial::Viewer);
            }
            if (viewer.active && !ui.cinematic.active) {
                ui.cinematic.enter(clock, enthusia ? enthusiaPose(clock - motionBegan).yaw : heading() + pi);
            }
            if (ui.cinematic.active) {
                rememberViewerCamera();
            }
            if (!viewer.active && !viewer.targetActive && viewerSnapshot && viewer.phase == ViewerPhase::FadeIn) {
                ui.cinematic = beforeViewer;
                ui.cinematic.began += clock - viewerBegan;
                ui.cinematic.lightBegan += clock - viewerBegan;
                viewerSnapshot = false;
            }
            scroll += (scrollTarget() - scroll) * float(1 - std::exp(-seconds * 14));
            const float target = (page == Page::Paint || page == Page::Livery || page == Page::Extras) ? 1.f : 0.f;
            paintMix += (target - paintMix) * float(1 - std::exp(-seconds * 18));
        }

        void select(size_t index, const std::vector<Entry>& cars)
        {
            if (!open || busy() || index >= cars.size() || index == selected) {
                return;
            }
            count = cars.size();
            selected = index;
            twoWheelerSelected = twoWheelerPreview(cars[index]);
            selectionAdvancePending = skipSelectionExtras = false;
            shop = {};
            shopExitArmed = shopReturnNavigation = false;
            photo = {};
            shopResetGroup = -1;
            viewer.reset(clock);
            viewerSnapshot = postSelectionCinematic = false;
            spawnReview = SpawnReview::None;
            appearanceReady = true;
            resetVehicleAppearance();
            changed = motionBegan = clock;
            status.clear();
            cue = Cue::Slider;
        }

        void exit()
        {
            if (!open || busy()) {
                return;
            }
            spawn = false;
            dragging = false;
            searching = false;
            fade.request(CloseAction::Exit, clock, blackAlpha());
            cue = Cue::Back;
        }

        void pointerMove(float dx, float dy, float width, float height)
        {
            if (!std::isfinite(dx) || !std::isfinite(dy) || width <= 0 || height <= 0) {
                return;
            }
            ui.mouse(clock, dx, dy);
            pointerX = std::clamp(pointerX + dx / width, 0.f, 1.f);
            pointerY = std::clamp(pointerY + dy / height, 0.f, 1.f);
        }

        void move(int dir, const std::vector<Entry>& cars)
        {
            if (!open || busy() || cars.empty() || !dir || page == Page::Prize) {
                return;
            }
            size_t* value = page == Page::Cars ? &selected : page == Page::Paint ? &paint : &livery;
            const size_t n = page == Page::Cars    ? cars.size()
                             : page == Page::Paint ? std::max(size_t(1), cars[selected].paints.size())
                                                   : std::max(size_t(1), liveries.size());
            const auto next = size_t(((int64_t(*value) + dir) % int64_t(n) + int64_t(n)) % int64_t(n));
            if (next == *value) {
                return;
            }
            *value = next;
            ui.dock = true;
            if (page == Page::Cars) {
                count = cars.size();
                changed = motionBegan = clock;
                paint = livery = 0;
                status.clear();
            }
            cue = page == Page::Cars ? Cue::Slider : Cue::Move;
        }

        void accept(const std::vector<Entry>& cars)
        {
            if (plateOpen) {
                return;
            }
            if (!open || busy() || cars.empty()) {
                return;
            }
            if (spawnReview != SpawnReview::None) {
                chooseSpawnReview(static_cast<ReviewChoice>(reviewChoice));
                return;
            }
            if (page == Page::Prize) {
                if (prizeReady && clock - prizeBegan >= prizeReadyTime) {
                    spawn = true;
                    fade.request(CloseAction::Spawn, clock, blackAlpha());
                    cue = Cue::Spawn;
                }
                return;
            }
            if (selectionAdvancePending) { return; }
            lastCompletedSelection = page;
            if (page == Page::Cars) {
                selectionHasColors = !cars[selected].paints.empty();
                if (selectionHasColors) {
                    setSelectionPage(Page::Paint);
                } else if (liveries.size() > 1) {
                    setSelectionPage(Page::Livery);
                } else {
                    reviewSelectedAppearance();
                }
                cue = Cue::Accept;
            } else if (page == Page::Paint && liveries.size() > 1) {
                const auto& paints = cars[selected].paints;
                if (paints.empty() || paints[std::min(paint, paints.size() - 1)].ids[0] != 1) {
                    spawnReview = SpawnReview::LiveryWarning;
                    review = {clock};
                    pendingReviewChoice = -1;
                    reviewChoice = 0;
                } else {
                    page = Page::Livery;
                    stripChanged = clock;
                }
                cue = Cue::Accept;
            } else if (page == Page::Extras) {
                ui.menu.sliderDrag = -1;
                openReview(SpawnReview::Initial);
                cue = Cue::Accept;
            } else {
                reviewSelectedAppearance();
            }
        }

        void back()
        {
            if (plateOpen) {
                finishPlate(false);
                return;
            }
            if (!open || busy()) {
                return;
            }
            if (selectionAdvancePending) {
                selectionAdvancePending = false;
                cue = Cue::Back;
                return;
            }
            if (spawnReview != SpawnReview::None) {
                if (pendingReviewChoice < 0) {
                    chooseSpawnReview(spawnReview == SpawnReview::Cinematic
                                          ? ReviewChoice::BackToShowroom : ReviewChoice::Dismiss);
                }
                cue = Cue::Back;
                return;
            }
            if (ui.menu.open) {
                ui.menu.open = false;
                return;
            }
            if (shopReturnNavigation && !shop.open()) {
                returnToCarSelection();
                cue = Cue::Back;
                return;
            }
            if (ui.cinematic.active) {
                spawnReview = viewer.active ? SpawnReview::Viewer : SpawnReview::Cinematic;
                review = {clock};
                pendingReviewChoice = -1;
                reviewChoice = 0;
                cue = Cue::Back;
                return;
            }
            if (page == Page::Cars && ui.favoritesOnly) {
                ui.transition.request(StudioAction::AllCars, clock);
                cue = Cue::Back;
                return;
            }
            spawn = false;
            dragging = false;
            if (page == Page::Prize) {
                leavePrize();
                stripChanged = clock;
                cue = Cue::Back;
                return;
            }
            if (page == Page::Extras) {
                setSelectionPage(extrasFrom);
                if (page == Page::Paint) { colorsReturned = clock; }
                cue = Cue::Back;
            } else if (page == Page::Livery) {
                setSelectionPage(selectionHasColors ? Page::Paint : Page::Cars);
                colorsReturned = clock;
                cue = Cue::Back;
            } else if (page == Page::Paint) {
                setSelectionPage(Page::Cars);
                cue = Cue::Back;
            } else {
                exit();
            }
        }

        void favorite(std::vector<Entry>& cars)
        {
            if (open && !busy() && page != Page::Prize && !cars.empty()) {
                cars[selected].favorite = !cars[selected].favorite;
                cue = cars[selected].favorite ? Cue::Favorite : Cue::Back;
            }
        }

        float carAlpha(double age) const
        {
            if (enthusia) {
                return ease(float(age / .3));
            }
            return smoothFade(float((age - .12) / 1.30));
        }

        float carAlpha() const
        {
            return carAlpha(clock - changed);
        }

        float scale(double age) const
        {
            if (enthusia) {
                return 1.f;
            }
            // the retreat starts with the reveal but settles later, like a camera dolly easing to a stop.
            const float progress = std::clamp(float((age - .12) / 1.55), 0.f, 1.f);
            const float remaining = 1.f - progress;
            return 1.f + .95f * remaining * remaining * remaining;
        }

        float scale() const
        {
            return scale(clock - changed);
        }

        float heading(double age, bool twoWheeler = false) const
        {
            constexpr float frontThreeQuarter = -.59f;
            return float(age) * 2 * pi / 20.f + (twoWheeler ? frontThreeQuarter : .16f);
        }

        float heading() const
        {
            return heading(clock - changed, twoWheelerSelected);
        }

        float titleMix() const
        {
            constexpr double hold = 5., transition = .3;
            const double phase = std::fmod(std::max(0., clock - changed), 2 * (hold + transition));
            if (phase < hold) {
                return 0;
            }
            if (phase < hold + transition) {
                return ease(float((phase - hold) / transition));
            }
            if (phase < 2 * hold + transition) {
                return 1;
            }
            return 1 - ease(float((phase - 2 * hold - transition) / transition));
        }

        float opening() const
        {
            return smoothFade(float((clock - opened) / MenuFade::entryDuration));
        }

        float blackAlpha() const
        {
            return std::max({fade.action != CloseAction::None ? fade.opacity(clock) : 1 - opening(),
                             ui.transition.alpha(clock),
                             viewer.opacity(clock)});
        }
    };

    struct Rect
    {
        float x = 0, y = 0, w = 0, h = 0;

        bool contains(float px, float py) const
        {
            return px >= x && py >= y && px < x + w && py < y + h;
        }
    };

    struct Carousel
    {
        float width, height, scale, cell, left, y;
        int first, last;
        const Model& model;

        Carousel(float w, float h, const Model& s, size_t n)
            : width(w),
              height(h),
              scale(h / 720.f),
              cell(1.30f * std::max(w / 18.5f, std::min(100 * scale, (w - 140 * scale) / 11.5f))),
              left((w - 8.38f * cell) * .5f),
              y(h * .725f + 30 * scale * s.paintMix),
              first(std::max(0, int(std::floor(s.scroll - left / cell)) - 2)),
              last(std::min(int(n), int(std::ceil(s.scroll + (w - left) / cell)) + 2)),
              model(s)
        {
            while (first < last && box(first).x + box(first).w <= 0) {
                ++first;
            }
            while (last > first && box(last - 1).x >= w) {
                --last;
            }
        }

        Rect box(int i) const
        {
            const bool selected = size_t(i) == model.selected;
            return {left + (i - model.scroll) * cell + (size_t(i) > model.selected ? .38f * cell : 0),
                    y - (selected ? 15 : 0) * scale * 1.30f,
                    (selected ? 132 : 94) * cell / 100,
                    (selected ? 61 : 43) * scale * 1.30f};
        }

        int hit(float x, float py) const
        {
            for (int i = first; i < last; ++i) {
                if (box(i).contains(x, py)) {
                    return i;
                }
            }
            return -1;
        }
    };

    struct BrowseBar
    {
        Rect track, thumb, hit;
        float travel = 0;

        BrowseBar(float w, float h, const Model& s, size_t count)
        {
            const float scale = h / 720.f;
            track = {66 * scale, h * .897f, std::max(0.f, w - 132 * scale), 4 * scale};
            thumb.w = std::min(track.w,
                               std::max(40 * scale, track.w * std::min(1.f, 8.f / float(std::max(size_t(1), count)))));
            thumb.h = 3 * scale;
            travel = track.w - thumb.w;
            thumb.x = track.x + travel * (count > 1 ? float(s.selected) / float(count - 1) : 0);
            thumb.y = track.y;
            hit = {track.x, track.y - 10 * scale, track.w, 24 * scale};
        }

        size_t index(float x, float grip, size_t count) const
        {
            return count > 1 && travel > 0
                       ? size_t(std::lround(std::clamp((x - track.x - grip) / travel, 0.f, 1.f) * float(count - 1)))
                       : 0;
        }
    };

    inline Rect paintBox(float w, float h, size_t i, size_t count, float styleMix = 0)
    {
        const float scale = h / 720.f;
        const float pitch = std::min((40.f + 54.f * styleMix) * scale,
                                     w * .93f / float(std::max(size_t(1), count)));
        const float width = std::min((24.f + 58.f * styleMix) * scale, pitch * (.80f + .10f * styleMix));
        const float height = 64.f * scale + (width - 64.f * scale) * styleMix;
        return {w * .5f + (float(i) - (float(count) - 1) * .5f) * pitch - width * .5f,
                h * .724f + (64.f * scale - height) * .5f - 3.f * scale * styleMix, width, height};
    }

    inline Rect liveryBox(float w, float h, size_t i, size_t count)
    {
        return paintBox(w, h, i, count);
    }

    inline void pointerClick(Model& model, const std::vector<Entry>& cars, float w, float h)
    {
        if (!model.open || model.busy() || cars.empty()) {
            return;
        }
        const float x = model.pointerX * w, y = model.pointerY * h;
        if (model.page == Page::Cars) {
            const Carousel layout(w, h, model, cars.size());
            const int hit = layout.hit(x, y);
            if (hit >= 0) {
                if (size_t(hit) == model.selected) {
                    model.accept(cars);
                } else {
                    model.select(size_t(hit), cars);
                }
            }
        } else if (model.page == Page::Prize) {
            if (Rect{(w - prizeButtonWidth(w, h)) * .5f,
                     h * prizeButtonTop,
                     prizeButtonWidth(w, h),
                     h * prizeButtonHeight}
                    .contains(x, y)) {
                model.accept(cars);
            }
        } else if (model.page == Page::Livery) {
            for (size_t i = 0; i < model.liveries.size(); ++i) {
                if (liveryBox(w, h, i, model.liveries.size()).contains(x, y)) {
                    if (model.livery == i) {
                        model.accept(cars);
                    } else {
                        model.livery = i;
                        model.cue = Cue::Move;
                    }
                    return;
                }
            }
        } else {
            const size_t n = std::max(size_t(1), cars[model.selected].paints.size());
            for (size_t i = 0; i < n; ++i) {
                if (paintBox(w, h, i, n, model.prologueMix).contains(x, y)) {
                    if (model.paint == i) {
                        model.accept(cars);
                    } else {
                        model.paint = i;
                        model.cue = Cue::Move;
                    }
                    return;
                }
            }
        }
    }

    inline void pointerButton(Model& model, const std::vector<Entry>& cars, float w, float h, bool down, bool pressed)
    {
        if (!model.open || model.busy() || cars.empty()) {
            return;
        }
        const float x = model.pointerX * w, y = model.pointerY * h;
        if (!down || model.page != Page::Cars) {
            model.dragging = false;
        }
        if (model.page == Page::Cars) {
            const BrowseBar bar(w, h, model, cars.size());
            if (pressed && bar.hit.contains(x, y)) {
                model.dragging = true;
                model.dragGrip =
                    (x >= bar.thumb.x && x <= bar.thumb.x + bar.thumb.w) ? x - bar.thumb.x : bar.thumb.w * .5f;
            }
            if (model.dragging) {
                model.select(bar.index(x, model.dragGrip, cars.size()), cars);
                model.scroll = model.scrollTarget();
                return;
            }
        }
        if (pressed) {
            pointerClick(model, cars, w, h);
        }
    }

    inline void dockPointer(Model& s, const std::vector<Entry>& cars, float w, float h, double dt)
    {
        if (!s.open || s.dragging || !s.ui.dock || w <= 0 || h <= 0) {
            return;
        }
        float x = .5f, y = .79f;
        if (s.photo.active) {
            if (s.photo.hidden || s.photo.dragging || s.photo.movingPanel) {
                return;
            }
            const auto box = s.photo.pointerTarget(w, h);
            x = (box.x + box.w * .5f) / w;
            y = (box.y + box.h * .5f) / h;
        } else if (s.plateOpen) {
            const PlatePanel panel(w, h, s.ui.keyboard.open, s.plateMotion.offset(s.clock, h), s.plateReturn != SpawnReview::None);
            const auto box = s.ui.keyboard.open ? s.ui.keyboard.box(w, h, s.ui.keyboard.selected)
                                               : panel.target(s.plateRow);
            x = (box.x + box.w * .5f) / w;
            y = (box.y + box.h * .5f) / h;
        } else if (s.spawnReview != SpawnReview::None) {
            if (s.reviewChoice < 0) {
                return;
            }
            auto button = spawnReviewButton(w, h, s.reviewChoice, s.reviewButtons());
            button.y += s.reviewOffset(h);
            x = (button.x + button.w * .5f) / w;
            y = (button.y + button.h * .5f) / h;
        } else if (s.shop.open()) {
            const auto& shop = s.shop;
            if (shop.locked() || shop.stripDrag || shop.liftDrag) {
                return;
            }
            if (shop.partReview) {
                if (shop.reviewChoice < 0) {
                    return;
                }
                auto button = spawnReviewButton(w, h, shop.reviewChoice, 2);
                button.y += shop.review.offset(s.clock, h);
                x = (button.x + button.w * .5f) / w;
                y = (button.y + button.h * .5f) / h;
            } else if (shop.page == ShopPage::Menu) {
                const auto section = size_t(std::clamp(shop.section, 0, int(shopSections.size()) - 1));
                x = shopIconX[section];
                y = shopIconY[section];
            } else if (shop.page == ShopPage::Wheels || shop.page == ShopPage::Components) {
                x = .5f;
                y = .751f;
            } else if (shop.page == ShopPage::Suspension) {
                const float amount = float(shop.rideHeight - shopLiftMin) / (shopLiftMax - shopLiftMin);
                x = .91f;
                y = shopLiftBottom - std::clamp(amount, 0.f, 1.f) * (shopLiftBottom - shopLiftTop);
            } else if (shop.page == ShopPage::CarWash) {
                x = .58f;
                y = .838f;
            } else {
                return;
            }
        } else if (s.ui.menu.open) {
            if (s.ui.menu.monitorDragLevel >= 0) {
                return;
            }
            const auto& menu = s.ui.menu;
            if (menu.monitorFooter < 0 && menu.focus() == 1 && menu.sliderWindow()) {
                const int row = menu.current(1);
                const auto track = menu.monitorSlider(row, w, h);
                const float amount = float(menu.sliderValue(row)) / std::max(1, menu.sliderMaximum(row));
                x = (track.x + track.w * amount) / w;
                y = (track.y + track.h * .5f) / h;
            } else {
                const auto box = menu.pointerTarget(w, h);
                x = (box.x + box.w * (menu.monitorFooter >= 0 ? .5f : .68f)) / w;
                y = (box.y + box.h * .5f) / h;
            }
        } else if (s.searching) {
            if (!s.ui.keyboard.open) {
                if (s.searchRow < 0) {
                    return;
                }
                const SearchPanel panel(w, h, false, s.searchOffset(SearchPanel(w, h, false).scale));
                const auto box = panel.target(s.searchRow, s.searchPicker, s.searchModeCursor == 1);
                x = (box.x + box.w * .5f) / w;
                y = (box.y + box.h * .5f) / h;
            } else {
                const auto b = s.ui.keyboard.box(w, h, s.ui.keyboard.selected);
                x = (b.x + b.w * .65f) / w;
                y = (b.y + b.h * .62f) / h;
            }
        } else if (s.page == Page::Prize) {
            x = .5f;
            y = prizeButtonTop + prizeButtonHeight * .45f;
        } else {
            if (s.ui.cinematic.active || s.ui.cinematic.mix > .01f || cars.empty() || s.selected >= cars.size()) {
                return;
            }
            if (s.page == Page::Cars) {
                const auto b = Carousel(w, h, s, cars.size()).box(int(s.selected));
                x = (b.x + b.w * .68f) / w;
                y = (b.y + b.h * .68f) / h;
            } else if (s.page == Page::Paint) {
                const auto b = paintBox(w, h, s.paint, std::max(size_t(1), cars[s.selected].paints.size()), s.prologueMix);
                x = (b.x + b.w * .65f) / w;
                y = (b.y + b.h * .62f) / h;
            } else if (s.page == Page::Livery) {
                const auto b = liveryBox(w, h, s.livery, s.liveries.size());
                x = (b.x + b.w * .65f) / w;
                y = (b.y + b.h * .62f) / h;
            } else {
                return;
            }
        }
        const float a = float(1 - std::exp(-std::max(0., dt) * 14));
        s.pointerX += (std::clamp(x, 0.f, 1.f) - s.pointerX) * a;
        s.pointerY += (std::clamp(y, 0.f, 1.f) - s.pointerY) * a;
        if (std::abs((s.pointerX - x) * w) + std::abs((s.pointerY - y) * h) < .3f &&
            std::abs(s.scrollTarget() - s.scroll) < .001f) {
            s.ui.dock = false;
        }
    }

    struct Camera
    {
        static constexpr V3 origin = {100000, 100000, 1000};
        V3 eye = {0, -12, 3.4f}, right = {1, 0, 0}, up, forward;
        float focal = 1, w = 1280, h = 720;

        Camera(float width, float height, float span, float centerY = .48f)
            : w(width),
              h(height)
        {
            eye = {0, -span * 2.2f, span * .57f};
            const V3 target = {0, 0, span * .11f};
            forward = unit(target - eye);
            right = unit(cross(forward, {0, 0, 1}));
            up = cross(right, forward);
            focal = h * 1.40f;
            cy = h * centerY;
        }

        float cy = 0, offsetX = 0, offsetY = 0;

        bool project(V3 p, float& x, float& y, float& z, float& rhw) const
        {
            const V3 d = p - eye;
            const float depth = dot(d, forward);
            if (depth <= .1f) {
                return false;
            }
            rhw = 1 / depth;
            x = offsetX + w * .5f + dot(d, right) * focal * rhw;
            y = offsetY + cy - dot(d, up) * focal * rhw;
            z = 1.f - .1f * rhw;
            return true;
        }
    };
}
