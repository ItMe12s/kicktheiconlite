#pragma once

#include "OverlayEffectState.h"
#include "PlayerVisual.h"

namespace sandevistan_trail {

// After strong impacts, spawn semi-transparent icon ghosts on the trail layer until speed drops.

void stopIfSlowOrGrab(overlay_effects::SandevistanTrailState& state, bool grabActive, float playerSpeedPx);
void updateAndSpawn(
    overlay_effects::SandevistanTrailState& state,
    cocos2d::CCNode* playerRoot,
    SimplePlayer* player,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float dt
);

} // namespace sandevistan_trail
