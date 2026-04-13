#include "PhysicsOverlay.h"

#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {

constexpr int kImpactFlashBackdropZOrder = -1;

constexpr int kImpactFlashInvertPhaseEndPhaseCount = 2;

inline ccColor4F flashBackdropBlackFill() {
    return ccc4f(0, 0, 0, 1);
}

inline ccColor4F flashBackdropWhiteFill() {
    return ccc4f(1, 1, 1, 1);
}

inline ccColor4F flashBackdropBorderTransparent() {
    return ccc4f(0, 0, 0, 0);
}

} // namespace

void PhysicsOverlay::tryBuildPlayerVisual() {
    if (m_visualBuilt) {
        return;
    }

    auto pr = player_visual::tryBuildPlayerRoot(
        this,
        m_winSize,
        m_targetSize,
        m_frameId,
        m_iconTypeInt
    );
    if (!pr.ok) {
        return;
    }

    m_captureSize = overlay_rendering::captureSizeForTarget(m_targetSize);
    auto mr = overlay_rendering::attachMotionBlur(pr.root, m_captureSize);
    if (!mr.ok) {
        return;
    }

    m_playerRoot = pr.root;
    m_player = pr.player;
    m_renderTexture = mr.renderTexture;
    m_blurProgram = mr.blurProgram;
    m_whiteFlashProgram = mr.whiteFlashProgram;
    m_colorInvertProgram = mr.colorInvertProgram;
    m_blurSprite = mr.blurSprite;
    m_whiteFlashSprite = mr.whiteFlashSprite;
    m_visualBuilt = true;
}

bool PhysicsOverlay::tryBeginGrab(CCPoint const& locationInNode) {
    if (!m_physics) {
        return false;
    }

    auto const state = m_physics->getPlayerState();
    float const dx = locationInNode.x - state.x;
    float const dy = locationInNode.y - state.y;
    float const distSq = dx * dx + dy * dy;
    float const grabRadius = m_targetSize * kGrabRadiusFraction;
    if (distSq > grabRadius * grabRadius) {
        return false;
    }

    m_physics->setDragGrabOffsetPixels(dx, dy);
    m_physics->setDragTargetPixels(locationInNode.x, locationInNode.y);
    m_physics->setDragging(true);
    m_grabActive = true;
    return true;
}

void PhysicsOverlay::endGrab() {
    if (!m_grabActive) {
        return;
    }
    m_grabActive = false;
    if (m_physics) {
        m_physics->setDragging(false);
    }
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (!m_physics) {
        return false;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    return tryBeginGrab(p);
}

void PhysicsOverlay::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_grabActive || !m_physics) {
        return;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    m_physics->setDragTargetPixels(p.x, p.y);
}

void PhysicsOverlay::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endGrab();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endGrab();
}

bool PhysicsOverlay::init() {
    if (!CCLayer::init()) {
        return false;
    }

    m_winSize = CCDirector::get()->getWinSize();
    float smaller = m_winSize.width < m_winSize.height ? m_winSize.width : m_winSize.height;
    m_targetSize = smaller * player_visual::kPlayerTargetSizeFraction;

    auto* gm = GameManager::get();
    if (!gm) {
        return false;
    }

    m_frameId = gm->getPlayerFrame();
    if (m_frameId < player_visual::kMinPlayerFrameId) {
        m_frameId = player_visual::kMinPlayerFrameId;
    }

    player_visual::requestCubeIconLoad(gm, m_frameId, m_iconTypeInt);

    m_physics = std::make_unique<PhysicsWorld>(
        m_winSize.width, m_winSize.height,
        m_targetSize, m_targetSize
    );

    this->setContentSize(m_winSize);

    m_flashBackdrop = CCDrawNode::create();
    if (m_flashBackdrop) {
        m_flashBackdrop->drawRect(
            CCRectMake(0, 0, m_winSize.width, m_winSize.height),
            flashBackdropBlackFill(),
            0.0f,
            flashBackdropBorderTransparent()
        );
        m_flashBackdrop->setPosition({0, 0});
        m_flashBackdrop->setVisible(false);
        this->addChild(m_flashBackdrop, kImpactFlashBackdropZOrder);
    }

    m_flashBackdropWhite = CCDrawNode::create();
    if (m_flashBackdropWhite) {
        m_flashBackdropWhite->drawRect(
            CCRectMake(0, 0, m_winSize.width, m_winSize.height),
            flashBackdropWhiteFill(),
            0.0f,
            flashBackdropBorderTransparent()
        );
        m_flashBackdropWhite->setPosition({0, 0});
        m_flashBackdropWhite->setVisible(false);
        this->addChild(m_flashBackdropWhite, kImpactFlashBackdropZOrder);
    }

    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchPriority(kPhysicsOverlayTouchPriority);

    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, kPhysicsOverlaySchedulerPriority, false);
    return true;
}

void PhysicsOverlay::decrementCooldowns(float dt) {
    if (m_impactFlashCooldownRemaining > 0.0f) {
        m_impactFlashCooldownRemaining -= dt;
        if (m_impactFlashCooldownRemaining < 0.0f) {
            m_impactFlashCooldownRemaining = 0.0f;
        }
    }
}

void PhysicsOverlay::tryBuildVisualIfNeeded() {
    if (!m_visualBuilt) {
        tryBuildPlayerVisual();
    }
}

