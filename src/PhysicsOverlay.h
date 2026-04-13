#pragma once

#include <Geode/Geode.hpp>
#include <Geode/Enums.hpp>
#include <Geode/binding/SimplePlayer.hpp>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PlayerVisual.h"

namespace cocos2d {
class CCSprite;
}

// CCMenu uses -128.
constexpr int kPhysicsOverlayTouchPriority = -6767;

constexpr float kWallShakeDuration = 0.25f;
constexpr float kMaxWallShakeStrength = 5.0f;
constexpr float kWallShakeSpeedToStrength = 0.0025f;
constexpr float kMinWallShakeSpeed = 150.0f;

constexpr float kImpactMinSpeed = 1600.0f;
constexpr float kImpactHitstopSeconds = 0.075f;
constexpr float kImpactWhiteFlashSeconds = 0.05f;

constexpr float kGrabRadiusFraction = 2.0f / 3.0f;
constexpr float kRadToDeg = 180.0f / 3.14159265f;
constexpr int kPhysicsOverlaySchedulerPriority = 0;
constexpr int kPhysicsOverlayZOrder = 1000;

class PhysicsOverlay : public cocos2d::CCLayer {
    PhysicsWorld* m_physics = nullptr;
    cocos2d::CCNode* m_playerRoot = nullptr;
    SimplePlayer* m_player = nullptr;
    overlay_rendering::MotionBlurSprite* m_blurSprite = nullptr;
    cocos2d::CCRenderTexture* m_renderTexture = nullptr;
    cocos2d::CCGLProgram* m_blurProgram = nullptr;
    cocos2d::CCGLProgram* m_whiteFlashProgram = nullptr;
    int m_captureSize = 0;

    int m_frameId = player_visual::kMinPlayerFrameId;
    int m_iconTypeInt = static_cast<int>(IconType::Cube);
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    cocos2d::CCSize m_winSize{};
    float m_hitstopRemaining = 0.0f;
    float m_whiteFlashRemaining = 0.0f;
    cocos2d::CCSprite* m_whiteFlashSprite = nullptr;

public:
    CREATE_FUNC(PhysicsOverlay);
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
};
