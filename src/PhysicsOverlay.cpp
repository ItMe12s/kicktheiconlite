#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "PlayerVisual.h"
#include "events/ClickEvents.h"
#include "vfx/ImpactFlash.h"
#include "vfx/ImpactNoise.h"
#include "vfx/ObjectMotionBlurPipeline.h"
#include "vfx/SandevistanTrail.h"
#include "vfx/StarBurst.h"

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

inline ccColor4F debugLabelBoxFill() {
    return ccc4f(
        kDebugLabelBoxColorR,
        kDebugLabelBoxColorG,
        kDebugLabelBoxColorB,
        kDebugLabelBoxAlpha
    );
}

inline GLubyte colorToByte(float channel) {
    float const clamped = std::max(0.0f, std::min(channel, 1.0f));
    return static_cast<GLubyte>(clamped * 255.0f);
}

std::vector<std::string> splitDebugLines(std::string const& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.emplace_back(std::move(line));
    }
    if (lines.empty()) {
        lines.emplace_back("");
    }
    return lines;
}

inline void mergeImpactSnapshot(PhysicsImpactEvent& total, PhysicsImpactEvent const& current) {
    total.triggered = total.triggered || current.triggered;
    total.preSpeedPx = std::max(total.preSpeedPx, current.preSpeedPx);
    total.postSpeedPx = std::max(total.postSpeedPx, current.postSpeedPx);
    total.impactSpeedPx = std::max(total.impactSpeedPx, current.impactSpeedPx);
}

} // namespace

PhysicsOverlay::PhysicsOverlay() = default;
PhysicsOverlay::~PhysicsOverlay() = default;

namespace {
cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> const& roots,
    overlay_rendering::OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
}
}

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

    cocos2d::CCSize const blurCaptureSize = m_winSize;
    if (!vfx::object_motion_blur::attach(
        m_objectBlur,
        this,
        blurCaptureSize,
        m_winSize,
        kUnifiedBlurCompositeZOrder,
        {
            overlay_rendering::MotionBlurObjectSeed {
                .id = overlay_rendering::MotionBlurObjectId::Player,
                .sourceRoot = pr.root,
                .enabled = true,
                .tuning = {
                    .minBlurSpeedPx = kPlayerMinBlurSpeedPx,
                    .maxBlurSpeedPx = kPlayerMaxBlurSpeedPx,
                    .blurUvSpread = kPlayerBlurUvSpread,
                    .blurStepDivisor = kPlayerBlurStepDivisor,
                    .keepBaseVisible = kPlayerKeepBaseVisible,
                },
            },
            overlay_rendering::MotionBlurObjectSeed {
                .id = overlay_rendering::MotionBlurObjectId::PhysicsMenu,
                .sourceRoot = nullptr,
                .enabled = false,
                .tuning = {
                    .minBlurSpeedPx = kMenuMinBlurSpeedPx,
                    .maxBlurSpeedPx = kMenuMaxBlurSpeedPx,
                    .blurUvSpread = kMenuBlurUvSpread,
                    .blurStepDivisor = kMenuBlurStepDivisor,
                    .keepBaseVisible = kMenuKeepBaseVisible,
                },
            },
        }
    )) {
        pr.root->removeFromParentAndCleanup(true);
        return;
    }

    m_playerRoot = pr.root;
    auto* worldRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::World);
    if (worldRoot) {
        m_playerRoot->retain();
        this->removeChild(m_playerRoot, false);
        worldRoot->addChild(m_playerRoot, kPlayerRootZOrder);
        m_playerRoot->release();
    } else {
        m_playerRoot->setZOrder(kPlayerRootZOrder);
    }
    m_player = pr.player;

    auto const fa = overlay_rendering::attachFireAura(pr.root, m_targetSize * kFireAuraDiameterScale);
    if (fa.ok) {
        m_fireAura.sprite = fa.sprite;
        m_fireAura.program = fa.program;
    }

    vfx::star_burst::createSprites(m_starBurst);

    // Always-visible invisible sprite so hit tests work when SimplePlayer is hidden (like motion blur)
    if (auto* hitProxy = CCSprite::create("img_star1.png"_spr)) {
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

    m_impactFlash.hitstopRemaining = 0.0f;
    m_impactFlash.whiteFlashRemaining = 0.0f;
    vfx::star_burst::reset(m_starBurst);
    if (m_objectBlur.whiteFlashSprite) {
        m_objectBlur.whiteFlashSprite->stopAllActions();
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
        auto& menuCapture =
            m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::PhysicsMenu)];
        menuCapture.enabled = false;
        menuCapture.sourceRoot = nullptr;
        menuCapture.velocity = {};
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
        auto* uiLayerRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Ui);
        if (uiLayerRoot) {
            uiLayerRoot->addChild(root, kPhysicsMenuZOrder);
        } else {
            log::warn("world capture root missing; physics menu will bypass blur capture");
            this->addChild(root, kPhysicsMenuZOrder);
        }
        auto& menuCapture =
            m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::PhysicsMenu)];
        menuCapture.sourceRoot = root;
        menuCapture.enabled = true;
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

