#pragma once

#include "OverlayEffectState.h"
#include "PlayerVisual.h"

namespace sandevistan_trail {

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
