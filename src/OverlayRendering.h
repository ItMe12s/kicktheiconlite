#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include <array>

#include "ModTuning.h"
#include "PhysicsWorld.h"

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

inline cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, kOverlayLayerCount> const& roots,
    OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
}

enum class MotionBlurObjectId : int {
    Player = 0,
};

constexpr int kMotionBlurObjectCount = 1;

struct MotionBlurObjectTuning {
    float minBlurSpeedPx = 0.0f;
    float maxBlurSpeedPx = 1.0f;
    float blurUvSpread = 0.0f;
    int blurStepDivisor = 1;
    bool keepBaseVisible = false;
    bool alwaysCaptureWhenEnabled = false;
};

struct MotionBlurObjectSeed {
    MotionBlurObjectId id = MotionBlurObjectId::Player;
    cocos2d::CCNode* sourceRoot = nullptr;
    bool enabled = false;
    MotionBlurObjectTuning tuning = {};
};

struct MotionBlurObjectCapture {
    MotionBlurObjectId id = MotionBlurObjectId::Player;
    cocos2d::CCNode* sourceRoot = nullptr;
    geode::Ref<cocos2d::CCRenderTexture> renderTexture{};
    OverlayShaderSprite* blurSprite = nullptr;
    bool enabled = false;
    MotionBlurObjectTuning tuning = {};
    PhysicsVelocity velocity = {};
};

// ok: shared pipeline (programs + merge root + composites) built successfully
// Per-object render textures / blur OverlayShaderSprite in objects[] are best-effort, missing entries stay disabled
struct ObjectMotionBlurAttachResult {
    bool ok = false;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
    cocos2d::CCRenderTexture* unifiedMergeTexture = nullptr;
    cocos2d::CCNode* mergeRoot = nullptr;
    cocos2d::CCSprite* finalCompositeSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    std::array<MotionBlurObjectCapture, kMotionBlurObjectCount> objects = {};
};

ObjectMotionBlurAttachResult attachObjectMotionBlur(
    cocos2d::CCNode* overlayLayer,
    cocos2d::CCSize captureSize,
    cocos2d::CCSize outputSize,
    int outputZOrder,
    std::array<MotionBlurObjectSeed, kMotionBlurObjectCount> const& objectSeeds
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

struct ObjectMotionBlurRefreshArgs {
    std::array<MotionBlurObjectCapture, kMotionBlurObjectCount>* objects = nullptr;
    cocos2d::CCNode* mergeRoot = nullptr;
    cocos2d::CCRenderTexture* unifiedMergeTexture = nullptr;
    cocos2d::CCSprite* finalCompositeSprite = nullptr;
    cocos2d::CCSprite* whiteFlashSprite = nullptr;
    cocos2d::CCGLProgram* whiteFlashProgram = nullptr;
    cocos2d::CCGLProgram* colorInvertProgram = nullptr;
    ImpactFlashMode impactFlashMode = ImpactFlashMode::None;
};

void refreshObjectMotionBlurComposite(ObjectMotionBlurRefreshArgs const& args);

struct FireAuraRefreshArgs {
    OverlayShaderSprite* fireAura = nullptr;
    PhysicsWorld* physics = nullptr;
    float dt = 0.0f;
    ImpactFlashMode impactFlashMode = ImpactFlashMode::None;
    float* fireTime = nullptr;
};

void refreshFireAura(FireAuraRefreshArgs const& args);

struct ImpactNoiseRefreshArgs {
    OverlayShaderSprite* sprite = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCSprite* compositeSprite = nullptr;
    float dt = 0.0f;
    float extraTimeSkip = 0.0f;
    float* time = nullptr;
    float alpha = 0.0f;
    bool visible = false;
};

void refreshImpactNoise(ImpactNoiseRefreshArgs const& args);

} // namespace overlay_rendering
