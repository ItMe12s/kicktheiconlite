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

class FireAuraSprite : public cocos2d::CCSprite {
    cocos2d::CCGLProgram* m_fireProg = nullptr;
    GLint m_locVelocity = -1;
    GLint m_locTime = -1;
    GLint m_locIntensity = -1;
    float m_velX = 0.0f;
    float m_velY = 0.0f;
    float m_time = 0.0f;
    float m_intensity = 0.0f;

    void setFireUniforms(
        cocos2d::CCGLProgram* prog,
        GLint locVelocity,
        GLint locTime,
        GLint locIntensity
    );

public:
    void setFireState(float velX, float velY, float time, float intensity);
    void draw() override;

    static FireAuraSprite* create(
        cocos2d::CCTexture2D* tex,
        cocos2d::CCGLProgram* prog,
        GLint locVelocity,
        GLint locTime,
        GLint locIntensity
    );
};

cocos2d::CCGLProgram* createMotionBlurProgram(GLint* outBlurDir);
cocos2d::CCGLProgram* createFireAuraProgram(GLint* outVelocity, GLint* outTime, GLint* outIntensity);
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

struct FireAuraAttachResult {
    bool ok = false;
    FireAuraSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
};

FireAuraAttachResult attachFireAura(cocos2d::CCNode* playerRoot, float auraDiameterPx);

void globalScreenShake(float duration, float strength);

struct MotionBlurRefreshArgs {
    SimplePlayer* player = nullptr;
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

struct FireAuraRefreshArgs {
    FireAuraSprite* fireAura = nullptr;
    PhysicsWorld* physics = nullptr;
    float dt = 0.0f;
    ImpactFlashMode impactFlashMode = ImpactFlashMode::None;
    float* fireTime = nullptr;
};

void refreshFireAura(FireAuraRefreshArgs const& args);

} // namespace overlay_rendering
