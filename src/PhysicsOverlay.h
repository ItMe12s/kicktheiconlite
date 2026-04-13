#pragma once

#include <Geode/Geode.hpp>
#include <Geode/Enums.hpp>
#include <Geode/binding/SimplePlayer.hpp>

#include <memory>
#include <numbers>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PlayerVisual.h"

namespace cocos2d {
class CCDrawNode;
class CCSprite;
}

constexpr int kPhysicsOverlayZOrder = 1000;
constexpr int kPhysicsOverlayTouchPriority = -6767; // CCMenu uses -128.
constexpr int kPhysicsOverlaySchedulerPriority = 0;

constexpr float kGrabRadiusFraction = 2.0f / 3.0f;

constexpr float kImpactHitstopSeconds = 0.15f;
constexpr float kImpactFlashTotalSeconds = 0.15f;
constexpr float kImpactFlashPhaseSeconds = 0.05f;
constexpr float kImpactFlashCooldownSeconds = 0.7f;
constexpr float kWallShakeDuration = 0.25f;

constexpr float kImpactMinSpeed = 1800.0f;
constexpr float kMinWallShakeSpeed = 150.0f;
constexpr float kWallShakeSpeedToStrength = 0.0025f;
constexpr float kMaxWallShakeStrength = 5.0f;

constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;

class PhysicsOverlay : public cocos2d::CCLayer {
    std::unique_ptr<PhysicsWorld> m_physics;
    cocos2d::CCNode* m_playerRoot = nullptr;
    SimplePlayer* m_player = nullptr;
    overlay_rendering::MotionBlurSprite* m_blurSprite = nullptr;
    cocos2d::CCRenderTexture* m_renderTexture = nullptr;
    cocos2d::CCGLProgram* m_blurProgram = nullptr;
    cocos2d::CCGLProgram* m_whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* m_colorInvertProgram = nullptr;
    int m_captureSize = 0;

    int m_frameId = player_visual::kMinPlayerFrameId;
    int m_iconTypeInt = static_cast<int>(IconType::Cube);
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    cocos2d::CCSize m_winSize{};
    float m_hitstopRemaining = 0.0f;
    float m_whiteFlashRemaining = 0.0f;
    float m_impactFlashCooldownRemaining = 0.0f;
    cocos2d::CCSprite* m_whiteFlashSprite = nullptr;
    cocos2d::CCDrawNode* m_flashBackdrop = nullptr;
    cocos2d::CCDrawNode* m_flashBackdropWhite = nullptr;

public:
    CREATE_FUNC(PhysicsOverlay);
    bool init() override;
    void update(float dt) override;
    void onEnter() override;
    void onExit() override;

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    bool ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    bool ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    bool ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    void tryBuildPlayerVisual();
    bool tryBeginGrab(cocos2d::CCPoint const& locationInNode);
    void endGrab();

    void decrementCooldowns(float dt);
    void tryBuildVisualIfNeeded();
    void stepPhysicsUnlessHitstop(float dt);
    void tickWhiteFlashWhenNoPlayer(float dt);
    void syncPlayerNodeFromPhysics();
    overlay_rendering::ImpactFlashMode currentImpactFlashMode() const;
    void updateFlashBackdrops(overlay_rendering::ImpactFlashMode mode);
    void tickWhiteFlashRemaining(float dt);
};
