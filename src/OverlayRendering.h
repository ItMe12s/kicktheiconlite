#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include "PhysicsWorld.h"

namespace cocos2d {
class CCSprite;
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
cocos2d::CCGLProgram* createWhiteFlashProgram();
cocos2d::CCGLProgram* createBwBackgroundProgram();

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
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCRenderTexture* bgCaptureRT = nullptr;
    cocos2d::CCSprite* bgSprite = nullptr;
    cocos2d::CCGLProgram* bgProgram = nullptr;
};

MotionBlurAttachResult attachMotionBlur(cocos2d::CCNode* playerRoot, int captureSize);

void globalScreenShake(float duration, float strength);

void captureBwBackground(
    cocos2d::CCLayer*        hostLayer,
    cocos2d::CCRenderTexture* bgCaptureRT,
    cocos2d::CCSprite*        bgSprite,
    bool                      whiteFlashActive
);

void refreshPlayerMotionBlur(
    float dt,
    SimplePlayer* player,
    cocos2d::CCNode* playerRoot,
    cocos2d::CCLayer* hostLayer,
    cocos2d::CCRenderTexture* renderTexture,
    MotionBlurSprite* blurSprite,
    cocos2d::CCSprite* whiteFlashSprite,
    PhysicsWorld* physics,
    int captureSize,
    bool whiteFlashActive
);

} // namespace overlay_rendering
