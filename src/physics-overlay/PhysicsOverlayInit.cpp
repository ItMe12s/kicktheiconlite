#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/utils/cocos.hpp>
#include <Geode/cocos/platform/CCEGLViewProtocol.h>

#include "OverlayRendering.h"
#include "ModTuning.h"
#include "PlayerVisual.h"
#include "RuntimeRestart.h"
#include "PhysicsWorld.h"

using namespace geode::prelude;

namespace {

inline ccColor4F flashBackdropBlackFill() {
    return ccc4f(0, 0, 0, 1);
}

inline ccColor4F flashBackdropBorderTransparent() {
    return ccc4f(0, 0, 0, 0);
}

} // namespace

// Three child roots (World, Trail, Ui) separate capture, trail ghosts, and debug UI z-order.

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

    auto* uiLayerRoot = m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::Ui)];

    auto* uiRoot = uiLayerRoot;
    if (uiRoot) {
        float const baseX = kDebugLabelMarginX;
        float const baseY = m_winSize.height - kDebugLabelMarginY;

        m_debugLabelBackground = CCNode::create();
        if (m_debugLabelBackground) {
            m_debugLabelBackground->setID("debug-overlay-line-backgrounds"_spr);
            m_debugLabelBackground->setPosition({0.0f, 0.0f});
            m_debugLabelBackground->setVisible(kDebugLabelEnabled);
            uiRoot->addChild(m_debugLabelBackground, kDebugLabelBackgroundZOrder);

            if (CCRenderTexture* dbgRt = CCRenderTexture::create(1, 1)) {
                m_debugLabelBackgroundTexture = dbgRt;
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
            m_debugLabel->setVisible(kDebugLabelEnabled);
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

    auto const noiseAttach = overlay_rendering::attachImpactNoise(this, m_winSize);
    if (noiseAttach.ok) {
        m_impactNoise.sprite = noiseAttach.sprite;
        m_impactNoise.program = Ref<CCGLProgram>::adopt(noiseAttach.program);
        m_impactNoise.renderTexture = Ref<CCRenderTexture>::adopt(noiseAttach.renderTexture);
        m_impactNoise.composite = noiseAttach.compositeSprite;
    }

    m_trail.layer = CCNode::create();
    if (m_trail.layer) {
        m_trail.layer->setID("sandevistan-trail-layer"_spr);
        m_trail.layer->setPosition({0, 0});
        auto* trailRoot = m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::Trail)];
        if (trailRoot) {
            trailRoot->addChild(m_trail.layer, kSandevistanTrailLayerZOrder);
        } else {
            log::warn("missing trail root, trail blur bypassed");
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
