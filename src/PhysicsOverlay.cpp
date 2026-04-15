#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "PlayerVisual.h"
#include "events/ClickEvents.h"

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

PhysicsOverlay::PhysicsOverlay() = default;
PhysicsOverlay::~PhysicsOverlay() = default;

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

    m_blurCaptureSize = m_winSize;
    auto mr = overlay_rendering::attachMotionBlur(
        this,
        m_blurCaptureSize,
        m_winSize,
        kUnifiedBlurCompositeZOrder
    );
    if (!mr.ok) {
        pr.root->removeFromParentAndCleanup(true);
        return;
    }

    m_playerRoot = pr.root;
    if (m_worldCaptureRoot) {
        m_playerRoot->retain();
        this->removeChild(m_playerRoot, false);
        m_worldCaptureRoot->addChild(m_playerRoot, kPlayerRootZOrder);
        m_playerRoot->release();
    } else {
        m_playerRoot->setZOrder(kPlayerRootZOrder);
    }
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
            star->setID(std::string(GEODE_MOD_ID) + "/star-burst-" + std::to_string(i));
            star->setVisible(false);
            star->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
            if (m_starBurstLayer) {
                m_starBurstLayer->addChild(star, kStarBurstZOrder);
                m_starSprites[i] = star;
            }
        }
    }

    // Always-visible invisible sprite so hit tests work when SimplePlayer is hidden (like motion blur)
    if (auto* hitProxy = CCSprite::create("star1_hd.png"_spr)) {
        hitProxy->setID(std::string(GEODE_MOD_ID) + "/player-hit-proxy");
        hitProxy->setPosition({0.f, 0.f});
        hitProxy->setOpacity(0);
        hitProxy->setVisible(true);
        m_playerRoot->addChild(hitProxy, kHitProxyLocalZOrder);
        m_hitProxy = hitProxy;
        events::ClickTracker::get()->track(m_hitProxy, m_targetSize * kGrabRadiusFraction);
    }

    m_doubleClickListener = events::DoubleClickEvent().listen([this](cocos2d::CCSprite* sprite, cocos2d::CCTouch*) {
        if (sprite != m_hitProxy) {
            return false;
        }
        log::info("double clicked the icon");
        return false;
    });
    m_tripleClickListener = events::TripleClickEvent().listen([this](cocos2d::CCSprite* sprite, cocos2d::CCTouch*) {
        if (sprite != m_hitProxy) {
            return false;
        }
        toggleTestPanel();
        return false;
    });

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

void PhysicsOverlay::toggleTestPanel() {
    if (!m_physics) {
        return;
    }
    if (m_physicsMenuVisual) {
        m_physics->destroyPanel();
        m_physicsMenuVisual.reset();
        m_panelDragActive = false;
        log::info("physics menu destroyed");
        return;
    }

    float const w = m_winSize.width * kPanelDefaultWFrac;
    float const h = m_winSize.height * kPanelDefaultHFrac;
    float const x = m_winSize.width * kPanelDefaultXFrac;
    float const y = m_winSize.height * kPanelDefaultYFrac;

    auto panel = std::make_unique<PhysicsMenu>();
    if (!panel->build(w, h)) {
        return;
    }
    if (auto* root = panel->getRoot()) {
        if (m_worldCaptureRoot) {
            m_worldCaptureRoot->addChild(root, kPhysicsMenuZOrder);
        } else {
            log::warn("world capture root missing; physics menu will bypass blur capture");
            this->addChild(root, kPhysicsMenuZOrder);
        }
    }
    m_physics->spawnPanel(w, h, x, y);
    m_physicsMenuVisual = std::move(panel);
    log::info("physics menu spawned");
}

bool PhysicsOverlay::tryBeginPanelGrab(CCPoint const& locationInNode) {
    if (!m_physicsMenuVisual || !m_physics || !m_physics->hasPanel()) {
        return false;
    }
    PhysicsState const st = m_physics->getPanelState();
    float const dx = locationInNode.x - st.x;
    float const dy = locationInNode.y - st.y;
    float const c = std::cos(st.angle);
    float const s = std::sin(st.angle);
    float const localX = c * dx + s * dy;
    float const localY = -s * dx + c * dy;
    if (!m_physicsMenuVisual->containsLocalPoint(localX, localY)) {
        return false;
    }
    m_physics->setPanelDragGrabOffsetPixels(dx, dy);
    m_physics->setPanelDragTargetPixels(locationInNode.x, locationInNode.y);
    m_physics->setPanelDragging(true);
    m_panelDragActive = true;
    return true;
}

