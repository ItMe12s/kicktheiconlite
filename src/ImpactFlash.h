#pragma once

#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include "OverlayEffectState.h"

namespace impact_flash {

void decrementCooldown(overlay_effects::ImpactFlashState& state, float dt);
void decrementWhiteFlash(overlay_effects::ImpactFlashState& state, float dt);
overlay_rendering::ImpactFlashMode currentMode(overlay_effects::ImpactFlashState const& state);
void updateBackdrops(
    overlay_rendering::ImpactFlashMode mode,
    cocos2d::CCDrawNode* blackBackdrop,
    cocos2d::CCDrawNode* whiteBackdrop
);

} // namespace impact_flash