void PhysicsOverlay::endTouchInteraction() {
    if (m_panelDragActive && m_physics) {
        m_physics->setPanelDragging(false);
        m_panelDragActive = false;
    }
    endGrab();
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    (void)event;
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
    endTouchInteraction();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)event;
    events::ClickTracker::get()->onTouchCancelled(touch);
    endTouchInteraction();
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

    auto createLayerRoot = [this](char const* id, int z) -> CCNode* {
        auto* root = CCNode::create();
        if (!root) {
            return nullptr;
        }
        root->setID(id);
        root->setPosition({0.0f, 0.0f});
        this->addChild(root, z);
        return root;
    };
    m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::World)] =
        createLayerRoot("layer-world-root"_spr, kUnifiedWorldCaptureZOrder + kLayerWorldZOrderOffset);
    m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::Trail)] =
        createLayerRoot("layer-trail-root"_spr, kUnifiedWorldCaptureZOrder + kLayerTrailZOrderOffset);
    m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::Ui)] =
        createLayerRoot("layer-ui-root"_spr, kUnifiedWorldCaptureZOrder + kLayerUiZOrderOffset);

    auto* uiRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Ui);
    if (uiRoot) {
        float const baseX = kDebugLabelMarginX;
        float const baseY = m_winSize.height - kDebugLabelMarginY;

        m_debugLabelBackground = CCNode::create();
        if (m_debugLabelBackground) {
            m_debugLabelBackground->setID("debug-overlay-line-backgrounds"_spr);
            m_debugLabelBackground->setPosition({0.0f, 0.0f});
            uiRoot->addChild(m_debugLabelBackground, kDebugLabelBackgroundZOrder);

            m_debugLabelBackgroundTexture = CCRenderTexture::create(1, 1);
            if (m_debugLabelBackgroundTexture) {
                m_debugLabelBackgroundTexture->retain();
                m_debugLabelBackgroundTexture->beginWithClear(1.0f, 1.0f, 1.0f, 1.0f);
                m_debugLabelBackgroundTexture->end();
            }
        }

        m_debugLabel = CCLabelBMFont::create("Yo", "chatFont.fnt");
        if (m_debugLabel) {
            m_debugLabel->setID("debug-overlay-label"_spr);
            m_debugLabel->setAnchorPoint({0.0f, 1.0f});
            m_debugLabel->setScale(kDebugLabelFontScale);
            m_debugLabel->setPosition({baseX, baseY});
            uiRoot->addChild(m_debugLabel, kDebugLabelZOrder);
        }

        m_debugLabelMeasure = CCLabelBMFont::create("Ag", "chatFont.fnt");
        if (m_debugLabelMeasure) {
            m_debugLabelMeasure->setScale(kDebugLabelFontScale);
            m_debugLabelMeasure->setVisible(false);
            uiRoot->addChild(m_debugLabelMeasure, kDebugLabelBackgroundZOrder);
        }
    }

    m_starBurst.layer = CCNode::create();
    if (m_starBurst.layer) {
        m_starBurst.layer->setID("global-star-burst-layer"_spr);
        m_starBurst.layer->setPosition({0.0f, 0.0f});
        this->addChild(m_starBurst.layer, kGlobalStartBurstZOrder);
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
        m_impactNoise.sprite = noiseAttach.sprite;
        m_impactNoise.program = noiseAttach.program;
        m_impactNoise.renderTexture = noiseAttach.renderTexture;
        m_impactNoise.composite = noiseAttach.compositeSprite;
    }

    m_trail.layer = CCNode::create();
    if (m_trail.layer) {
        m_trail.layer->setID("sandevistan-trail-layer"_spr);
        m_trail.layer->setPosition({0, 0});
        auto* trailRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Trail);
        if (trailRoot) {
            trailRoot->addChild(m_trail.layer, kSandevistanTrailLayerZOrder);
        } else {
            log::warn("world capture root missing; sandevistan trail will bypass blur capture");
            this->addChild(m_trail.layer, kSandevistanTrailLayerZOrder);
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
    vfx::impact_flash::decrementCooldown(m_impactFlash, dt);
}

