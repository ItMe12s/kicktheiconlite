#pragma once

#include <Geode/Geode.hpp>
#include <Geode/loader/Event.hpp>

#include <memory>

#include "PhysicsOverlayTuning.h"
#include "PhysicsWorld.h"

class SimplePlayer;

namespace overlay_rendering {
class MotionBlurSprite;
class FireAuraSprite;
class ImpactNoiseSprite;
enum class ImpactFlashMode : int;
}

namespace cocos2d {
class CCNode;
class CCDrawNode;
class CCSprite;
class CCRenderTexture;
class CCGLProgram;
}

class PhysicsOverlay : public cocos2d::CCLayer {
    std::unique_ptr<PhysicsWorld> m_physics;
    cocos2d::CCNode* m_playerRoot = nullptr;
    SimplePlayer* m_player = nullptr;
    cocos2d::CCSprite* m_hitProxy = nullptr;
    overlay_rendering::MotionBlurSprite* m_blurSprite = nullptr;
    overlay_rendering::FireAuraSprite* m_fireAuraSprite = nullptr;
    overlay_rendering::ImpactNoiseSprite* m_impactNoiseSprite = nullptr;
    cocos2d::CCRenderTexture* m_impactNoiseRenderTexture = nullptr;
    cocos2d::CCSprite* m_impactNoiseComposite = nullptr;
    cocos2d::CCRenderTexture* m_renderTexture = nullptr;
    cocos2d::CCGLProgram* m_blurProgram = nullptr;
    cocos2d::CCGLProgram* m_fireAuraProgram = nullptr;
    cocos2d::CCGLProgram* m_whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* m_colorInvertProgram = nullptr;
    cocos2d::CCGLProgram* m_impactNoiseProgram = nullptr;
    int m_captureSize = 0;

    int m_frameId = 0;
    int m_iconTypeInt = 0;
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
    float m_physicsAccumulator = 0.0f;
    float m_fireAuraTime = 0.0f;
    float m_impactNoiseRemaining = 0.0f;
    float m_impactNoiseTime = 0.0f;
    float m_impactNoiseExtraTimeSkip = 0.0f;

    cocos2d::CCSprite* m_starSprites[kStarBurstCount]{};
    int m_starPhaseIndex = -1;

    cocos2d::CCNode* m_trailLayer = nullptr;
    bool m_sandevistanTrailActive = false;
    float m_sandevistanSpawnAccumulator = 0.0f;

    geode::ListenerHandle m_doubleClickListener{};
    geode::ListenerHandle m_tripleClickListener{};

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

    void decrementCooldowns(float dt);
    void tryBuildVisualIfNeeded();
    void stepPhysicsUnlessHitstop(float dt);
    void decrementWhiteFlashRemaining(float dt);
    void syncPlayerNodeFromPhysics();
    overlay_rendering::ImpactFlashMode currentImpactFlashMode() const;
    void updateFlashBackdrops(overlay_rendering::ImpactFlashMode mode);

    int computeCurrentStarPhase() const;
    void repositionStarBurst();
    void applyStarBurstTint();
    void hideAllStars();
    void updateStarBurst();
    void updateSandevistanTrail(float dt);
    void updateImpactNoise(float dt);
};
