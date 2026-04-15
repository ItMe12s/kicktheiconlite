#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/Event.hpp>

#include <array>
#include <memory>

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
}

class PhysicsOverlay : public cocos2d::CCLayer {
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
    cocos2d::CCSize m_blurCaptureSize{};

    int m_frameId = 0;
    int m_iconTypeInt = 0;
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    cocos2d::CCSize m_winSize{};
    cocos2d::CCDrawNode* m_flashBackdrop = nullptr;
    cocos2d::CCDrawNode* m_flashBackdropWhite = nullptr;
    float m_physicsAccumulator = 0.0f;
    vfx::SandevistanTrailState m_trail{};

    geode::ListenerHandle m_doubleClickListener{};
    geode::ListenerHandle m_tripleClickListener{};

    std::unique_ptr<PhysicsMenu> m_physicsMenuVisual;
    bool m_panelDragActive = false;

public:
    CREATE_FUNC(PhysicsOverlay);
    PhysicsOverlay();
    ~PhysicsOverlay() override;
    bool init() override;
    void update(float dt) override;
    void onEnter() override;
    void onExit() override;

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    void tryBuildPlayerVisual();
    bool tryBeginGrab(cocos2d::CCPoint const& locationInNode);
    void endGrab();

    void toggleTestPanel();
    bool tryBeginPanelGrab(cocos2d::CCPoint const& locationInNode);
    void syncPanelNodeFromPhysics(float alpha);

    void decrementCooldowns(float dt);
    void tryBuildVisualIfNeeded();
    void stepPhysicsUnlessHitstop(float dt);
    void syncPlayerNodeFromPhysics();
};
