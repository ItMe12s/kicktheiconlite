#include "ImpactFlash.h"

#include "ModTuning.h"

#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include <algorithm>

namespace impact_flash {

namespace {

void decrementPositiveRemain(float& slot, float dt) {
    if (slot > 0.0f) {
        slot -= dt;
        slot = std::max(0.0f, slot);
    }
}

} // namespace

void decrementCooldown(overlay_effects::ImpactFlashState& state, float dt) {
    decrementPositiveRemain(state.impactFlashCooldownRemaining, dt);
}

void decrementWhiteFlash(overlay_effects::ImpactFlashState& state, float dt) {
    decrementPositiveRemain(state.whiteFlashRemaining, dt);
}

overlay_rendering::ImpactFlashMode currentMode(overlay_effects::ImpactFlashState const& state) {
    if (state.whiteFlashRemaining <= 0.0f) {
        return overlay_rendering::ImpactFlashMode::None;
    }
    float const elapsed = kImpactFlashTotalSeconds - state.whiteFlashRemaining;
    if (elapsed < kImpactFlashPhaseSeconds) {
        return overlay_rendering::ImpactFlashMode::WhiteSilhouette;
    }
    if (elapsed < static_cast<float>(kImpactFlashInvertPhaseEndPhaseCount) * kImpactFlashPhaseSeconds) {
        return overlay_rendering::ImpactFlashMode::InvertSilhouette;
    }
    return overlay_rendering::ImpactFlashMode::WhiteSilhouette;
}

void updateFlashBackdrop(
    overlay_rendering::ImpactFlashMode mode,
    cocos2d::CCDrawNode* backdrop,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode& lastDrawnMode
) {
    if (!backdrop) {
        return;
    }
    if (mode == overlay_rendering::ImpactFlashMode::None) {
        backdrop->setVisible(false);
        lastDrawnMode = overlay_rendering::ImpactFlashMode::None;
        return;
    }
    if (lastDrawnMode != mode) {
        backdrop->clear();
        cocos2d::ccColor4F const fill = mode == overlay_rendering::ImpactFlashMode::WhiteSilhouette
            ? cocos2d::ccc4f(0, 0, 0, 1)
            : cocos2d::ccc4f(1, 1, 1, 1);
        cocos2d::ccColor4F const border = cocos2d::ccc4f(0, 0, 0, 0);
        cocos2d::CCRect const rect(0.0f, 0.0f, winSize.width, winSize.height);
        backdrop->drawRect(rect, fill, 0.0f, border);
        lastDrawnMode = mode;
    }
    backdrop->setVisible(true);
}

} // namespace impact_flash
