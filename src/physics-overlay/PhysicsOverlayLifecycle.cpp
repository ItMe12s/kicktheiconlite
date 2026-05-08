#include "PhysicsOverlay.h"

#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/utils/cocos.hpp>

#include "RuntimeRestart.h"

using namespace geode::prelude;

void PhysicsOverlay::detachOverlaySceneNodes() {
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
    if (m_skipGraphicsCleanup) {
        (void)m_debugLabelBackgroundTexture.take();
    }
    m_debugLabelBackgroundTexture = nullptr;
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
    m_player = nullptr;
    m_fireAura.sprite = nullptr;
    if (m_impactNoise.composite) {
        m_impactNoise.composite->removeFromParentAndCleanup(true);
        m_impactNoise.composite = nullptr;
    }
    if (m_impactNoise.sprite) {
        m_impactNoise.sprite->removeFromParentAndCleanup(true);
        m_impactNoise.sprite = nullptr;
    }
    m_starBurst.sprites = {};
    m_starBurst.phaseIndex = -1;
}

void PhysicsOverlay::clearOverlayGraphicsRefs() {
    if (m_skipGraphicsCleanup) {
        (void)m_objectBlur.finalCompositeSprite.take();
        (void)m_objectBlur.whiteFlashSprite.take();
        (void)m_impactNoise.renderTexture.take();
        (void)m_fireAura.program.take();
        (void)m_impactNoise.program.take();
        (void)m_objectBlur.player.renderTexture.take();
        m_objectBlur.player.blurSprite = nullptr;
        m_objectBlur.player.sourceRoot = nullptr;
        m_objectBlur.player.enabled = false;
        m_objectBlur.player.velocity = {};
        (void)m_objectBlur.mergeRoot.take();
        (void)m_objectBlur.unifiedMergeTexture.take();
        (void)m_objectBlur.blurProgram.take();
        (void)m_objectBlur.whiteFlashProgram.take();
        (void)m_objectBlur.colorInvertProgram.take();
    } else {
        m_objectBlur.finalCompositeSprite = nullptr;
        m_objectBlur.whiteFlashSprite = nullptr;
        m_impactNoise.renderTexture = nullptr;
        m_fireAura.program = nullptr;
        m_impactNoise.program = nullptr;
        m_objectBlur = {};
    }
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
    Ref<PhysicsOverlay> keepAlive(this);
    geode::queueInMainThread([keepAlive] {
        if (keepAlive->getParent()) {
            keepAlive->removeFromParentAndCleanup(true);
        }
    });
}

void PhysicsOverlay::onExit() {
    runtime_restart::unregisterPhysicsOverlay(this);
    endGrab();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    m_physics.reset();
    detachOverlaySceneNodes();
    clearOverlayGraphicsRefs();
    CCLayer::onExit();
}
