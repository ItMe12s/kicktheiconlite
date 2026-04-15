#include "FireAura.h"

namespace vfx::fire_aura {

void update(
    FireAuraState& state,
    PhysicsWorld* physics,
    float dt,
    overlay_rendering::ImpactFlashMode impactFlashMode
) {
    overlay_rendering::refreshFireAura({
        .fireAura = state.sprite,
        .physics = physics,
        .dt = dt,
        .impactFlashMode = impactFlashMode,
        .fireTime = &state.time,
    });
}

} // namespace vfx::fire_aura
