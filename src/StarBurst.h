#pragma once

#include "OverlayEffectState.h"

namespace star_burst {

void clampStarBurstCountsInPlace();

void createSprites(overlay_effects::StarBurstState& state);
void hideAll(overlay_effects::StarBurstState& state);
void reset(overlay_effects::StarBurstState& state);
void update(
    overlay_effects::StarBurstState& state,
    float whiteFlashRemaining,
    cocos2d::CCSize winSize,
    overlay_rendering::ImpactFlashMode flashMode
);

} // namespace star_burst
