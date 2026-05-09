#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include <array>

#include "ModTuning.h"
#include "PhysicsTypes.h"

// Full-screen overlay shaders and attach helpers. Layer indices: OverlayLayerId 0=World,
// 1=Trail, 2=Ui (kOverlayLayerCount). OverlayShaderSprite dispatches draw to motion blur,
// impact noise, or fire aura programs. attachPlayerMotionBlur builds the render-texture
// capture + merge + flash stack, attachFireAura / attachImpactNoise hang sprites on the
// overlay tree from PhysicsOverlay init / tryBuildPlayerVisual.

namespace cocos2d {
class CCSprite;
class CCRenderTexture;
class CCSpriteFrame;
}

namespace overlay_rendering {

enum class ShaderSpriteMode {
    MotionBlur,
    ImpactNoise,
    FireAura,
};

class OverlayShaderSprite : public cocos2d::CCSprite {
    ShaderSpriteMode m_mode = ShaderSpriteMode::MotionBlur;
    cocos2d::CCGLProgram* m_program = nullptr;

    GLint m_blurLocDir = -1;
    float m_blurStepX = 0.0f;
    float m_blurStepY = 0.0f;

    GLint m_noiseLocTime = -1;
    GLint m_noiseLocAlpha = -1;
    float m_noiseTime = 0.0f;
    float m_noiseAlpha = 0.0f;

    GLint m_fireLocVelocity = -1;
    GLint m_fireLocTime = -1;
    GLint m_fireLocIntensity = -1;
    GLint m_fireLocColorPrimary = -1;
    GLint m_fireLocColorSecondary = -1;
    float m_fireVelX = 0.0f;
    float m_fireVelY = 0.0f;
    float m_fireTime = 0.0f;
    float m_fireIntensity = 0.0f;
    float m_fireColorPrimaryR = kFireAuraDefaultPrimaryR;
    float m_fireColorPrimaryG = kFireAuraDefaultPrimaryG;
    float m_fireColorPrimaryB = kFireAuraDefaultPrimaryB;
    float m_fireColorSecondaryR = kFireAuraDefaultSecondaryR;
    float m_fireColorSecondaryG = kFireAuraDefaultSecondaryG;
    float m_fireColorSecondaryB = kFireAuraDefaultSecondaryB;

public:
    void setBlurStep(float x, float y);
    void setNoiseState(float time, float alpha);
    void setFireState(float velX, float velY, float time, float intensity);
    void setFireColors(cocos2d::ccColor3B primaryRgb, cocos2d::ccColor3B secondaryRgb);
    void draw() override;

    static OverlayShaderSprite* createMotionBlur(cocos2d::CCTexture2D* tex, cocos2d::CCGLProgram* prog, GLint locBlurDir);
    static OverlayShaderSprite* createImpactNoise(
        cocos2d::CCTexture2D* tex,
        cocos2d::CCGLProgram* prog,
        GLint locTime,
        GLint locAlpha
    );
    static OverlayShaderSprite* createFireAura(
        cocos2d::CCTexture2D* tex,
        cocos2d::CCGLProgram* prog,
        GLint locVelocity,
        GLint locTime,
        GLint locIntensity,
        GLint locColorPrimary,
        GLint locColorSecondary
    );
};

cocos2d::CCGLProgram* createMotionBlurProgram(GLint* outBlurDir);
cocos2d::CCGLProgram* createFireAuraProgram(
    GLint* outVelocity,
    GLint* outTime,
    GLint* outIntensity,
    GLint* outColorPrimary,
    GLint* outColorSecondary
);
cocos2d::CCGLProgram* createWhiteFlashProgram();
cocos2d::CCGLProgram* createColorInvertProgram();
cocos2d::CCGLProgram* createImpactNoiseProgram(GLint* outTime, GLint* outAlpha);

enum class ImpactFlashMode : int {
    None,
    WhiteSilhouette,
    InvertSilhouette,
};

enum class OverlayLayerId : int {
    World = 0,
    Trail = 1,
    Ui = 2,
};

constexpr int kOverlayLayerCount = 3;

struct PlayerMotionBlurCapture {
    cocos2d::CCNode* sourceRoot = nullptr;
    geode::Ref<cocos2d::CCRenderTexture> renderTexture{};
    OverlayShaderSprite* blurSprite = nullptr;
    bool enabled = false;
    PhysicsVelocity velocity = {};
};

struct PlayerMotionBlurAttachResult {
    bool ok = false;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
    cocos2d::CCRenderTexture* unifiedMergeTexture = nullptr;
    cocos2d::CCNode* mergeRoot = nullptr;
    cocos2d::CCSprite* finalCompositeSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    PlayerMotionBlurCapture capture = {};
};

PlayerMotionBlurAttachResult attachPlayerMotionBlur(
    cocos2d::CCNode* overlayLayer,
    cocos2d::CCSize captureSize,
    cocos2d::CCSize outputSize,
    int outputZOrder,
    cocos2d::CCNode* sourceRoot
);

struct FireAuraAttachResult {
    bool ok = false;
    OverlayShaderSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
};

FireAuraAttachResult attachFireAura(cocos2d::CCNode* playerRoot, float auraDiameterPx);

struct ImpactNoiseAttachResult {
    bool ok = false;
    OverlayShaderSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCSprite* compositeSprite = nullptr;
};

ImpactNoiseAttachResult attachImpactNoise(cocos2d::CCNode* overlayLayer, cocos2d::CCSize winSize);

void refreshPlayerMotionBlurComposite(
    PlayerMotionBlurCapture* capture,
    cocos2d::CCNode* mergeRoot,
    cocos2d::CCRenderTexture* unifiedMergeTexture,
    cocos2d::CCSprite* finalCompositeSprite,
    cocos2d::CCSprite* whiteFlashSprite,
    cocos2d::CCGLProgram* whiteFlashProgram,
    cocos2d::CCGLProgram* colorInvertProgram,
    ImpactFlashMode impactFlashMode
);

void refreshFireAura(
    OverlayShaderSprite* fireAura,
    PhysicsVelocity playerVelocity,
    float dt,
    ImpactFlashMode impactFlashMode,
    float* fireTime
);

void refreshImpactNoise(
    OverlayShaderSprite* sprite,
    cocos2d::CCRenderTexture* renderTexture,
    cocos2d::CCSprite* compositeSprite,
    float dt,
    float extraTimeSkip,
    float* time,
    float alpha,
    bool visible
);

namespace detail {

cocos2d::CCTexture2D* createOneByOneWhiteTexture();

}

} // namespace overlay_rendering
