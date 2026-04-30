#include "PhysicsOverlay.h"

#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PlayerVisual.h"
#include "vfx/ObjectMotionBlurPipeline.h"
#include "vfx/StarBurst.h"

using namespace geode::prelude;

namespace {

cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> const& roots,
    overlay_rendering::OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
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

    cocos2d::CCSize const blurCaptureSize = m_winSize;
    std::array<overlay_rendering::MotionBlurObjectSeed, overlay_rendering::kMotionBlurObjectCount> seeds{};
    seeds[0] = overlay_rendering::MotionBlurObjectSeed{
        .id = overlay_rendering::MotionBlurObjectId::Player,
        .sourceRoot = pr.root,
        .enabled = true,
        .tuning = {
            .minBlurSpeedPx = kPlayerMinBlurSpeedPx,
            .maxBlurSpeedPx = kPlayerMaxBlurSpeedPx,
            .blurUvSpread = kPlayerBlurUvSpread,
            .blurStepDivisor = kPlayerBlurStepDivisor,
            .keepBaseVisible = kPlayerKeepBaseVisible,
            .alwaysCaptureWhenEnabled = false,
        },
    };

    if (!vfx::object_motion_blur::attach(
            m_objectBlur,
            this,
            blurCaptureSize,
            m_winSize,
            kUnifiedBlurCompositeZOrder,
            seeds
        )) {
        pr.root->removeFromParentAndCleanup(true);
        return;
    }

    m_playerRoot = pr.root;
    auto* worldRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::World);
    if (worldRoot) {
        Ref<cocos2d::CCNode> hold(m_playerRoot);
        this->removeChild(m_playerRoot, false);
        worldRoot->addChild(m_playerRoot, kPlayerRootZOrder);
    } else {
        m_playerRoot->setZOrder(kPlayerRootZOrder);
    }
    m_player = pr.player;

    auto const fa = overlay_rendering::attachFireAura(pr.root, m_targetSize * kFireAuraDiameterScale);
    if (fa.ok) {
        m_fireAura.sprite = fa.sprite;
        m_fireAura.program = Ref<CCGLProgram>::adopt(fa.program);
    }

    vfx::star_burst::createSprites(m_starBurst);

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

void PhysicsOverlay::endTouchInteraction() {
    endGrab();
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    (void)event;
    if (!m_physics) {
        return false;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    return tryBeginGrab(p);
}

void PhysicsOverlay::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    (void)event;
    if (!m_physics) {
        return;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    if (!m_grabActive) {
        return;
    }
    m_physics->setDragTargetPixels(p.x, p.y);
}

void PhysicsOverlay::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endTouchInteraction();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endTouchInteraction();
}
