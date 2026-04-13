#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include "PhysicsWorld.h"

namespace cocos2d {
class CCLayerColor;
}

namespace overlay_rendering {

class MotionBlurSprite : public cocos2d::CCSprite {
public:
    cocos2d::CCGLProgram* m_blurProg = nullptr;
    GLint m_locBlurDir = -1;
    float m_stepX = 0.0f;
    float m_stepY = 0.0f;

    void setBlurStep(float x, float y);
    void draw() override;

    static MotionBlurSprite* create(cocos2d::CCTexture2D* tex, cocos2d::CCGLProgram* prog, GLint locBlurDir);
};

cocos2d::CCGLProgram* createMotionBlurProgram(GLint* outBlurDir);

constexpr float kMinBlurSpeedPx = 8.0f;
constexpr float kMaxBlurSpeedPx = 1200.0f;
constexpr float kBlurUvSpread = 0.038f;
constexpr float kBlurCaptureScale = 2.6f;

int captureSizeForTarget(float targetSize);

struct MotionBlurAttachResult {
    bool ok = false;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    GLint locBlurDir = -1;
    MotionBlurSprite* blurSprite = nullptr;
};

MotionBlurAttachResult attachMotionBlur(cocos2d::CCNode* playerRoot, int captureSize);

void globalScreenShake(float duration, float strength);

void runOverlayWhiteFlash(cocos2d::CCLayerColor* layer, float duration, unsigned char peakOpacity);

void refreshPlayerMotionBlur(
    float dt,
    SimplePlayer* player,
    cocos2d::CCNode* playerRoot,
    cocos2d::CCLayer* hostLayer,
    cocos2d::CCRenderTexture* renderTexture,
    MotionBlurSprite* blurSprite,
    PhysicsWorld* physics,
    int captureSize
);

} // namespace overlay_rendering
