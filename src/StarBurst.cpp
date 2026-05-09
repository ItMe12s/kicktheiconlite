#include "StarBurst.h"

#include <Geode/Geode.hpp>

#include <Geode/utils/random.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include "ModTuning.h"

using namespace geode::prelude;

namespace star_burst {

void clampStarBurstCountsInPlace() {
    constexpr int cap = kStarBurstSpriteSlots;
    int nBig = std::clamp(kBigStarCount, 0, cap);
    int nSmall = std::clamp(kSmallStarCount, 0, cap);
    if (nBig + nSmall > cap) {
        nSmall = std::max(0, cap - nBig);
    }
    kBigStarCount = nBig;
    kSmallStarCount = nSmall;
}

namespace {

int computeCurrentPhase(float whiteFlashRemaining) {
    if (whiteFlashRemaining <= 0.0f) {
        return -1;
    }
    float const elapsed = kImpactFlashTotalSeconds - whiteFlashRemaining;
    int const phase = static_cast<int>(elapsed / kImpactFlashPhaseSeconds);
    return std::min(phase, kStarBurstMaxPhaseIndex);
}

void applyTint(overlay_effects::StarBurstState& state, overlay_rendering::ImpactFlashMode flashMode) {
    bool const whiteBackdrop = flashMode == overlay_rendering::ImpactFlashMode::InvertSilhouette;
    cocos2d::ccColor3B const tint = whiteBackdrop ? ccc3(0, 0, 0) : ccc3(255, 255, 255);
    for (auto* sprite : state.sprites) {
        if (sprite) {
            sprite->setColor(tint);
        }
    }
}

void layoutBurstSprite(
    cocos2d::CCSprite* sprite,
    float screenSmaller,
    float angle,
    float radius,
    float screenFrac
) {
    if (!sprite) {
        return;
    }
    float const cw = sprite->getContentSize().width;
    float const baseScale = cw > 0.0f ? (screenSmaller * screenFrac) / cw : 1.0f;
    sprite->setPosition({std::cos(angle) * radius, std::sin(angle) * radius});
    sprite->setRotation(0.0f);
    sprite->setScale(
        baseScale
        * (1.0f + geode::utils::random::generate<float>(-kStarScaleVariance, kStarScaleVariance))
    );
    sprite->setVisible(true);
}

void reposition(overlay_effects::StarBurstState& state, cocos2d::CCSize winSize, overlay_rendering::ImpactFlashMode flashMode) {
    clampStarBurstCountsInPlace();

    float const screenSmaller = winSize.width < winSize.height ? winSize.width : winSize.height;
    float const twoPi = 2.0f * std::numbers::pi_v<float>;

    int const cap = kStarBurstSpriteSlots;
    int const nBig = std::clamp(kBigStarCount, 0, cap);
    int const nSmall = std::clamp(kSmallStarCount, 0, cap);
    float const bigDivisor = std::max(1, nBig);

    for (int i = 0; i < nBig; ++i) {
        auto* sprite = state.sprites[static_cast<size_t>(i)];
        float const sector =
            (static_cast<float>(i) + geode::utils::random::generate<float>(0.0f, 1.0f)) / static_cast<float>(bigDivisor);
        float const angle = sector * twoPi;
        float const radius = geode::utils::random::generate<float>(
            screenSmaller * kBigStarRadiusMin,
            screenSmaller * kBigStarRadiusMax
        );
        layoutBurstSprite(sprite, screenSmaller, angle, radius, kBigStarScreenFrac);
    }

    for (int i = 0; i < nSmall; ++i) {
        auto* sprite = state.sprites[static_cast<size_t>(nBig + i)];
        float const angle = geode::utils::random::generate<float>(0.0f, twoPi);
        float const radius = geode::utils::random::generate<float>(
            screenSmaller * kSmallStarRadiusMin,
            screenSmaller * kSmallStarRadiusMax
        );
        layoutBurstSprite(sprite, screenSmaller, angle, radius, kSmallStarScreenFrac);
    }

    applyTint(state, flashMode);
}

} // namespace

void createSprites(overlay_effects::StarBurstState& state) {
    if (!state.layer) {
        return;
    }
    for (int i = 0; i < kStarBurstSpriteSlots; ++i) {
        auto* star = cocos2d::CCSprite::create("img_star1.png"_spr);
        if (!star) {
            continue;
        }
        star->setID(std::string(GEODE_MOD_ID) + "/star-burst-" + std::to_string(i));
        star->setVisible(false);
        star->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        state.layer->addChild(star, kStarBurstZOrder);
        state.sprites[static_cast<size_t>(i)] = star;
    }
}

void hideAll(overlay_effects::StarBurstState& state) {
    for (auto* sprite : state.sprites) {
        if (!sprite) {
            continue;
        }
        sprite->setVisible(false);
        sprite->setColor(ccc3(255, 255, 255));
    }
}

void reset(overlay_effects::StarBurstState& state) {
    hideAll(state);
    state.phaseIndex = -1;
}

void update(
    overlay_effects::StarBurstState& state,
    float whiteFlashRemaining,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode flashMode
) {
    int const newPhase = computeCurrentPhase(whiteFlashRemaining);
    if (newPhase < 0) {
        if (state.phaseIndex >= 0) {
            hideAll(state);
            state.phaseIndex = -1;
        }
        return;
    }
    if (newPhase != state.phaseIndex) {
        state.phaseIndex = newPhase;
        reposition(state, winSize, flashMode);
    } else {
        applyTint(state, flashMode);
    }
}

} // namespace star_burst
