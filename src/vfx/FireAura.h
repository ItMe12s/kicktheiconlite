#pragma once

#include "../PhysicsWorld.h"
#include "VfxTypes.h"

namespace vfx::fire_aura {

void update(
    FireAuraState& state,
    PhysicsWorld* physics,
    float dt,
    overlay_rendering::ImpactFlashMode impactFlashMode
);

} // namespace vfx::fire_aura
