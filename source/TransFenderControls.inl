void restoreShopParts()
{
    const auto& groups = state.ui.menu.customizations;
    const auto& confirmed = state.shop.confirmedParts;
    for (size_t row = 0; row < groups.size(); ++row) {
        if (groups[row].choice != confirmed[row]) {
            selectStudioCustomization(int(row), 0);
        }
    }
    for (size_t row = 0; row < groups.size(); ++row) {
        if (groups[row].choice != confirmed[row]) {
            selectStudioCustomization(int(row), confirmed[row]);
        }
    }
}

void resetShopSelection(bool wheels)
{
    restoreShopParts();
    auto& shop = state.shop;
    const auto& groups = state.ui.menu.customizations;
    constexpr std::array<int, 7> bodySlots{6, 44, 45, 0, 14, 8, 9};
    bool complete = true;
    for (size_t row = 0; row < groups.size(); ++row) {
        const int slot = groups[row].slot;
        const bool matches =
            wheels ? slot == 34 : std::find(bodySlots.begin(), bodySlots.end(), slot) != bodySlots.end();
        if (!matches || shop.confirmedParts[row] == 0) {
            continue;
        }
        if (selectStudioCustomization(int(row), 0)) {
            shop.confirmedParts[row] = 0;
        } else {
            complete = false;
        }
    }
    shop.partsCost = 0;
    for (size_t row = 0; row < groups.size(); ++row) {
        if (shop.confirmedParts[row] > 0) {
            shop.partsCost += shopPartPrice(groups[row].slot);
        }
    }
    shop.status = complete ? (wheels ? "Wheels reset to stock" : "Components reset to stock")
                           : "Some parts could not be reset";
    state.cue = complete ? Cue::Accept : Cue::Error;
}

void previewShopPart(int index)
{
    auto& shop = state.shop;
    if (index < 0 || size_t(index) >= shop.parts.size()) {
        return;
    }
    const auto part = shop.parts[size_t(index)];
    restoreShopParts();
    if (selectStudioCustomization(part.group, part.option)) {
        shop.option = index;
        shop.status.clear();
        state.cue = Cue::Move;
    }
}

