#include "overlay-rendering/OverlayRenderingInternal.h"
#include "OverlayRendering.h"
#include "ModTuning.h"
#include "PhysicsTypes.h"

#include <Geode/binding/GameManager.hpp>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace overlay_rendering {

FireAuraAttachResult attachFireAura(CCNode* playerRoot, float auraDiameterPx) {
    FireAuraAttachResult out{};
    if (!playerRoot || auraDiameterPx <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = detail::createOneByOneWhiteTexture();
    if (!tex) {
        return out;
    }

    GLint locVelocity = -1;
    GLint locTime = -1;
    GLint locIntensity = -1;
    GLint locColorPrimary = -1;
    GLint locColorSecondary = -1;
    CCGLProgram* program = createFireAuraProgram(
        &locVelocity,
        &locTime,
        &locIntensity,
        &locColorPrimary,
        &locColorSecondary
    );
    if (!program || locVelocity < 0 || locTime < 0 || locIntensity < 0 || locColorPrimary < 0
        || locColorSecondary < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    OverlayShaderSprite* sprite = OverlayShaderSprite::createFireAura(
        tex,
        program,
        locVelocity,
        locTime,
        locIntensity,
        locColorPrimary,
        locColorSecondary
    );
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setID("fire-aura-sprite"_spr);
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    float const cw = sprite->getContentSize().width;
    sprite->setScale(cw > 0.0f ? auraDiameterPx / cw : auraDiameterPx);
    sprite->setPosition({0, 0});
    sprite->setVisible(false);
    playerRoot->addChild(sprite, kFireAuraZOrder);

    out.ok = true;
    out.sprite = sprite;
    out.program = program;
    return out;
}

void refreshFireAura(FireAuraRefreshArgs const& args) {
    OverlayShaderSprite* const fireAura = args.fireAura;
    float const dt = args.dt;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;
    float* const fireTime = args.fireTime;

    if (!fireAura || !fireTime) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    if (impactFlashActive) {
        fireAura->setVisible(false);
        return;
    }

    PhysicsVelocity const vel = args.playerVelocity;
    float const speed = std::hypot(vel.vx, vel.vy);
    float const lo = std::min(kMinFireAuraSpeedPx, kMaxFireAuraSpeedPx);
    float const hi = std::max(kMinFireAuraSpeedPx, kMaxFireAuraSpeedPx);
    float const denom = hi - lo;
    float intensity = 0.0f;
    if (denom > 1e-5f && speed > lo) {
        intensity = (speed - lo) / denom;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
    }

    if (intensity <= 0.0f) {
        fireAura->setVisible(false);
        return;
    }

    *fireTime += dt;
    float const tWrapped = std::fmod(*fireTime, 1000.0f);

    float const vx = vel.vx * kFireAuraVelocityToShader;
    float const vy = vel.vy * kFireAuraVelocityToShader;

    ccColor3B primaryRgb = {255, 255, 255};
    ccColor3B secondaryRgb = {200, 200, 200};
    if (GameManager* gm = GameManager::get()) {
        primaryRgb = gm->colorForIdx(gm->getPlayerColor());
        secondaryRgb = gm->colorForIdx(gm->getPlayerColor2());
    }
    fireAura->setFireColors(primaryRgb, secondaryRgb);
    fireAura->setFireState(vx, vy, tWrapped, intensity);
    fireAura->setVisible(true);
}

} // namespace overlay_rendering
