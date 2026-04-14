#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include "PhysicsWorld.h"

namespace cocos2d {
class CCSprite;
}

namespace overlay_rendering {

constexpr float kMinBlurSpeedPx = 8.0f;
constexpr float kMaxBlurSpeedPx = 1200.0f;
constexpr float kBlurUvSpread = 0.038f;
constexpr float kBlurCaptureScale = 2.6f;

constexpr float kScreenShakeCooldownExtraSeconds = 0.1f;

class MotionBlurSprite : public cocos2d::CCSprite {
    cocos2d::CCGLProgram* m_blurProg = nullptr;
    GLint m_locBlurDir = -1;
    float m_stepX = 0.0f;
    float m_stepY = 0.0f;

    void setBlurUniforms(cocos2d::CCGLProgram* prog, GLint locBlurDir);

public:
    void setBlurStep(float x, float y);
    void draw() override;

    static MotionBlurSprite* create(cocos2d::CCTexture2D* tex, cocos2d::CCGLProgram* prog, GLint locBlurDir);
};

cocos2d::CCGLProgram* createMotionBlurProgram(GLint* outBlurDir);
cocos2d::CCGLProgram* createWhiteFlashProgram();
cocos2d::CCGLProgram* createColorInvertProgram();

enum class ImpactFlashMode {
    None,
    WhiteSilhouette,
    InvertSilhouette,
};

int captureSizeForTarget(float targetSize);

struct MotionBlurAttachResult {
    bool ok = false;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    GLint locBlurDir = -1;
    MotionBlurSprite* blurSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
};

MotionBlurAttachResult attachMotionBlur(cocos2d::CCNode* playerRoot, int captureSize);

void globalScreenShake(float duration, float strength);

struct MotionBlurRefreshArgs {
    float dt = 0.0f;
    SimplePlayer* player = nullptr;
    cocos2d::CCNode* playerRoot = nullptr;
    cocos2d::CCLayer* hostLayer = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    MotionBlurSprite* blurSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
    PhysicsWorld* physics = nullptr;
    int captureSize = 0;
    ImpactFlashMode impactFlashMode = ImpactFlashMode::None;
};

void refreshPlayerMotionBlur(MotionBlurRefreshArgs const& args);

} // namespace overlay_rendering
