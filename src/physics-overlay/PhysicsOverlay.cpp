#include "PhysicsOverlay.h"

#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>
#include <Geode/cocos/platform/CCEGLViewProtocol.h>

#include <algorithm>
#include <cmath>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "RuntimeRestart.h"
#include "ModTuning.h"
#include "ImpactFlash.h"
#include "SandevistanTrail.h"
#include "StarBurst.h"

using namespace geode::prelude;

namespace {

inline void mergeImpactSnapshot(PhysicsImpactEvent& total, PhysicsImpactEvent const& current) {
    total.triggered = total.triggered || current.triggered;
    total.preSpeedPx = std::max(total.preSpeedPx, current.preSpeedPx);
    total.postSpeedPx = std::max(total.postSpeedPx, current.postSpeedPx);
    total.impactSpeedPx = std::max(total.impactSpeedPx, current.impactSpeedPx);
}

} // namespace

PhysicsOverlay::PhysicsOverlay() = default;
PhysicsOverlay::~PhysicsOverlay() = default;

void PhysicsOverlay::applyHideModOverlayFromTuning() {
    if (m_selfDestructRequested || runtime_restart::isRestartRequired()) {
        return;
    }
    if (kHideModOverlay) {
        endTouchInteraction();
        this->setPosition({kHideModOverlayOffsetX, kHideModOverlayOffsetY});
        this->setTouchEnabled(false);
    } else {
        this->setPosition({0.0f, 0.0f});
        this->setTouchEnabled(true);
        handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
    }
}

void PhysicsOverlay::decrementCooldowns(float dt) {
    impact_flash::decrementCooldown(m_impactFlash, dt);
}

void PhysicsOverlay::tryBuildVisualIfNeeded() {
    if (!m_visualBuilt) {
        tryBuildPlayerVisual();
    }
}

void PhysicsOverlay::stepPhysicsUnlessHitstop(float dt) {
    m_lastPhysicsSubsteps = 0;
    m_lastPlayerImpact = {};

    if (m_impactFlash.hitstopRemaining > 0.0f) {
        m_impactFlash.hitstopRemaining -= dt;
        if (m_impactFlash.hitstopRemaining < 0.0f) {
            m_impactFlash.hitstopRemaining = 0.0f;
        }
        return;
    }

    m_physicsAccumulator += dt;
    if (m_physicsAccumulator > kPhysicsAccumulatorCap) {
        m_physicsAccumulator = kPhysicsAccumulatorCap;
    }

    int substepCount = 0;
    while (m_physicsAccumulator >= kFixedPhysicsDt && substepCount < kMaxPhysicsSubsteps) {
        m_physics->step(kFixedPhysicsDt);

        auto const playerImpact = m_physics->consumePlayerImpactAny();
        mergeImpactSnapshot(m_lastPlayerImpact, playerImpact);
        if (playerImpact.triggered) {
            float const playerImpactIntensityPx = std::max(playerImpact.postSpeedPx, playerImpact.impactSpeedPx);
            if (
                kEnablePlayerImpactTrail
                && playerImpactIntensityPx >= kPlayerImpactMinTrailSpeed
                && !m_grabActive
            ) {
                m_trail.active = true;
                m_trail.spawnAccumulator = kSandevistanSpawnIntervalSec;
            }

            if (
                kEnablePlayerImpactFlashStack
                && !m_grabActive
                && playerImpact.impactSpeedPx >= kPlayerImpactMinFlashSpeed
                && m_impactFlash.impactFlashCooldownRemaining <= 0.0f
            ) {
                if (m_impactNoise.remaining > 0.0f) {
                    m_impactNoise.extraTimeSkip += kImpactNoiseStackedImpactTimeSkip;
                }
                m_impactFlash.hitstopRemaining = kImpactHitstopSeconds;
                m_impactFlash.whiteFlashRemaining = kImpactFlashTotalSeconds;
                m_impactFlash.impactFlashCooldownRemaining = kImpactFlashCooldownSeconds;
                m_impactNoise.remaining = kImpactNoiseFadeSeconds;
                if (m_objectBlur.whiteFlashSprite) {
                    m_objectBlur.whiteFlashSprite->stopAllActions();
                }
            }
        }

        m_physicsAccumulator -= kFixedPhysicsDt;
        ++substepCount;
        m_lastPhysicsSubsteps = substepCount;

        if (m_impactFlash.hitstopRemaining > 0.0f) {
            break;
        }
    }
}

void PhysicsOverlay::syncPlayerNodeFromPhysics() {
    float const alpha = m_physicsAccumulator / kFixedPhysicsDt;
    auto state = m_physics->getPlayerRenderState(alpha);
    m_playerRoot->setPosition({state.x, state.y});
    m_player->setRotation(-state.angle * kRadToDeg);
    if (m_starBurst.layer) {
        m_starBurst.layer->setPosition(m_playerRoot->getPosition());
    }
}

