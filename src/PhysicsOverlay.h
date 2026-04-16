#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/Event.hpp>

#include <array>
#include <memory>
#include <vector>

#include "OverlayRendering.h"
#include "PhysicsOverlayTuning.h"
#include "PhysicsWorld.h"
#include "vfx/VfxTypes.h"

class SimplePlayer;
class PhysicsMenu;

namespace cocos2d {
class CCNode;
class CCDrawNode;
class CCSprite;
class CCRenderTexture;
class CCGLProgram;
class CCLabelBMFont;
}

class PhysicsOverlay : public cocos2d::CCLayer {
    struct MenuShatterPiece {
        int bodyHandle = -1;
        cocos2d::CCSprite* sprite = nullptr;
    };

    struct MenuShatterState {
        bool active = false;
        float elapsed = 0.0f;
        cocos2d::CCNode* root = nullptr;
        cocos2d::CCNode* captureRoot = nullptr;
        cocos2d::CCRenderTexture* snapshot = nullptr;
        std::vector<MenuShatterPiece> pieces;
    };

    std::unique_ptr<PhysicsWorld> m_physics;
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> m_layerRoots{};
    vfx::ObjectMotionBlurPipelineState m_objectBlur{};
    vfx::StarBurstState m_starBurst{};
    cocos2d::CCNode* m_playerRoot = nullptr;
    SimplePlayer* m_player = nullptr;
    cocos2d::CCSprite* m_hitProxy = nullptr;
    vfx::FireAuraState m_fireAura{};
    vfx::ImpactNoiseState m_impactNoise{};
    vfx::ImpactFlashState m_impactFlash{};

    int m_frameId = 0;
    int m_iconTypeInt = 0;
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    cocos2d::CCSize m_winSize{};
    cocos2d::CCDrawNode* m_flashBackdrop = nullptr;
    cocos2d::CCDrawNode* m_flashBackdropWhite = nullptr;
    cocos2d::CCNode* m_debugLabelBackground = nullptr;
    cocos2d::CCRenderTexture* m_debugLabelBackgroundTexture = nullptr;
    std::vector<cocos2d::CCSprite*> m_debugLabelBackgroundSprites;
    cocos2d::CCLabelBMFont* m_debugLabel = nullptr;
    cocos2d::CCLabelBMFont* m_debugLabelMeasure = nullptr;
    float m_debugLabelAccumulator = 0.0f;
    float m_physicsAccumulator = 0.0f;
    int m_lastPhysicsSubsteps = 0;
    PhysicsImpactEvent m_lastPlayerImpact{};
    PhysicsImpactEvent m_lastPanelImpact{};
    vfx::SandevistanTrailState m_trail{};

    geode::ListenerHandle m_doubleClickListener{};
    geode::ListenerHandle m_tripleClickListener{};

    std::unique_ptr<PhysicsMenu> m_physicsMenuVisual;
    MenuShatterState m_menuShatter{};
    bool m_panelDragActive = false;
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

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    void tryBuildPlayerVisual();
    bool tryBeginGrab(cocos2d::CCPoint const& locationInNode);
    void endGrab();

    void tryOpenPhysicsMenu();
    bool tryBeginPanelGrab(cocos2d::CCPoint const& locationInNode);
    void syncPanelNodeFromPhysics(float alpha);
    void destroyPhysicsMenuVisual();
    bool beginMenuShatter(float impactSpeedPx);
    void updateMenuShatter(float dt);
    void clearMenuShatter();
    void endTouchInteraction();

    void decrementCooldowns(float dt);
    void tryBuildVisualIfNeeded();
    void stepPhysicsUnlessHitstop(float dt);
    void syncPlayerNodeFromPhysics();
    void updateDebugOverlayText(float dt);
};
