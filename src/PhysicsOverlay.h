#pragma once

#include <Geode/Geode.hpp>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "OverlayRendering.h"
#include "OverlayEffectState.h"
#include "PhysicsWorld.h"

class SimplePlayer;

// Top-level CCLayer: owns PhysicsWorld plus VFX state (motion blur pipeline, flashes,
// trail, noise, star burst). m_selfDestructRequested / fullscreen self-destruct path tears
// down GL-backed nodes before invalid context, m_skipGraphicsCleanup avoids draw calls
// during that teardown (see RuntimeRestart, PhysicsOverlayLifecycle).

namespace cocos2d {
class CCNode;
class CCDrawNode;
class CCGLProgram;
class CCLabelBMFont;
}

class PhysicsOverlay : public cocos2d::CCLayer {
    std::unique_ptr<PhysicsWorld> m_physics;
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> m_layerRoots{};
    overlay_effects::ObjectMotionBlurPipelineState m_objectBlur{};
    overlay_effects::StarBurstState m_starBurst{};
    cocos2d::CCNode* m_playerRoot = nullptr;
    SimplePlayer* m_player = nullptr;
    overlay_effects::FireAuraState m_fireAura{};
    overlay_effects::ImpactNoiseState m_impactNoise{};
    overlay_effects::ImpactFlashState m_impactFlash{};

    int m_frameId = 0;
    int m_iconTypeInt = 0;
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    cocos2d::CCSize m_winSize{};
    cocos2d::CCDrawNode* m_flashBackdrop = nullptr;
    overlay_rendering::ImpactFlashMode m_lastFlashBackdropMode = overlay_rendering::ImpactFlashMode::None;
    cocos2d::CCNode* m_debugLabelBackground = nullptr;
    geode::Ref<cocos2d::CCRenderTexture> m_debugLabelBackgroundTexture{};
    std::vector<cocos2d::CCSprite*> m_debugLabelBackgroundSprites;
    cocos2d::CCLabelBMFont* m_debugLabel = nullptr;
    cocos2d::CCLabelBMFont* m_debugLabelMeasure = nullptr;
    float m_debugLabelAccumulator = 0.0f;
    float m_physicsAccumulator = 0.0f;
    int m_lastPhysicsSubsteps = 0;
    PhysicsImpactEvent m_lastPlayerImpact{};
    overlay_effects::SandevistanTrailState m_trail{};
    std::vector<std::string> m_debugLineScratch{};

    bool m_selfDestructRequested = false;
    bool m_skipGraphicsCleanup = false;
    bool m_glReadyCheckInitialized = false;
    bool m_lastGlReady = true;

public:
    CREATE_FUNC(PhysicsOverlay);
    PhysicsOverlay();
    ~PhysicsOverlay() override;
    bool init() override;
    void update(float dt) override;
    void onEnter() override;
    void onExit() override;
    void beginFullscreenSelfDestruct();
    void applyHideModOverlayFromTuning();

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    void tryBuildPlayerVisual();
    bool tryBeginGrab(cocos2d::CCPoint const& locationInNode);
    void endGrab();
    void endTouchInteraction();

    void decrementCooldowns(float dt);
    void tryBuildVisualIfNeeded();
    void stepPhysicsUnlessHitstop(float dt);
    void syncPlayerNodeFromPhysics();
    void detachOverlaySceneNodes();
    void clearOverlayGraphicsRefs();
    bool updateEarlyGuards(float& dt);
    void updateSimulationAndDebug(float dt);
    void updateVisualPipeline(float dt);
    void updateDebugOverlayText(float dt);
    std::vector<std::string> const& splitDebugLinesInto(std::string const& text);
};
