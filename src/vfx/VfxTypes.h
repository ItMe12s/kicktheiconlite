#pragma once

#include <Geode/cocos/base_nodes/CCNode.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/support/CCPointExtension.h>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include <array>

#include "../OverlayRendering.h"
#include "../PhysicsOverlayTuning.h"

namespace vfx {

struct ImpactFlashState {
    float hitstopRemaining = 0.0f;
    float whiteFlashRemaining = 0.0f;
    float impactFlashCooldownRemaining = 0.0f;
};

struct ImpactNoiseState {
    overlay_rendering::ImpactNoiseSprite* sprite = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCSprite* composite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
    float remaining = 0.0f;
    float time = 0.0f;
    float extraTimeSkip = 0.0f;
};

struct StarBurstState {
    cocos2d::CCNode* layer = nullptr;
    std::array<cocos2d::CCSprite*, kStarBurstCount> sprites{};
    int phaseIndex = -1;
};

struct SandevistanTrailState {
    cocos2d::CCNode* layer = nullptr;
    bool active = false;
    float spawnAccumulator = 0.0f;
};

struct FireAuraState {
    overlay_rendering::FireAuraSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
    float time = 0.0f;
};

struct ObjectMotionBlurPipelineState {
    std::array<overlay_rendering::MotionBlurObjectCapture, overlay_rendering::kMotionBlurObjectCount> objects = {};
    cocos2d::CCNode* mergeRoot = nullptr;
    cocos2d::CCRenderTexture* unifiedMergeTexture = nullptr;
    cocos2d::CCSprite* finalCompositeSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
};

} // namespace vfx
