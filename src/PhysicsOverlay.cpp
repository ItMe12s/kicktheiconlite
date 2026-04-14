#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>
#include <random>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PlayerVisual.h"

using namespace geode::prelude;

namespace {

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

    auto const fa = overlay_rendering::attachFireAura(pr.root, m_targetSize * kFireAuraDiameterScale);
    if (fa.ok) {
        m_fireAuraSprite = fa.sprite;
        m_fireAuraProgram = fa.program;
    }

    for (int i = 0; i < kStarBurstCount; ++i) {
        auto* star = CCSprite::create("star1_hd.png"_spr);
        if (star) {
            star->setVisible(false);
            star->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
            m_playerRoot->addChild(star, kStarBurstZOrder);
            m_starSprites[i] = star;
        }
    }

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

    m_hitstopRemaining = 0.0f;
    m_whiteFlashRemaining = 0.0f;
    hideAllStars();
    m_starPhaseIndex = -1;
    if (m_whiteFlashSprite) {
        m_whiteFlashSprite->stopAllActions();
    }
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

    m_frameId = player_visual::kMinPlayerFrameId;
    m_iconTypeInt = static_cast<int>(IconType::Cube);

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

    m_physicsAccumulator += dt;
    if (m_physicsAccumulator > kPhysicsAccumulatorCap) {
        m_physicsAccumulator = kPhysicsAccumulatorCap;
    }

    int substepCount = 0;
    while (m_physicsAccumulator >= kFixedPhysicsDt && substepCount < kMaxPhysicsSubsteps) {
        m_physics->step(kFixedPhysicsDt);

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
            if (!m_grabActive && preSpeed >= kImpactMinSpeed && m_impactFlashCooldownRemaining <= 0.0f) {
                m_hitstopRemaining = kImpactHitstopSeconds;
                m_whiteFlashRemaining = kImpactFlashTotalSeconds;
                m_impactFlashCooldownRemaining = kImpactFlashCooldownSeconds;
                if (m_whiteFlashSprite) {
                    m_whiteFlashSprite->stopAllActions();
                }
            }
        }

        m_physicsAccumulator -= kFixedPhysicsDt;
        ++substepCount;

        if (m_hitstopRemaining > 0.0f) {
            break;
        }
    }
}

void PhysicsOverlay::decrementWhiteFlashRemaining(float dt) {
    if (m_whiteFlashRemaining > 0.0f) {
        m_whiteFlashRemaining -= dt;
        if (m_whiteFlashRemaining < 0.0f) {
            m_whiteFlashRemaining = 0.0f;
        }
    }
}

void PhysicsOverlay::syncPlayerNodeFromPhysics() {
    float const alpha = m_physicsAccumulator / kFixedPhysicsDt;
    auto state = m_physics->getPlayerRenderState(alpha);
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

int PhysicsOverlay::computeCurrentStarPhase() const {
    if (m_whiteFlashRemaining <= 0.0f) {
        return -1;
    }
    float const elapsed = kImpactFlashTotalSeconds - m_whiteFlashRemaining;
    int const phase = static_cast<int>(elapsed / kImpactFlashPhaseSeconds);
    return std::min(phase, kStarBurstMaxPhaseIndex);
}

void PhysicsOverlay::hideAllStars() {
    for (auto* s : m_starSprites) {
        if (s) {
            s->setVisible(false);
            s->setColor(ccc3(255, 255, 255));
        }
    }
}

void PhysicsOverlay::repositionStarBurst() {
    thread_local std::mt19937 rng{std::random_device{}()};
    float const screenSmaller = m_winSize.width < m_winSize.height ? m_winSize.width : m_winSize.height;
    std::uniform_real_distribution<float> sectorJitter(0.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> varianceDist(-kStarScaleVariance, kStarScaleVariance);
    std::uniform_real_distribution<float> bigRadDist(
        screenSmaller * kBigStarRadiusMin,
        screenSmaller * kBigStarRadiusMax
    );
    std::uniform_real_distribution<float> smallRadDist(
        screenSmaller * kSmallStarRadiusMin,
        screenSmaller * kSmallStarRadiusMax
    );

    for (int i = 0; i < kBigStarCount; ++i) {
        auto* s = m_starSprites[i];
        if (!s) continue;

        float const sector = (static_cast<float>(i) + sectorJitter(rng))
                            / static_cast<float>(kBigStarCount);
        float const angle = sector * 2.0f * std::numbers::pi_v<float>;
        float const radius = bigRadDist(rng);
        float const cw = s->getContentSize().width;
        float const baseScale = cw > 0.0f ? (screenSmaller * kBigStarScreenFrac) / cw : 1.0f;

        s->setPosition({std::cos(angle) * radius, std::sin(angle) * radius});
        s->setRotation(0.0f);
        s->setScale(baseScale * (1.0f + varianceDist(rng)));
        s->setVisible(true);
    }

    for (int i = 0; i < kSmallStarCount; ++i) {
        auto* s = m_starSprites[kBigStarCount + i];
        if (!s) continue;

        float const angle = angleDist(rng);
        float const radius = smallRadDist(rng);
        float const cw = s->getContentSize().width;
        float const baseScale = cw > 0.0f ? (screenSmaller * kSmallStarScreenFrac) / cw : 1.0f;

        s->setPosition({std::cos(angle) * radius, std::sin(angle) * radius});
        s->setRotation(0.0f);
        s->setScale(baseScale * (1.0f + varianceDist(rng)));
        s->setVisible(true);
    }

    applyStarBurstTint();
}

void PhysicsOverlay::applyStarBurstTint() {
    bool const whiteBackdrop =
        currentImpactFlashMode() == overlay_rendering::ImpactFlashMode::InvertSilhouette;
    ccColor3B const tint = whiteBackdrop ? ccc3(0, 0, 0) : ccc3(255, 255, 255);
    for (auto* s : m_starSprites) {
        if (s) {
            s->setColor(tint);
        }
    }
}

void PhysicsOverlay::updateStarBurst() {
    int const newPhase = computeCurrentStarPhase();

    if (newPhase < 0) {
        if (m_starPhaseIndex >= 0) {
            hideAllStars();
            m_starPhaseIndex = -1;
        }
        return;
    }

    if (newPhase != m_starPhaseIndex) {
        m_starPhaseIndex = newPhase;
        repositionStarBurst();
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
        decrementWhiteFlashRemaining(dt);
        return;
    }

    syncPlayerNodeFromPhysics();

    overlay_rendering::ImpactFlashMode const flashMode = currentImpactFlashMode();
    updateFlashBackdrops(flashMode);

    overlay_rendering::refreshPlayerMotionBlur({
        .player = m_player,
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

    overlay_rendering::refreshFireAura({
        .fireAura = m_fireAuraSprite,
        .physics = m_physics.get(),
        .dt = dt,
        .impactFlashMode = flashMode,
        .fireTime = &m_fireAuraTime,
    });

    decrementWhiteFlashRemaining(dt);
    updateStarBurst();
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
    m_fireAuraSprite = nullptr;
    m_whiteFlashSprite = nullptr;
    for (auto*& s : m_starSprites) { s = nullptr; }
    m_starPhaseIndex = -1;
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
    if (m_fireAuraProgram) {
        m_fireAuraProgram->release();
        m_fireAuraProgram = nullptr;
    }
    if (m_renderTexture) {
        m_renderTexture->release();
        m_renderTexture = nullptr;
    }
    CCLayer::onExit();
}