void processShopInput(
    bool leftPress, bool rightPress, bool upPress, bool downPress, bool accept, bool cancel, bool click, bool held, bool moved, float width, float height)
{
    auto& shop = state.shop;
    if (shop.locked()) {
        return;
    }
    const auto& groups = state.ui.menu.customizations;
    const int horizontal = int(rightPress) - int(leftPress);
    const int vertical = int(downPress) - int(upPress);
    auto moveCue = [&] {
        state.cue = Cue::Move;
    };
    auto fail = [&](const char* message) {
        shop.status = message;
        state.cue = Cue::Error;
    };
    if (shop.partReview) {
        if (shop.review.leaving >= 0.) {
            return;
        }
        if (horizontal || vertical) {
            shop.reviewChoice = 1 - shop.reviewChoice;
            moveCue();
        }
        if (moved || click) {
            for (int i = 0; i < 2; ++i) {
                auto button = spawnReviewButton(width, height, i, 2);
                button.y += shop.review.offset(state.clock, height);
                if (button.contains(state.pointerX * width, state.pointerY * height)) {
                    shop.reviewChoice = i;
                    accept = accept || click;
                    break;
                }
            }
        }
        if (cancel || accept) {
            if (!cancel && shop.reviewChoice == 1) {
                int total = 0;
                for (const auto& selection : groups) {
                    if (selection.choice > 0) {
                        total += shopPartPrice(selection.slot);
                    }
                }
                if (total + shop.pendingCost() - shop.partsCost > shop.balance) {
                    fail("Insufficient funds");
                    return;
                }
                if (!state.appearanceReady) {
                    fail("Wait for the preview to finish loading");
                    return;
                }
                for (size_t row = 0; row < groups.size(); ++row) {
                    shop.confirmedParts[row] = groups[row].choice;
                }
                shop.partsCost = total;
                shop.status = "Selected Charged when you spawn the car";
                state.cue = Cue::Accept;
            } else {
                state.cue = Cue::Back;
            }
            shop.review.dismiss(state.clock);
            blockActions();
        }
        return;
    }
    const bool backArrow = click && state.pointerX >= .86f &&
                           state.pointerY >= .77f && state.pointerY < .92f &&
                           (shop.page == ShopPage::CarWash || shop.page == ShopPage::Suspension);
    if (cancel || backArrow) {
        if (shop.page == ShopPage::Menu) {
            state.spawnReview = SpawnReview::ShopExit;
            state.review = {state.clock};
            state.pendingReviewChoice = -1;
            state.reviewChoice = 0;
        } else {
            if (shop.page == ShopPage::Wheels || shop.page == ShopPage::Components) {
                restoreShopParts();
                shop.parts.clear();
            }
            shop.rideHeight = shop.confirmedRideHeight;
            shop.request(ShopPage::Menu, state.clock);
        }
        state.cue = Cue::Back;
        blockActions();
        return;
    }
    if (shop.page == ShopPage::Menu) {
        if (horizontal || vertical) {
            shop.section = (shop.section + horizontal + vertical + int(shopSections.size())) % int(shopSections.size());
            moveCue();
        }
        if (click || (moved && !horizontal && !vertical)) {
            for (size_t i = 0; i < shopSections.size(); ++i) {
                if (std::abs(state.pointerX - shopIconX[i]) < .087f &&
                    state.pointerY >= shopIconY[i] - .095f && state.pointerY <= shopIconY[i] + .096f) {
                    if (shop.section != int(i)) {
                        shop.section = int(i);
                        moveCue();
                    }
                    accept = accept || click;
                    break;
                }
            }
        }
        if (!accept) {
            return;
        }
        const ShopPage page = shopSections[size_t(shop.section)];
        if (page == ShopPage::Wheels || page == ShopPage::Components) {
            const bool wheels = page == ShopPage::Wheels;
            shop.parts = shopParts(groups, wheels);
            if (shop.parts.empty()) {
                fail(wheels ? "This vehicle has no compatible wheels" : "This vehicle has no compatible components");
                return;
            }
            shop.option = 0;
            previewShopPart(shop.option);
        }
        shop.request(page, state.clock);
        state.cue = Cue::Accept;
        blockActions();
        return;
    }
    if (shop.page == ShopPage::Wheels || shop.page == ShopPage::Components) {
        const int count = int(shop.parts.size());
        if (!count) {
            shop.request(ShopPage::Menu, state.clock);
            return;
        }
        int target = (shop.option + horizontal + count) % count;
        if (!held) {
            shop.stripDrag = false;
        }
        if (click && state.pointerY >= shopTrackY - .012f && state.pointerY < .918f &&
            state.pointerX >= shopTrackLeft && state.pointerX <= shopTrackRight) {
            shop.stripDrag = true;
        }
        if (shop.stripDrag) {
            const float amount = (state.pointerX - shopTrackLeft) / (shopTrackRight - shopTrackLeft);
            target = std::clamp(int(amount * count), 0, count - 1);
        } else if (click && state.pointerY >= .64f && state.pointerY <= .83f) {
            const int tile = std::clamp(int(state.pointerX * 7), 0, 6);
            target = shop.option + tile - 3;
            accept = accept || target == shop.option;
        }
        if (target != shop.option && target >= 0 && target < count) {
            previewShopPart(target);
        }
        if (accept) {
            previewShopPart(shop.option);
            shop.partReview = true;
            shop.review = {state.clock};
            shop.reviewChoice = 0;
            state.cue = Cue::Accept;
            blockActions();
        }
        return;
    }
    if (shop.page == ShopPage::Suspension) {
        if (horizontal || vertical) {
            shop.rideHeight = std::clamp(shop.rideHeight + horizontal - vertical, shopLiftMin, shopLiftMax);
            moveCue();
        }
        if (!held) {
            shop.liftDrag = false;
        }
        if (click && state.pointerX >= .875f && state.pointerX < .953f &&
            state.pointerY >= shopLiftTop && state.pointerY <= shopLiftBottom) {
            shop.liftDrag = true;
        }
        if (shop.liftDrag) {
            const float amount =
                std::clamp((shopLiftBottom - state.pointerY) / (shopLiftBottom - shopLiftTop), 0.f, 1.f);
            const int height = shopLiftMin + int(std::lround(amount * (shopLiftMax - shopLiftMin)));
            if (height != shop.rideHeight) {
                shop.rideHeight = height;
                moveCue();
            }
        }
        if (accept) {
            const int other = shop.pendingCost() - (shop.confirmedRideHeight ? TransFender::suspensionPrice : 0);
            if (shop.rideHeight && other + TransFender::suspensionPrice > shop.balance) {
                fail("Insufficient funds");
            } else {
                shop.confirmedRideHeight = shop.rideHeight;
                shop.status = "Ride height selected";
                state.cue = Cue::Accept;
            }
        }
        return;
    }
    if (shop.page == ShopPage::CarWash &&
        (accept || (click && state.pointerX >= .31f && state.pointerX < .86f &&
                    state.pointerY >= .77f && state.pointerY < .92f))) {
        if (shop.washed) {
            shop.status = "Wash already selected";
        } else if (shop.pendingCost() + TransFender::washPrice > shop.balance) {
            fail("Insufficient funds");
        } else {
            shop.washStart = state.clock;
            shop.status.clear();
            state.cue = Cue::WashSpin;
        }
    }
}
