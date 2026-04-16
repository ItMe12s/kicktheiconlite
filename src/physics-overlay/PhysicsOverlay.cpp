#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/utils/cocos.hpp>
#include <Geode/cocos/platform/CCEGLViewProtocol.h>

#include <algorithm>
#include <cmath>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "PlayerVisual.h"
#include "RuntimeRestart.h"
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

inline void mergeImpactSnapshot(PhysicsImpactEvent& total, PhysicsImpactEvent const& current) {
    total.triggered = total.triggered || current.triggered;
    total.preSpeedPx = std::max(total.preSpeedPx, current.preSpeedPx);
    total.postSpeedPx = std::max(total.postSpeedPx, current.postSpeedPx);
    total.impactSpeedPx = std::max(total.impactSpeedPx, current.impactSpeedPx);
}

cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> const& roots,
    overlay_rendering::OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
}

} // namespace

PhysicsOverlay::PhysicsOverlay() = default;
PhysicsOverlay::~PhysicsOverlay() = default;

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

    auto* uiLayerRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Ui);
    m_menuShatter.root = CCNode::create();
    if (m_menuShatter.root) {
        m_menuShatter.root->setID("menu-shatter-root"_spr);
        m_menuShatter.root->setPosition({0.0f, 0.0f});
        if (uiLayerRoot) {
            uiLayerRoot->addChild(m_menuShatter.root, kPhysicsMenuZOrder);
        } else {
            this->addChild(m_menuShatter.root, kPhysicsMenuZOrder);
        }
    }

    auto* uiRoot = uiLayerRoot;
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

    if (auto* view = CCDirector::get()->getOpenGLView()) {
        if (auto* protocol = typeinfo_cast<cocos2d::CCEGLViewProtocol*>(view)) {
            m_lastGlReady = protocol->isOpenGLReady();
            m_glReadyCheckInitialized = true;
        }
    }

    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, kPhysicsOverlaySchedulerPriority, false);
    runtime_restart::registerPhysicsOverlay(this);
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
        if (
            panelImpact.triggered
            && panelImpact.impactSpeedPx >= kPanelShatterMinImpactSpeed
            && !m_menuShatter.active
            && m_physicsMenuVisual
            && m_physics->hasPanel()
        ) {
            beginMenuShatter(panelImpact.impactSpeedPx);
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

void PhysicsOverlay::update(float dt) {
    if (m_selfDestructRequested || runtime_restart::isRestartRequired()) {
        return;
    }

    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }
    dt = std::min(dt, kMaxSimulationFrameDt);

    // GL context/draw-surface invalidation detection
    // Fullscreen toggles and some lifecycle events that recreate the GL context
    // isOpenGLReady() is the cross-platform signal to bail out before drawing with stale handles
    if (m_glReadyCheckInitialized) {
        if (auto* view = CCDirector::get()->getOpenGLView()) {
            if (auto* protocol = typeinfo_cast<cocos2d::CCEGLViewProtocol*>(view)) {
                bool const glReady = protocol->isOpenGLReady();
                if (m_lastGlReady && !glReady) {
                    runtime_restart::requestFullscreenSelfDestruct("openGL context became unready");
                    return;
                }
                m_lastGlReady = glReady;
            }
        }
    }

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
    updateMenuShatter(dt);
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
    bool const shatterAvailable =
        m_menuShatter.active && m_menuShatter.captureRoot && !m_menuShatter.pieces.empty();
    menuCapture.sourceRoot = menuAvailable
        ? m_physicsMenuVisual->getRoot()
        : (shatterAvailable ? m_menuShatter.captureRoot : nullptr);
    menuCapture.enabled = menuCapture.sourceRoot != nullptr;
    menuCapture.tuning.alwaysCaptureWhenEnabled = shatterAvailable;
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

void PhysicsOverlay::beginFullscreenSelfDestruct() {
    if (m_selfDestructRequested) {
        return;
    }

    m_selfDestructRequested = true;
    m_skipGraphicsCleanup = true;
    this->setTouchEnabled(false);
    this->setVisible(false);
    endTouchInteraction();
    this->stopAllActions();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    this->retain();
    geode::queueInMainThread([this] {
        if (this->getParent()) {
            this->removeFromParentAndCleanup(true);
        }
        this->release();
    });
}

void PhysicsOverlay::onExit() {
    runtime_restart::unregisterPhysicsOverlay(this);
    endGrab();
    if (m_hitProxy) {
        events::ClickTracker::get()->untrack(m_hitProxy);
        m_hitProxy = nullptr;
    }
    if (m_physicsMenuVisual) {
        destroyPhysicsMenuVisual();
    }
    clearMenuShatter();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_physics.reset();
    if (m_trail.layer) {
        m_trail.layer->removeAllChildrenWithCleanup(true);
        m_trail.layer->removeFromParentAndCleanup(true);
        m_trail.layer = nullptr;
    }
    if (m_menuShatter.root) {
        m_menuShatter.root->removeFromParentAndCleanup(true);
        m_menuShatter.root = nullptr;
    }
    m_menuShatter.captureRoot = nullptr;
    if (m_playerRoot) {
        m_playerRoot->removeFromParentAndCleanup(true);
        m_playerRoot = nullptr;
    }
    m_debugLabel = nullptr;
    m_debugLabelBackground = nullptr;
    if (m_debugLabelBackgroundTexture) {
        if (!m_skipGraphicsCleanup) {
            m_debugLabelBackgroundTexture->release();
        }
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
        if (!m_skipGraphicsCleanup) {
            m_impactNoise.renderTexture->release();
        }
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
        if (!m_skipGraphicsCleanup) {
            m_fireAura.program->release();
        }
        m_fireAura.program = nullptr;
    }
    if (m_impactNoise.program) {
        if (!m_skipGraphicsCleanup) {
            m_impactNoise.program->release();
        }
        m_impactNoise.program = nullptr;
    }
    if (!m_skipGraphicsCleanup) {
        vfx::object_motion_blur::release(m_objectBlur);
    } else {
        m_objectBlur = {};
    }
    CCLayer::onExit();
}
