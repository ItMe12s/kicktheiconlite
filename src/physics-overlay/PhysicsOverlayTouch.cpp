#include "PhysicsOverlay.h"

#include <Geode/utils/cocos.hpp>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PlayerVisual.h"
#include "StarBurst.h"

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

    cocos2d::CCSize const blurCaptureSize = m_winSize;

    auto const blurResult = overlay_rendering::attachPlayerMotionBlur(
        this,
        blurCaptureSize,
        m_winSize,
        kUnifiedBlurCompositeZOrder,
        pr.root
    );
    if (!blurResult.ok) {
        pr.root->removeFromParentAndCleanup(true);
        return;
    }
    m_objectBlur.player = blurResult.capture;
    m_objectBlur.mergeRoot = Ref<cocos2d::CCNode>(blurResult.mergeRoot);
    m_objectBlur.unifiedMergeTexture = Ref<cocos2d::CCRenderTexture>::adopt(blurResult.unifiedMergeTexture);
    m_objectBlur.finalCompositeSprite = Ref<cocos2d::CCSprite>(blurResult.finalCompositeSprite);
    m_objectBlur.whiteFlashSprite = Ref<cocos2d::CCSprite>(blurResult.whiteFlashSprite);
    m_objectBlur.blurProgram = Ref<cocos2d::CCGLProgram>::adopt(blurResult.blurProgram);
    m_objectBlur.whiteFlashProgram = Ref<cocos2d::CCGLProgram>::adopt(blurResult.whiteFlashProgram);
    m_objectBlur.colorInvertProgram = Ref<cocos2d::CCGLProgram>::adopt(blurResult.colorInvertProgram);

    m_playerRoot = pr.root;
    auto* worldRoot = m_layerRoots[static_cast<size_t>(overlay_rendering::OverlayLayerId::World)];
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

    star_burst::createSprites(m_starBurst);

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
    star_burst::reset(m_starBurst);
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