void PhysicsOverlay::syncPanelNodeFromPhysics(float alpha) {
    if (!m_physicsMenuVisual || !m_physics || !m_physics->hasPanel()) {
        return;
    }
    auto* root = m_physicsMenuVisual->getRoot();
    if (!root) {
        return;
    }
    PhysicsState const st = m_physics->getPanelRenderState(alpha);
    root->setPosition({st.x, st.y});
    root->setRotation(-st.angle * kRadToDeg);
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (!m_physics) {
        return false;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    if (m_physicsMenuVisual && m_physics->hasPanel() && tryBeginPanelGrab(p)) {
        events::ClickTracker::get()->onTouchBegan(touch);
        return true;
    }
    bool const clickHit = events::ClickTracker::get()->onTouchBegan(touch);
    bool const grabbed = tryBeginGrab(p);
    return clickHit || grabbed;
}

void PhysicsOverlay::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_physics) {
        return;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    if (m_panelDragActive) {
        m_physics->setPanelDragTargetPixels(p.x, p.y);
        return;
    }
    if (!m_grabActive) {
        return;
    }
    m_physics->setDragTargetPixels(p.x, p.y);
}

void PhysicsOverlay::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    (void)event;
    events::ClickTracker::get()->onTouchEnded(touch);
    if (m_panelDragActive && m_physics) {
        m_physics->setPanelDragging(false);
        m_panelDragActive = false;
    }
    endGrab();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)event;
    events::ClickTracker::get()->onTouchCancelled(touch);
    if (m_panelDragActive && m_physics) {
        m_physics->setPanelDragging(false);
        m_panelDragActive = false;
    }
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

    m_worldCaptureRoot = CCNode::create();
    if (m_worldCaptureRoot) {
        m_worldCaptureRoot->setID("world-capture-root"_spr);
        m_worldCaptureRoot->setPosition({0.0f, 0.0f});
        this->addChild(m_worldCaptureRoot, kUnifiedWorldCaptureZOrder);
    }

    m_starBurstLayer = CCNode::create();
    if (m_starBurstLayer) {
        m_starBurstLayer->setID("global-star-burst-layer"_spr);
        m_starBurstLayer->setPosition({0.0f, 0.0f});
        this->addChild(m_starBurstLayer, kGlobalStartBurstZOrder);
    }

    m_flashBackdrop = CCDrawNode::create();
    if (m_flashBackdrop) {
        m_flashBackdrop->setID("impact-flash-backdrop"_spr);
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
        m_flashBackdropWhite->setID("impact-flash-backdrop-white"_spr);
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

    auto const noiseAttach = overlay_rendering::attachImpactNoise(this, m_winSize);
    if (noiseAttach.ok) {
        m_impactNoiseSprite = noiseAttach.sprite;
        m_impactNoiseProgram = noiseAttach.program;
        m_impactNoiseRenderTexture = noiseAttach.renderTexture;
        m_impactNoiseComposite = noiseAttach.compositeSprite;
    }

    m_trailLayer = CCNode::create();
    if (m_trailLayer) {
        m_trailLayer->setID("sandevistan-trail-layer"_spr);
        m_trailLayer->setPosition({0, 0});
        if (m_worldCaptureRoot) {
            m_worldCaptureRoot->addChild(m_trailLayer, kSandevistanTrailLayerZOrder);
        } else {
            log::warn("world capture root missing; sandevistan trail will bypass blur capture");
            this->addChild(m_trailLayer, kSandevistanTrailLayerZOrder);
        }
    }

    setID("physics-overlay"_spr);

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

        auto const playerImpact = m_physics->consumePlayerImpactAny();
        if (playerImpact.triggered) {
            float const playerImpactIntensityPx = std::max(playerImpact.postSpeedPx, playerImpact.impactSpeedPx);
            if (playerImpactIntensityPx >= kPlayerImpactMinShakeSpeed) {
                float const strength = std::min(
                    kPlayerImpactMaxShakeStrength,
                    playerImpactIntensityPx * kPlayerImpactShakeSpeedToStrength
                );
                overlay_rendering::globalScreenShake(kPlayerImpactShakeDuration, strength);
            }

            if (
                kEnablePlayerImpactTrail
                && playerImpactIntensityPx >= kPlayerImpactMinShakeSpeed
                && !m_grabActive
            ) {
                m_sandevistanTrailActive = true;
                m_sandevistanSpawnAccumulator = kSandevistanSpawnIntervalSec;
            }

            if (
                kEnablePlayerImpactFlashStack
                && !m_grabActive
                && playerImpact.impactSpeedPx >= kPlayerImpactMinFlashSpeed
                && m_impactFlashCooldownRemaining <= 0.0f
            ) {
                if (m_impactNoiseRemaining > 0.0f) {
                    m_impactNoiseExtraTimeSkip += kImpactNoiseStackedImpactTimeSkip;
                }
                m_hitstopRemaining = kImpactHitstopSeconds;
                m_whiteFlashRemaining = kImpactFlashTotalSeconds;
                m_impactFlashCooldownRemaining = kImpactFlashCooldownSeconds;
                m_impactNoiseRemaining = kImpactNoiseFadeSeconds;
                if (m_whiteFlashSprite) {
                    m_whiteFlashSprite->stopAllActions();
                }
            }
        }

        auto const panelImpact = m_physics->consumePanelImpactAny();
        if (
            kEnablePanelImpactShake
            && panelImpact.triggered
            && panelImpact.impactSpeedPx >= kPanelImpactMinShakeSpeed
        ) {
            float const strength = std::min(
                kPanelImpactMaxShakeStrength,
                panelImpact.impactSpeedPx * kPanelImpactShakeSpeedToStrength
            );
            overlay_rendering::globalScreenShake(kPanelImpactShakeDuration, strength);
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
    if (m_starBurstLayer) {
        m_starBurstLayer->setPosition(m_playerRoot->getPosition());
    }
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

void PhysicsOverlay::updateImpactNoise(float dt) {
    bool const flashActive = m_whiteFlashRemaining > 0.0f;
    if (!flashActive && m_impactNoiseRemaining > 0.0f) {
        m_impactNoiseRemaining -= dt;
        if (m_impactNoiseRemaining < 0.0f) {
            m_impactNoiseRemaining = 0.0f;
        }
    }

    float alpha = m_impactNoiseRemaining / kImpactNoiseFadeSeconds;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    bool const visible = !flashActive && m_impactNoiseRemaining > 0.0f;

    float const extraSkip = m_impactNoiseExtraTimeSkip;
    m_impactNoiseExtraTimeSkip = 0.0f;

    overlay_rendering::refreshImpactNoise({
        .sprite = m_impactNoiseSprite,
        .renderTexture = m_impactNoiseRenderTexture,
        .compositeSprite = m_impactNoiseComposite,
        .dt = dt,
        .extraTimeSkip = extraSkip,
        .time = &m_impactNoiseTime,
        .alpha = alpha,
        .visible = visible,
    });
}

void PhysicsOverlay::updateSandevistanTrail(float dt) {
    if (!m_trailLayer || !m_sandevistanTrailActive || !m_playerRoot || !m_player) {
        return;
    }

    m_sandevistanSpawnAccumulator += dt;
    while (m_sandevistanSpawnAccumulator >= kSandevistanSpawnIntervalSec) {
        int childCount = 0;
        if (CCArray* const ch = m_trailLayer->getChildren()) {
            childCount = ch->count();
        }
        if (childCount >= kSandevistanMaxConcurrentGhosts) {
            break;
        }
        m_sandevistanSpawnAccumulator -= kSandevistanSpawnIntervalSec;
        player_visual::spawnFadingGhost(
            m_trailLayer,
            m_playerRoot->getPosition(),
            m_player->getRotation(),
            m_targetSize,
            m_frameId,
            m_iconTypeInt,
            kSandevistanGhostFadeSec,
            static_cast<unsigned char>(kSandevistanGhostStartOpacity)
        );
    }
}

void PhysicsOverlay::update(float dt) {
    if (!m_physics) {
        return;
    }

    decrementCooldowns(dt);
    tryBuildVisualIfNeeded();
    stepPhysicsUnlessHitstop(dt);

    if (m_physics && (m_grabActive || m_physics->getPlayerSpeed() < kSandevistanEndSpeedPx)) {
        m_sandevistanTrailActive = false;
    }

    if (!m_playerRoot || !m_player) {
        decrementWhiteFlashRemaining(dt);
        return;
    }

    syncPlayerNodeFromPhysics();
    if (m_physicsMenuVisual && m_physics->hasPanel()) {
        float const alpha = m_physicsAccumulator / kFixedPhysicsDt;
        syncPanelNodeFromPhysics(alpha);
    }
    updateSandevistanTrail(dt);

    overlay_rendering::ImpactFlashMode const flashMode = currentImpactFlashMode();
    updateFlashBackdrops(flashMode);

    overlay_rendering::refreshFireAura({
        .fireAura = m_fireAuraSprite,
        .physics = m_physics.get(),
        .dt = dt,
        .impactFlashMode = flashMode,
        .fireTime = &m_fireAuraTime,
    });

    overlay_rendering::refreshMotionBlurComposite({
        .captureRoot = m_worldCaptureRoot,
        .renderTexture = m_renderTexture,
        .blurSprite = m_blurSprite,
        .whiteFlashSprite = m_whiteFlashSprite,
        .whiteFlashProgram = m_whiteFlashProgram,
        .colorInvertProgram = m_colorInvertProgram,
        .velocity = m_physics->getPlayerVelocityPixels(),
        .impactFlashMode = flashMode,
    });

    decrementWhiteFlashRemaining(dt);
    updateImpactNoise(dt);
    updateStarBurst();
}

void PhysicsOverlay::onEnter() {
    CCLayer::onEnter();
    handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
}

void PhysicsOverlay::onExit() {
    endGrab();
    if (m_physicsMenuVisual) {
        if (m_physics) {
            m_physics->destroyPanel();
        }
        m_physicsMenuVisual.reset();
        m_panelDragActive = false;
    }
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_physics.reset();
    if (m_trailLayer) {
        m_trailLayer->removeAllChildrenWithCleanup(true);
        m_trailLayer->removeFromParentAndCleanup(true);
        m_trailLayer = nullptr;
    }
    if (m_playerRoot) {
        m_playerRoot->removeFromParentAndCleanup(true);
        m_playerRoot = nullptr;
    }
    if (m_starBurstLayer) {
        m_starBurstLayer->removeFromParentAndCleanup(true);
        m_starBurstLayer = nullptr;
    }
    if (m_worldCaptureRoot) {
        m_worldCaptureRoot->removeFromParentAndCleanup(true);
        m_worldCaptureRoot = nullptr;
    }
    m_doubleClickListener.destroy();
    m_tripleClickListener.destroy();
    if (m_hitProxy) {
        events::ClickTracker::get()->untrack(m_hitProxy);
        m_hitProxy = nullptr;
    }
    m_player = nullptr;
    m_blurSprite = nullptr;
    m_fireAuraSprite = nullptr;
    if (m_impactNoiseComposite) {
        m_impactNoiseComposite->removeFromParentAndCleanup(true);
        m_impactNoiseComposite = nullptr;
    }
    if (m_impactNoiseRenderTexture) {
        m_impactNoiseRenderTexture->release();
        m_impactNoiseRenderTexture = nullptr;
    }
    if (m_impactNoiseSprite) {
        m_impactNoiseSprite->removeFromParentAndCleanup(true);
        m_impactNoiseSprite = nullptr;
    }
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
    if (m_impactNoiseProgram) {
        m_impactNoiseProgram->release();
        m_impactNoiseProgram = nullptr;
    }
    if (m_renderTexture) {
        m_renderTexture->release();
        m_renderTexture = nullptr;
    }
    CCLayer::onExit();
}
