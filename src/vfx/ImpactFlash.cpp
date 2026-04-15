#include "ImpactFlash.h"

#include "../PhysicsOverlayTuning.h"

namespace vfx::impact_flash {

void decrementCooldown(ImpactFlashState& state, float dt) {
    if (state.impactFlashCooldownRemaining > 0.0f) {
        state.impactFlashCooldownRemaining -= dt;
        if (state.impactFlashCooldownRemaining < 0.0f) {
            state.impactFlashCooldownRemaining = 0.0f;
        }
    }
}

void decrementWhiteFlash(ImpactFlashState& state, float dt) {
    if (state.whiteFlashRemaining > 0.0f) {
        state.whiteFlashRemaining -= dt;
        if (state.whiteFlashRemaining < 0.0f) {
            state.whiteFlashRemaining = 0.0f;
        }
    }
}

overlay_rendering::ImpactFlashMode currentMode(ImpactFlashState const& state) {
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

void updateBackdrops(
    overlay_rendering::ImpactFlashMode mode,
    cocos2d::CCDrawNode* blackBackdrop,
    cocos2d::CCDrawNode* whiteBackdrop
) {
    bool const impactFlashActive = mode != overlay_rendering::ImpactFlashMode::None;
    if (blackBackdrop) {
        blackBackdrop->setVisible(impactFlashActive && mode == overlay_rendering::ImpactFlashMode::WhiteSilhouette);
    }
    if (whiteBackdrop) {
        whiteBackdrop->setVisible(impactFlashActive && mode == overlay_rendering::ImpactFlashMode::InvertSilhouette);
    }
}

} // namespace vfx::impact_flash
