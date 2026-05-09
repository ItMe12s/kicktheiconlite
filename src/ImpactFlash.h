#pragma once

#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include "OverlayEffectState.h"

namespace impact_flash {

// Hitstop / cooldown timers and full-screen flash backdrop mode (white vs color invert).

void decrementCooldown(overlay_effects::ImpactFlashState& state, float dt);
void decrementWhiteFlash(overlay_effects::ImpactFlashState& state, float dt);
overlay_rendering::ImpactFlashMode currentMode(overlay_effects::ImpactFlashState const& state);
void updateFlashBackdrop(
    overlay_rendering::ImpactFlashMode mode,
    cocos2d::CCDrawNode* backdrop,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode& lastDrawnMode
);

} // namespace impact_flash