bool PhysicsOverlay::updateEarlyGuards(float& dt) {
    if (m_selfDestructRequested || runtime_restart::isRestartRequired()) {
        return false;
    }

    if (!std::isfinite(dt) || dt <= 0.0f) {
        return false;
    }
    dt = std::min(dt, kMaxSimulationFrameDt);

    if (m_glReadyCheckInitialized) {
        if (auto* view = CCDirector::get()->getOpenGLView()) {
            if (auto* protocol = typeinfo_cast<cocos2d::CCEGLViewProtocol*>(view)) {
                bool const glReady = protocol->isOpenGLReady();
                if (m_lastGlReady && !glReady) {
                    runtime_restart::requestFullscreenSelfDestruct("openGL context became unready");
                    return false;
                }
                m_lastGlReady = glReady;
            }
        }
    }

    if (!m_physics) {
        return false;
    }

    if (kHideModOverlay) {
        return false;
    }

    return true;
}

void PhysicsOverlay::updateSimulationAndDebug(float dt) {
    decrementCooldowns(dt);
    tryBuildVisualIfNeeded();
    stepPhysicsUnlessHitstop(dt);

    m_debugLabelAccumulator += dt;
    if (m_debugLabelAccumulator >= kDebugLabelUpdateInterval) {
        updateDebugOverlayText(m_debugLabelAccumulator);
        m_debugLabelAccumulator = 0.0f;
    }

    sandevistan_trail::stopIfSlowOrGrab(m_trail, m_grabActive, m_physics->getPlayerSpeed());
}

void PhysicsOverlay::updateVisualPipeline(float dt) {
    if (!m_playerRoot || !m_player) {
        impact_flash::decrementWhiteFlash(m_impactFlash, dt);
        return;
    }

    syncPlayerNodeFromPhysics();
    sandevistan_trail::updateAndSpawn(m_trail, m_playerRoot, m_player, m_targetSize, m_frameId, m_iconTypeInt, dt);

    overlay_rendering::ImpactFlashMode const flashMode = impact_flash::currentMode(m_impactFlash);
    impact_flash::updateFlashBackdrop(flashMode, m_flashBackdrop, m_winSize, m_lastFlashBackdropMode);

    m_objectBlur.player.sourceRoot = m_playerRoot;
    m_objectBlur.player.enabled = true;
    PhysicsVelocity const playerVel = m_physics->getPlayerVelocityPixels();
    m_objectBlur.player.velocity = playerVel;

    overlay_rendering::refreshFireAura({
        .fireAura = m_fireAura.sprite,
        .playerVelocity = playerVel,
        .dt = dt,
        .impactFlashMode = flashMode,
        .fireTime = &m_fireAura.time,
    });

    overlay_rendering::refreshPlayerMotionBlurComposite({
        .capture = &m_objectBlur.player,
        .mergeRoot = m_objectBlur.mergeRoot,
        .unifiedMergeTexture = m_objectBlur.unifiedMergeTexture,
        .finalCompositeSprite = m_objectBlur.finalCompositeSprite,
        .whiteFlashSprite = m_objectBlur.whiteFlashSprite,
        .whiteFlashProgram = m_objectBlur.whiteFlashProgram,
        .colorInvertProgram = m_objectBlur.colorInvertProgram,
        .impactFlashMode = flashMode,
    });

    impact_flash::decrementWhiteFlash(m_impactFlash, dt);
    {
        bool const flashActive = m_impactFlash.whiteFlashRemaining > 0.0f;
        if (!flashActive && m_impactNoise.remaining > 0.0f) {
            m_impactNoise.remaining -= dt;
            m_impactNoise.remaining = std::max(0.0f, m_impactNoise.remaining);
        }
        float const alpha = std::clamp(m_impactNoise.remaining / kImpactNoiseFadeSeconds, 0.0f, 1.0f);
        bool const visible = !flashActive && m_impactNoise.remaining > 0.0f;
        float const extraSkip = m_impactNoise.extraTimeSkip;
        m_impactNoise.extraTimeSkip = 0.0f;
        overlay_rendering::refreshImpactNoise({
            .sprite = m_impactNoise.sprite,
            .renderTexture = m_impactNoise.renderTexture,
            .compositeSprite = m_impactNoise.composite,
            .dt = dt,
            .extraTimeSkip = extraSkip,
            .time = &m_impactNoise.time,
            .alpha = alpha,
            .visible = visible,
        });
    }
    star_burst::update(m_starBurst, m_impactFlash.whiteFlashRemaining, m_winSize, flashMode);
}

void PhysicsOverlay::update(float dt) {
    if (!updateEarlyGuards(dt)) {
        return;
    }
    updateSimulationAndDebug(dt);
    updateVisualPipeline(dt);
}

void PhysicsOverlay::onEnter() {
    CCLayer::onEnter();
    handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
    applyHideModOverlayFromTuning();
}
