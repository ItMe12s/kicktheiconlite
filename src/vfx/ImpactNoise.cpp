#include "ImpactNoise.h"

#include "../PhysicsOverlayTuning.h"

namespace vfx::impact_noise {

void update(ImpactNoiseState& state, float dt, bool flashActive) {
    if (!flashActive && state.remaining > 0.0f) {
        state.remaining -= dt;
        if (state.remaining < 0.0f) {
            state.remaining = 0.0f;
        }
    }

    float alpha = state.remaining / kImpactNoiseFadeSeconds;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    bool const visible = !flashActive && state.remaining > 0.0f;
    float const extraSkip = state.extraTimeSkip;
    state.extraTimeSkip = 0.0f;

    overlay_rendering::refreshImpactNoise({
        .sprite = state.sprite,
        .renderTexture = state.renderTexture,
        .compositeSprite = state.composite,
        .dt = dt,
        .extraTimeSkip = extraSkip,
        .time = &state.time,
        .alpha = alpha,
        .visible = visible,
    });
}

} // namespace vfx::impact_noise