void PhysicsOverlay::tryBuildVisualIfNeeded() {
    if (!m_visualBuilt) {
        tryBuildPlayerVisual();
    }
}

void PhysicsOverlay::stepPhysicsUnlessHitstop(float dt) {
    m_lastPhysicsSubsteps = 0;
    m_lastPlayerImpact = {};
    m_lastPanelImpact = {};

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

        auto const panelImpact = m_physics->consumePanelImpactAny();
        mergeImpactSnapshot(m_lastPanelImpact, panelImpact);
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
        m_lastPhysicsSubsteps = substepCount;

        if (m_impactFlash.hitstopRemaining > 0.0f) {
            break;
        }
    }
}

void PhysicsOverlay::updateDebugOverlayText(float intervalSec) {
    if (!m_debugLabel || !m_physics) {
        return;
    }

    float const sampleMs = intervalSec * 1000.0f;
    float const sampleHz = intervalSec > 0.00001f ? (1.0f / intervalSec) : 0.0f;
    float const accumulatorRatio = std::clamp(m_physicsAccumulator / kFixedPhysicsDt, 0.0f, 1.0f);

    PhysicsState const playerState = m_physics->getPlayerState();
    PhysicsVelocity const playerVelocity = m_physics->getPlayerVelocityPixels();
    float const playerSpeed = m_physics->getPlayerSpeed();

    bool const panelAvailable = m_physics->hasPanel();
    PhysicsState panelState{};
    PhysicsVelocity panelVelocity{};
    if (panelAvailable) {
        panelState = m_physics->getPanelState();
        panelVelocity = m_physics->getPanelVelocityPixels();
    }

    int const trailGhosts = m_trail.layer ? m_trail.layer->getChildrenCount() : 0;

    std::ostringstream debug;
    debug << std::fixed << std::setprecision(1);
    debug << "sample ms:" << sampleMs
          << " hz:" << sampleHz
          << " substeps:" << m_lastPhysicsSubsteps
          << " acc:" << accumulatorRatio << "\n";
    debug << "player pos(" << playerState.x << "," << playerState.y
          << ") ang:" << playerState.angle
          << " vel(" << playerVelocity.vx << "," << playerVelocity.vy
          << ") speed:" << playerSpeed << "\n";
    debug << "panel on:" << (panelAvailable ? 1 : 0);
    if (panelAvailable) {
        debug << " pos(" << panelState.x << "," << panelState.y
              << ") ang:" << panelState.angle
              << " vel(" << panelVelocity.vx << "," << panelVelocity.vy << ")";
    }
    debug << "\n";
    debug << "impact player trig:" << (m_lastPlayerImpact.triggered ? 1 : 0)
          << " pre:" << m_lastPlayerImpact.preSpeedPx
          << " post:" << m_lastPlayerImpact.postSpeedPx
          << " hit:" << m_lastPlayerImpact.impactSpeedPx << "\n";
    debug << "impact panel trig:" << (m_lastPanelImpact.triggered ? 1 : 0)
          << " pre:" << m_lastPanelImpact.preSpeedPx
          << " post:" << m_lastPanelImpact.postSpeedPx
          << " hit:" << m_lastPanelImpact.impactSpeedPx << "\n";
    debug << "cooldowns hitstop:" << m_impactFlash.hitstopRemaining
          << " white:" << m_impactFlash.whiteFlashRemaining
          << " flashCd:" << m_impactFlash.impactFlashCooldownRemaining
          << " noise:" << m_impactNoise.remaining
          << " starPh:" << m_starBurst.phaseIndex
          << " fireT:" << m_fireAura.time << "\n";
    debug << "trail on:" << (m_trail.active ? 1 : 0)
          << " ghosts:" << trailGhosts
          << " spawnAcc:" << m_trail.spawnAccumulator << "\n";
    debug << "world bodies:" << m_physics->getBodyCount()
          << " joints:" << m_physics->getJointCount()
          << " arbiters:" << m_physics->getArbiterCount()
          << " grab:" << (m_grabActive ? 1 : 0)
          << " panelDrag:" << (m_panelDragActive ? 1 : 0)
          << " vis:" << (m_visualBuilt ? 1 : 0);

    std::string const text = debug.str();
    m_debugLabel->setString(text.c_str());
    if (m_debugLabelBackground && m_debugLabelMeasure) {
        auto const lines = splitDebugLines(text);

        float const scaledTotalHeight = m_debugLabel->getContentSize().height * kDebugLabelFontScale;
        float const lineAdvance =
            lines.empty() ? 0.0f : (scaledTotalHeight / static_cast<float>(lines.size()));
        float const baseX = kDebugLabelMarginX;
        float const topY = m_winSize.height - kDebugLabelMarginY;
        ccColor4F const boxFill = debugLabelBoxFill();
        ccColor3B const boxColor = ccc3(
            colorToByte(boxFill.r),
            colorToByte(boxFill.g),
            colorToByte(boxFill.b)
        );
        GLubyte const boxOpacity = colorToByte(boxFill.a);
        auto* textureSprite = m_debugLabelBackgroundTexture ? m_debugLabelBackgroundTexture->getSprite() : nullptr;
        auto* boxTexture = textureSprite ? textureSprite->getTexture() : nullptr;

        if (m_debugLabelBackgroundSprites.size() < lines.size() && boxTexture) {
            for (size_t i = m_debugLabelBackgroundSprites.size(); i < lines.size(); ++i) {
                auto* sprite = CCSprite::createWithTexture(boxTexture);
                if (!sprite) {
                    continue;
                }
                sprite->setAnchorPoint({0.0f, 0.0f});
                sprite->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
                sprite->setVisible(false);
                m_debugLabelBackground->addChild(sprite);
                m_debugLabelBackgroundSprites.push_back(sprite);
            }
        }

        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
            m_debugLabelMeasure->setString(lines[lineIndex].c_str());
            float const lineWidth =
                m_debugLabelMeasure->getContentSize().width * kDebugLabelFontScale + (kDebugLabelBoxPadX * 2.0f);
            float const minX = baseX - kDebugLabelBoxPadX;
            float const maxX = minX + lineWidth;
            float const lineTop = topY - (static_cast<float>(lineIndex) * lineAdvance);
            float const lineBottom = lineTop - lineAdvance;
            float const maxY = lineTop + kDebugLabelBoxPadY;
            float const minY = lineBottom - kDebugLabelBoxPadY;
            if (lineIndex >= m_debugLabelBackgroundSprites.size()) {
                continue;
            }

            auto* sprite = m_debugLabelBackgroundSprites[lineIndex];
            sprite->setColor(boxColor);
            sprite->setOpacity(boxOpacity);
            sprite->setPosition({minX, minY});
            sprite->setScaleX(maxX - minX);
            sprite->setScaleY(maxY - minY);
            sprite->setVisible(true);
        }

        for (size_t lineIndex = lines.size(); lineIndex < m_debugLabelBackgroundSprites.size(); ++lineIndex) {
            m_debugLabelBackgroundSprites[lineIndex]->setVisible(false);
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

void PhysicsOverlay::update(float dt) {
    if (!m_physics) {
        return;
    }

    decrementCooldowns(dt);
    tryBuildVisualIfNeeded();
    stepPhysicsUnlessHitstop(dt);

    m_debugLabelAccumulator += dt;
    if (m_debugLabelAccumulator >= kDebugLabelUpdateInterval) {
        updateDebugOverlayText(m_debugLabelAccumulator);
        m_debugLabelAccumulator = 0.0f;
    }

    vfx::trail::stopIfSlowOrGrab(m_trail, m_grabActive, m_physics->getPlayerSpeed());

    if (!m_playerRoot || !m_player) {
        vfx::impact_flash::decrementWhiteFlash(m_impactFlash, dt);
        return;
    }

    syncPlayerNodeFromPhysics();
    if (m_physicsMenuVisual && m_physics->hasPanel()) {
        float const alpha = m_physicsAccumulator / kFixedPhysicsDt;
        syncPanelNodeFromPhysics(alpha);
    }
    vfx::trail::updateAndSpawn(m_trail, m_playerRoot, m_player, m_targetSize, m_frameId, m_iconTypeInt, dt);

    overlay_rendering::ImpactFlashMode const flashMode = vfx::impact_flash::currentMode(m_impactFlash);
    vfx::impact_flash::updateBackdrops(flashMode, m_flashBackdrop, m_flashBackdropWhite);

    overlay_rendering::refreshFireAura({
        .fireAura = m_fireAura.sprite,
        .physics = m_physics.get(),
        .dt = dt,
        .impactFlashMode = flashMode,
        .fireTime = &m_fireAura.time,
    });

    auto& playerCapture =
        m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::Player)];
    playerCapture.sourceRoot = m_playerRoot;
    playerCapture.enabled = true;
    playerCapture.velocity = m_physics->getPlayerVelocityPixels();

    auto& menuCapture =
        m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::PhysicsMenu)];
    bool const menuAvailable = m_physicsMenuVisual && m_physics->hasPanel();
    menuCapture.enabled = menuAvailable;
    menuCapture.velocity = menuAvailable ? m_physics->getPanelVelocityPixels() : PhysicsVelocity{};

    vfx::object_motion_blur::refresh(m_objectBlur, flashMode);

    vfx::impact_flash::decrementWhiteFlash(m_impactFlash, dt);
    vfx::impact_noise::update(m_impactNoise, dt, m_impactFlash.whiteFlashRemaining > 0.0f);
    vfx::star_burst::update(m_starBurst, m_impactFlash.whiteFlashRemaining, m_winSize, flashMode);
}

