#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace gt4
{
    enum class ShopPage
    {
        Closed,
        Menu,
        Wheels,
        Components,
        Suspension,
        CarWash
    };

    inline constexpr std::array<ShopPage, 4> shopSections{
        ShopPage::CarWash, ShopPage::Suspension, ShopPage::Wheels, ShopPage::Components};

    inline constexpr std::array<float, 4> shopIconX{.18f, .35f, .52f, .69f};
    inline constexpr std::array<float, 4> shopIconY{.62f, .68f, .73f, .77f};
    inline constexpr float shopTrackLeft = .052f, shopTrackRight = .948f, shopTrackY = .904f;

    inline constexpr float shopLiftTop = .19f, shopLiftBottom = .70f;
    inline constexpr int shopLiftMin = -1, shopLiftMax = 2;

    struct ShopPart
    {
        int group, option;
    };

    template <class Groups>
    std::vector<ShopPart> shopParts(const Groups& groups, bool wheels)
    {
        constexpr std::array<int, 7> bodySlots{6, 44, 45, 0, 14, 8, 9};
        std::vector<ShopPart> parts;
        const auto addSlot = [&](int slot) {
            for (size_t group = 0; group < groups.size(); ++group) {
                if (groups[group].slot == slot) {
                    for (size_t option = 1; option < groups[group].options.size(); ++option) {
                        parts.push_back({int(group), int(option)});
                    }
                }
            }
        };
        if (wheels) {
            addSlot(34);
        } else {
            for (int slot : bodySlots) {
                addSlot(slot);
            }
        }
        return parts;
    }

    struct ReviewMotion
    {
        double began = 0, leaving = -1;
        float from = 1;

        static float smooth(double amount)
        {
            const float t = float(std::clamp(amount, 0., 1.));
            return t * t * (3.f - 2.f * t);
        }

        float reveal(double now) const
        {
            return leaving < 0. ? smooth((now - began) / .28) : from * (1.f - smooth((now - leaving) / .22));
        }

        float offset(double now, float height) const
        {
            const float fraction = leaving < 0. ? 1.f - reveal(now)
                                                : 1.f - from - smooth((now - leaving) / .22);
            return fraction * height * .047f;
        }

        void dismiss(double now)
        {
            from = reveal(now);
            leaving = now;
        }

        bool finished(double now) const
        {
            return leaving >= 0. && now - leaving >= .22;
        }
    };

    inline const char* shopTitle(ShopPage page)
    {
        switch (page) {
            case ShopPage::Wheels:
                return "Wheels";
            case ShopPage::Components:
                return "Components";
            case ShopPage::Suspension:
                return "Suspension";
            case ShopPage::CarWash:
                return "Car Wash";
            default:
                return "TransFender";
        }
    }

    inline int shopPartPrice(int slot)
    {
        switch (slot) {
            case 34:
                return 620;
            case 44:
            case 45:
                return 1000;
            case 6:
                return 800;
            case 16:
                return 1500;
            case 17:
                return 100;
            default:
                return 500;
        }
    }

    inline std::string shopMoney(int amount)
    {
        std::string result = std::to_string(std::max(0, amount));
        for (int pos = int(result.size()) - 3; pos > 0; pos -= 3) {
            result.insert(size_t(pos), 1, ',');
        }
        return result;
    }

    struct TransFender
    {
        static constexpr double transitionSeconds = 2.;
        static constexpr double washSpinSeconds = 8.5;
        static constexpr double washRevolutions = 10.;
        static constexpr double washSeconds = 14.5;
        static constexpr int suspensionPrice = 450;
        static constexpr int washPrice = 50;

        ShopPage page = ShopPage::Closed, destination = ShopPage::Closed;
        double transitionStart = -1., pageStart = 0., washStart = -1.;
        bool midpoint = false, stripDrag = false, liftDrag = false, fromViewer = false;
        double transitionLength = transitionSeconds;
        int section = 0, option = 0;
        std::vector<ShopPart> parts;
        bool partReview = false;
        int reviewChoice = 0;
        ReviewMotion review;
        std::array<int, 32> confirmedParts{};
        int balance = 0, partsCost = 0;
        int rideHeight = 0, confirmedRideHeight = 0;
        bool washed = false;
        std::string status;

        bool open() const
        {
            return page != ShopPage::Closed || transitionStart >= 0.;
        }

        bool locked() const
        {
            return transitionStart >= 0. || washStart >= 0.;
        }

        int pendingCost() const
        {
            return partsCost + (confirmedRideHeight ? suspensionPrice : 0) + (washed ? washPrice : 0);
        }

        void request(ShopPage next, double now)
        {
            if (locked() || next == page) {
                return;
            }
            destination = next;
            transitionStart = now;
            midpoint = false;
            stripDrag = liftDrag = false;
            transitionLength = transitionSeconds;
            if (next == ShopPage::Wheels || next == ShopPage::Components || next == ShopPage::CarWash) {
                transitionLength = 1.;
            }
            status.clear();
        }

        void step(double now)
        {
            if (transitionStart >= 0.) {
                const double elapsed = now - transitionStart;
                if (!midpoint && elapsed >= transitionLength / 2.) {
                    page = destination;
                    pageStart = now;
                    midpoint = true;
                }
                if (elapsed >= transitionLength) {
                    transitionStart = -1.;
                }
            }
            if (partReview && review.finished(now)) {
                partReview = false;
            }
            if (washStart >= 0. && now - washStart >= washSeconds) {
                washStart = -1.;
                washed = true;
                status = "Wash complete.";
            }
        }

        float fade(double now) const
        {
            if (transitionStart < 0.) {
                return 0.f;
            }
            const double age = now - transitionStart;
            const double half = transitionLength * .5;
            const float progress = float(std::clamp(std::min(age, transitionLength - age) / half, 0., 1.));
            return progress * progress * (3.f - 2.f * progress);
        }

        float reveal(double now) const
        {
            const double elapsed = std::max(0., now - pageStart);
            const double duration = page == ShopPage::CarWash ? 1.05 : .55;
            const float amount = float(std::clamp((elapsed - .12) / duration, 0., 1.));
            return amount * amount * (3.f - 2.f * amount);
        }

        float washTurns(double now) const
        {
            if (washStart < 0.) {
                return 0.f;
            }
            const double progress = std::clamp((now - washStart) / washSpinSeconds, 0., 1.);
            const double t = progress;
            const double turns = t * t * t * (10. + t * (-15. + 6. * t));
            return float(washRevolutions * turns);
        }

        float washProgress(double now) const
        {
            if (washStart >= 0.) {
                return float(std::clamp((now - washStart) / washSeconds, 0., 1.));
            }
            return washed ? 1.f : 0.f;
        }
    };
}
