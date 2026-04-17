#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>

#include <array>
#include <string>
#include <vector>

#include "ModTuning.h"
#include "PhysicsWorld.h"
#include "extras/GlyphTextRendering.h"

namespace cocos2d {
class CCSprite;
class CCRenderTexture;
class CCSpriteFrame;
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

class ImpactNoiseSprite : public cocos2d::CCSprite {
    cocos2d::CCGLProgram* m_noiseProg = nullptr;
    GLint m_locTime = -1;
    GLint m_locAlpha = -1;
    float m_time = 0.0f;
    float m_alpha = 0.0f;

    void setNoiseUniforms(cocos2d::CCGLProgram* prog, GLint locTime, GLint locAlpha);

public:
    void setNoiseState(float time, float alpha);
    void draw() override;

    static ImpactNoiseSprite* create(cocos2d::CCTexture2D* tex, cocos2d::CCGLProgram* prog, GLint locTime, GLint locAlpha);
};

class FireAuraSprite : public cocos2d::CCSprite {
    cocos2d::CCGLProgram* m_fireProg = nullptr;
    GLint m_locVelocity = -1;
    GLint m_locTime = -1;
    GLint m_locIntensity = -1;
    GLint m_locColorPrimary = -1;
    GLint m_locColorSecondary = -1;
    float m_velX = 0.0f;
    float m_velY = 0.0f;
    float m_time = 0.0f;
    float m_intensity = 0.0f;
    float m_colorPrimaryR = kFireAuraDefaultPrimaryR;
    float m_colorPrimaryG = kFireAuraDefaultPrimaryG;
    float m_colorPrimaryB = kFireAuraDefaultPrimaryB;
    float m_colorSecondaryR = kFireAuraDefaultSecondaryR;
    float m_colorSecondaryG = kFireAuraDefaultSecondaryG;
    float m_colorSecondaryB = kFireAuraDefaultSecondaryB;

    void setFireUniforms(
        cocos2d::CCGLProgram* prog,
        GLint locVelocity,
        GLint locTime,
        GLint locIntensity,
        GLint locColorPrimary,
        GLint locColorSecondary
    );

public:
    void setFireState(float velX, float velY, float time, float intensity);
    void setFireColors(cocos2d::ccColor3B primaryRgb, cocos2d::ccColor3B secondaryRgb);
    void draw() override;

    static FireAuraSprite* create(
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

enum class MotionBlurObjectId : int {
    Player = 0,
    PhysicsMenu = 1,
};

constexpr int kMotionBlurObjectCount = 2;

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
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    MotionBlurSprite* blurSprite = nullptr;
    bool enabled = false;
    MotionBlurObjectTuning tuning = {};
    PhysicsVelocity velocity = {};
};

// ok: shared pipeline (programs + merge root + composites) built successfully
// Per-object render textures / MotionBlurSprite in objects[] are best-effort, missing entries stay disabled
struct ObjectMotionBlurAttachResult {
    bool ok = false;
    cocos2d::CCGLProgram* blurProgram = nullptr;
    GLint locBlurDir = -1;
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
    FireAuraSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
};

FireAuraAttachResult attachFireAura(cocos2d::CCNode* playerRoot, float auraDiameterPx);

struct ImpactNoiseAttachResult {
    bool ok = false;
    ImpactNoiseSprite* sprite = nullptr;
    cocos2d::CCGLProgram* program = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCSprite* compositeSprite = nullptr;
};

ImpactNoiseAttachResult attachImpactNoise(cocos2d::CCNode* overlayLayer, cocos2d::CCSize winSize);

void globalScreenShake(float duration, float strength);

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
    FireAuraSprite* fireAura = nullptr;
    PhysicsWorld* physics = nullptr;
    float dt = 0.0f;
    ImpactFlashMode impactFlashMode = ImpactFlashMode::None;
    float* fireTime = nullptr;
};

void refreshFireAura(FireAuraRefreshArgs const& args);

struct ImpactNoiseRefreshArgs {
    ImpactNoiseSprite* sprite = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    cocos2d::CCSprite* compositeSprite = nullptr;
    float dt = 0.0f;
    float extraTimeSkip = 0.0f;
    float* time = nullptr;
    float alpha = 0.0f;
    bool visible = false;
};

void refreshImpactNoise(ImpactNoiseRefreshArgs const& args);

using GlyphTextAlign = extras::glyph_text::TextAlign;
using GlyphUnsupportedPolicy = extras::glyph_text::UnsupportedPolicy;
using GlyphWrapMode = extras::glyph_text::WrapMode;
using GlyphTextOptions = extras::glyph_text::TextOptions;
using GlyphLayoutGlyph = extras::glyph_text::LayoutGlyph;
using GlyphLayoutLine = extras::glyph_text::LayoutLine;
using GlyphLayoutResult = extras::glyph_text::LayoutResult;

// Returns glyph placements and bounds, No nodes/textures are created
GlyphLayoutResult layoutGlyphText(std::string const& text, GlyphTextOptions const& options = {});

// Returns autoreleased sprite nodes, one per laid out glyph
std::vector<cocos2d::CCSprite*> buildGlyphTextSprites(std::string const& text, GlyphTextOptions const& options = {});

// Returns an autoreleased render texture containing the full composed text
cocos2d::CCRenderTexture* buildGlyphTextTexture(std::string const& text, GlyphTextOptions const& options = {});

// Returns an autoreleased sprite built from the composed text texture
cocos2d::CCSprite* buildGlyphTextSprite(std::string const& text, GlyphTextOptions const& options = {});

} // namespace overlay_rendering