void PhysicsOverlay::onEnter() {
    CCLayer::onEnter();
    handleTouchPriorityWith(this, kPhysicsOverlayTouchPriority, true);
}

void PhysicsOverlay::onExit() {
    endGrab();
    if (m_hitProxy) {
        events::ClickTracker::get()->untrack(m_hitProxy);
        m_hitProxy = nullptr;
    }
    if (m_physicsMenuVisual) {
        if (m_physics) {
            m_physics->destroyPanel();
        }
        m_physicsMenuVisual.reset();
        m_panelDragActive = false;
    }
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_physics.reset();
    if (m_trail.layer) {
        m_trail.layer->removeAllChildrenWithCleanup(true);
        m_trail.layer->removeFromParentAndCleanup(true);
        m_trail.layer = nullptr;
    }
    if (m_playerRoot) {
        m_playerRoot->removeFromParentAndCleanup(true);
        m_playerRoot = nullptr;
    }
    m_debugLabel = nullptr;
    m_debugLabelBackground = nullptr;
    if (m_debugLabelBackgroundTexture) {
        m_debugLabelBackgroundTexture->release();
        m_debugLabelBackgroundTexture = nullptr;
    }
    m_debugLabelBackgroundSprites.clear();
    m_debugLabelMeasure = nullptr;
    if (m_starBurst.layer) {
        m_starBurst.layer->removeFromParentAndCleanup(true);
        m_starBurst.layer = nullptr;
    }
    for (auto*& layerRoot : m_layerRoots) {
        if (!layerRoot) {
            continue;
        }
        layerRoot->removeFromParentAndCleanup(true);
        layerRoot = nullptr;
    }
    m_doubleClickListener.destroy();
    m_tripleClickListener.destroy();
    m_player = nullptr;
    m_objectBlur.finalCompositeSprite = nullptr;
    m_fireAura.sprite = nullptr;
    if (m_impactNoise.composite) {
        m_impactNoise.composite->removeFromParentAndCleanup(true);
        m_impactNoise.composite = nullptr;
    }
    if (m_impactNoise.renderTexture) {
        m_impactNoise.renderTexture->release();
        m_impactNoise.renderTexture = nullptr;
    }
    if (m_impactNoise.sprite) {
        m_impactNoise.sprite->removeFromParentAndCleanup(true);
        m_impactNoise.sprite = nullptr;
    }
    m_objectBlur.whiteFlashSprite = nullptr;
    m_starBurst.sprites = {};
    m_starBurst.phaseIndex = -1;
    if (m_fireAura.program) {
        m_fireAura.program->release();
        m_fireAura.program = nullptr;
    }
    if (m_impactNoise.program) {
        m_impactNoise.program->release();
        m_impactNoise.program = nullptr;
    }
    vfx::object_motion_blur::release(m_objectBlur);
    CCLayer::onExit();
}