void PhysicsOverlay::stepPhysicsUnlessHitstop(float dt) {
    if (m_hitstopRemaining > 0.0f) {
        m_hitstopRemaining -= dt;
        if (m_hitstopRemaining < 0.0f) {
            m_hitstopRemaining = 0.0f;
        }
        return;
    }

    m_physics->step(dt);

    if (m_physics->consumeWallImpact()) {
        float const postSpeed = m_physics->getPlayerSpeed();
        if (postSpeed >= kMinWallShakeSpeed) {
            float const strength = std::min(
                kMaxWallShakeStrength,
                postSpeed * kWallShakeSpeedToStrength
            );
            overlay_rendering::globalScreenShake(kWallShakeDuration, strength);
        }

        float const preSpeed = m_physics->getPreStepPlayerSpeedPx();
        if (preSpeed >= kImpactMinSpeed && m_impactFlashCooldownRemaining <= 0.0f) {
            m_hitstopRemaining = kImpactHitstopSeconds;
            m_whiteFlashRemaining = kImpactFlashTotalSeconds;
            m_impactFlashCooldownRemaining = kImpactFlashCooldownSeconds;
            if (m_whiteFlashSprite) {
                m_whiteFlashSprite->stopAllActions();
            }
        }
    }
}

void PhysicsOverlay::tickWhiteFlashWhenNoPlayer(float dt) {
    if (m_whiteFlashRemaining > 0.0f) {
        m_whiteFlashRemaining -= dt;
        if (m_whiteFlashRemaining < 0.0f) {
            m_whiteFlashRemaining = 0.0f;
        }
    }
}

void PhysicsOverlay::syncPlayerNodeFromPhysics() {
    auto state = m_physics->getPlayerState();
    m_playerRoot->setPosition({state.x, state.y});
    m_player->setRotation(-state.angle * kRadToDeg);
}

overlay_rendering::ImpactFlashMode PhysicsOverlay::currentImpactFlashMode() const {
    if (m_whiteFlashRemaining <= 0.0f) {
        return overlay_rendering::ImpactFlashMode::None;
    }
    float const elapsed = kImpactFlashTotalSeconds - m_whiteFlashRemaining;
    if (elapsed < kImpactFlashPhaseSeconds) {
        return overlay_rendering::ImpactFlashMode::WhiteSilhouette;
    }
    if (elapsed < static_cast<float>(kImpactFlashInvertPhaseEndPhaseCount) * kImpactFlashPhaseSeconds) {
        return overlay_rendering::ImpactFlashMode::InvertSilhouette;
    }
    return overlay_rendering::ImpactFlashMode::WhiteSilhouette;
}

void PhysicsOverlay::updateFlashBackdrops(overlay_rendering::ImpactFlashMode flashMode) {
    bool const impactFlashActive = flashMode != overlay_rendering::ImpactFlashMode::None;
    if (m_flashBackdrop) {
        m_flashBackdrop->setVisible(impactFlashActive && flashMode == overlay_rendering::ImpactFlashMode::WhiteSilhouette);
    }
    if (m_flashBackdropWhite) {
        m_flashBackdropWhite->setVisible(impactFlashActive && flashMode == overlay_rendering::ImpactFlashMode::InvertSilhouette);
    }
}

void PhysicsOverlay::tickWhiteFlashRemaining(float dt) {
    if (m_whiteFlashRemaining > 0.0f) {
        m_whiteFlashRemaining -= dt;
        if (m_whiteFlashRemaining < 0.0f) {
            m_whiteFlashRemaining = 0.0f;
        }
    }
}

void PhysicsOverlay::update(float dt) {
    if (!m_physics) {
        return;
    }

    decrementCooldowns(dt);
    tryBuildVisualIfNeeded();
    stepPhysicsUnlessHitstop(dt);

    if (!m_playerRoot || !m_player) {
        tickWhiteFlashWhenNoPlayer(dt);
        return;
    }

    syncPlayerNodeFromPhysics();

    overlay_rendering::ImpactFlashMode const flashMode = currentImpactFlashMode();
    updateFlashBackdrops(flashMode);

    overlay_rendering::refreshPlayerMotionBlur({
        .dt = dt,
        .player = m_player,
        .playerRoot = m_playerRoot,
        .hostLayer = this,
        .renderTexture = m_renderTexture,
        .blurSprite = m_blurSprite,
        .whiteFlashSprite = m_whiteFlashSprite,
        .whiteFlashProgram = m_whiteFlashProgram,
        .colorInvertProgram = m_colorInvertProgram,
        .physics = m_physics.get(),
        .captureSize = m_captureSize,
        .impactFlashMode = flashMode,
    });

    tickWhiteFlashRemaining(dt);
}

void PhysicsOverlay::onEnter() {
    CCLayer::onEnter();
    handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
}

void PhysicsOverlay::onExit() {
    endGrab();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_physics.reset();
    if (m_playerRoot) {
        m_playerRoot->removeFromParentAndCleanup(true);
        m_playerRoot = nullptr;
    }
    m_player = nullptr;
    m_blurSprite = nullptr;
    m_whiteFlashSprite = nullptr;
    if (m_blurProgram) {
        m_blurProgram->release();
        m_blurProgram = nullptr;
    }
    if (m_whiteFlashProgram) {
        m_whiteFlashProgram->release();
        m_whiteFlashProgram = nullptr;
    }
    if (m_colorInvertProgram) {
        m_colorInvertProgram->release();
        m_colorInvertProgram = nullptr;
    }
    if (m_renderTexture) {
        m_renderTexture->release();
        m_renderTexture = nullptr;
    }
    CCLayer::onExit();
}
