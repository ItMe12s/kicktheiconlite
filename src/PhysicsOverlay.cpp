#include "PhysicsOverlay.h"

#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

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
    m_blurSprite = mr.blurSprite;
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

    m_physics = new PhysicsWorld(
        m_winSize.width, m_winSize.height,
        m_targetSize, m_targetSize
    );

    this->setContentSize(m_winSize);
    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchPriority(kPhysicsOverlayTouchPriority);

    m_whiteFlash = CCLayerColor::create(ccc4(255, 255, 255, 0), m_winSize.width, m_winSize.height);
    if (m_whiteFlash) {
        m_whiteFlash->setAnchorPoint({0, 0});
        m_whiteFlash->setPosition({0, 0});
        this->addChild(m_whiteFlash, 10000);
    }

    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, kPhysicsOverlaySchedulerPriority, false);
    return true;
}

void PhysicsOverlay::update(float dt) {
    if (!m_physics) {
        return;
    }

    if (!m_visualBuilt) {
        tryBuildPlayerVisual();
    }

    if (m_hitstopRemaining > 0.0f) {
        m_hitstopRemaining -= dt;
        if (m_hitstopRemaining < 0.0f) {
            m_hitstopRemaining = 0.0f;
        }
    } else {
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
            if (preSpeed >= kImpactMinSpeed) {
                m_hitstopRemaining = kImpactHitstopSeconds;
                overlay_rendering::runOverlayWhiteFlash(
                    m_whiteFlash,
                    kImpactWhiteFlashSeconds,
                    static_cast<unsigned char>(kImpactWhiteFlashPeakOpacity)
                );
            }
        }
    }

    if (!m_playerRoot || !m_player) {
        return;
    }

    auto state = m_physics->getPlayerState();
    m_playerRoot->setPosition({state.x, state.y});
    m_player->setRotation(-state.angle * kRadToDeg);
    overlay_rendering::refreshPlayerMotionBlur(
        dt,
        m_player,
        m_playerRoot,
        this,
        m_renderTexture,
        m_blurSprite,
        m_physics,
        m_captureSize
    );
}

void PhysicsOverlay::onEnter() {
    CCLayer::onEnter();
    handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
}

void PhysicsOverlay::onExit() {
    endGrab();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    delete m_physics;
    m_physics = nullptr;
    m_whiteFlash = nullptr;
    if (m_playerRoot) {
        m_playerRoot->removeFromParentAndCleanup(true);
        m_playerRoot = nullptr;
    }
    m_player = nullptr;
    m_blurSprite = nullptr;
    if (m_blurProgram) {
        m_blurProgram->release();
        m_blurProgram = nullptr;
    }
    if (m_renderTexture) {
        m_renderTexture->release();
        m_renderTexture = nullptr;
    }
    CCLayer::onExit();
}
