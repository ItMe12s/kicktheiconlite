#include "OverlayRendering.h"
#include "OverlayShaders.h"
#include "ModTuning.h"
#include "PhysicsWorld.h"

#include <Geode/binding/GameManager.hpp>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>

#include <algorithm>
#include <cmath>
#include <memory>

using namespace geode::prelude;

namespace overlay_rendering {

namespace {

CCGLProgram* createLinkedProgram(char const* vert, char const* frag) {
    Ref<CCGLProgram> p = Ref<CCGLProgram>::adopt(new CCGLProgram());
    if (!p->initWithVertexShaderByteArray(vert, frag)) {
        return nullptr;
    }
    p->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    p->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    p->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    if (!p->link()) {
        return nullptr;
    }
    p->updateUniforms();
    p->retain();
    return p.take();
}

CCTexture2D* createOneByOneWhiteTexture() {
    static unsigned char const pixels[4] = {255, 255, 255, 255};
    auto tex = std::unique_ptr<CCTexture2D>(new CCTexture2D());
    if (!tex->initWithData(
            pixels,
            kCCTexture2DPixelFormat_RGBA8888,
            1,
            1,
            CCSizeMake(1, 1)
        )) {
        return nullptr;
    }
    tex->autorelease();
    return tex.release();
}

void resetObjectVisualState(MotionBlurObjectCapture& object) {
    if (object.sourceRoot) {
        object.sourceRoot->setVisible(true);
    }
    if (object.blurSprite) {
        object.blurSprite->setVisible(false);
    }
}

} // namespace

void OverlayShaderSprite::setBlurStep(float x, float y) {
    m_blurStepX = x;
    m_blurStepY = y;
}

void OverlayShaderSprite::setNoiseState(float time, float alpha) {
    m_noiseTime = time;
    m_noiseAlpha = alpha;
}

void OverlayShaderSprite::setFireState(float velX, float velY, float time, float intensity) {
    m_fireVelX = velX;
    m_fireVelY = velY;
    m_fireTime = time;
    m_fireIntensity = intensity;
}

void OverlayShaderSprite::setFireColors(ccColor3B primaryRgb, ccColor3B secondaryRgb) {
    constexpr float kInv = 1.0f / 255.0f;
    m_fireColorPrimaryR = static_cast<float>(primaryRgb.r) * kInv;
    m_fireColorPrimaryG = static_cast<float>(primaryRgb.g) * kInv;
    m_fireColorPrimaryB = static_cast<float>(primaryRgb.b) * kInv;
    m_fireColorSecondaryR = static_cast<float>(secondaryRgb.r) * kInv;
    m_fireColorSecondaryG = static_cast<float>(secondaryRgb.g) * kInv;
    m_fireColorSecondaryB = static_cast<float>(secondaryRgb.b) * kInv;
}

void OverlayShaderSprite::draw() {
    switch (m_mode) {
    case ShaderSpriteMode::MotionBlur:
        if (m_program && m_blurLocDir >= 0) {
            m_program->use();
            m_program->setUniformLocationWith2f(m_blurLocDir, m_blurStepX, m_blurStepY);
        }
        break;
    case ShaderSpriteMode::ImpactNoise:
        if (m_program && m_noiseLocTime >= 0 && m_noiseLocAlpha >= 0) {
            m_program->use();
            m_program->setUniformLocationWith1f(m_noiseLocTime, m_noiseTime);
            m_program->setUniformLocationWith1f(m_noiseLocAlpha, m_noiseAlpha);
        }
        break;
    case ShaderSpriteMode::FireAura:
        if (
            m_program && m_fireLocVelocity >= 0 && m_fireLocTime >= 0 && m_fireLocIntensity >= 0
            && m_fireLocColorPrimary >= 0 && m_fireLocColorSecondary >= 0
        ) {
            m_program->use();
            m_program->setUniformLocationWith2f(m_fireLocVelocity, m_fireVelX, m_fireVelY);
            m_program->setUniformLocationWith1f(m_fireLocTime, m_fireTime);
            m_program->setUniformLocationWith1f(m_fireLocIntensity, m_fireIntensity);
            m_program->setUniformLocationWith3f(
                m_fireLocColorPrimary,
                m_fireColorPrimaryR,
                m_fireColorPrimaryG,
                m_fireColorPrimaryB
            );
            m_program->setUniformLocationWith3f(
                m_fireLocColorSecondary,
                m_fireColorSecondaryR,
                m_fireColorSecondaryG,
                m_fireColorSecondaryB
            );
        }
        break;
    }
    CCSprite::draw();
}

OverlayShaderSprite* OverlayShaderSprite::createMotionBlur(CCTexture2D* tex, CCGLProgram* prog, GLint locBlurDir) {
    auto* s = new OverlayShaderSprite();
    s->m_mode = ShaderSpriteMode::MotionBlur;
    s->m_program = prog;
    s->m_blurLocDir = locBlurDir;
    if (s->initWithTexture(tex)) {
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

OverlayShaderSprite* OverlayShaderSprite::createImpactNoise(CCTexture2D* tex, CCGLProgram* prog, GLint locTime, GLint locAlpha) {
    auto* s = new OverlayShaderSprite();
    s->m_mode = ShaderSpriteMode::ImpactNoise;
    s->m_program = prog;
    s->m_noiseLocTime = locTime;
    s->m_noiseLocAlpha = locAlpha;
    if (s->initWithTexture(tex)) {
        s->setColor(ccc3(255, 255, 255));
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

OverlayShaderSprite* OverlayShaderSprite::createFireAura(
    CCTexture2D* tex,
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity,
    GLint locColorPrimary,
    GLint locColorSecondary
) {
    auto* s = new OverlayShaderSprite();
    s->m_mode = ShaderSpriteMode::FireAura;
    s->m_program = prog;
    s->m_fireLocVelocity = locVelocity;
    s->m_fireLocTime = locTime;
    s->m_fireLocIntensity = locIntensity;
    s->m_fireLocColorPrimary = locColorPrimary;
    s->m_fireLocColorSecondary = locColorSecondary;
    if (s->initWithTexture(tex)) {
        s->setColor(ccc3(255, 255, 255));
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

CCGLProgram* createMotionBlurProgram(GLint* outBlurDir) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kMotionBlurFrag);
    if (!p) {
        return nullptr;
    }
    *outBlurDir = p->getUniformLocationForName("u_blurDir");
    return p;
}

CCGLProgram* createWhiteFlashProgram() {
    return createLinkedProgram(shaders::kMotionBlurVert, shaders::kWhiteFlashFrag);
}

CCGLProgram* createColorInvertProgram() {
    return createLinkedProgram(shaders::kMotionBlurVert, shaders::kColorInvertFrag);
}

CCGLProgram* createImpactNoiseProgram(GLint* outTime, GLint* outAlpha) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kImpactNoiseFrag);
    if (!p) {
        return nullptr;
    }
    *outTime = p->getUniformLocationForName("u_time");
    *outAlpha = p->getUniformLocationForName("u_alpha");
    return p;
}

CCGLProgram* createFireAuraProgram(
    GLint* outVelocity,
    GLint* outTime,
    GLint* outIntensity,
    GLint* outColorPrimary,
    GLint* outColorSecondary
) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kFireAuraFrag);
    if (!p) {
        return nullptr;
    }
    *outVelocity = p->getUniformLocationForName("u_velocity");
    *outTime = p->getUniformLocationForName("u_time");
    *outIntensity = p->getUniformLocationForName("u_intensity");
    *outColorPrimary = p->getUniformLocationForName("u_colorPrimary");
    *outColorSecondary = p->getUniformLocationForName("u_colorSecondary");
    return p;
}

ObjectMotionBlurAttachResult attachObjectMotionBlur(
    CCNode* overlayLayer,
    CCSize captureSize,
    CCSize outputSize,
    int outputZOrder,
    std::array<MotionBlurObjectSeed, kMotionBlurObjectCount> const& objectSeeds
) {
    ObjectMotionBlurAttachResult out{};
    if (!overlayLayer || captureSize.width <= 0.0f || captureSize.height <= 0.0f || outputSize.width <= 0.0f
        || outputSize.height <= 0.0f) {
        return out;
    }

    struct Rollback {
        Ref<CCRenderTexture> unifiedTexture{};
        Ref<CCGLProgram> blurProgram{};
        Ref<CCGLProgram> whiteFlashProgram{};
        Ref<CCGLProgram> colorInvertProgram{};
        CCSprite* finalComposite = nullptr;
        CCSprite* whiteFlashSprite = nullptr;
        CCNode* mergeRoot = nullptr;
        bool armed = true;
        ~Rollback() {
            if (!armed) {
                return;
            }
            if (mergeRoot) {
                mergeRoot->removeFromParentAndCleanup(true);
            }
            if (whiteFlashSprite) {
                whiteFlashSprite->removeFromParentAndCleanup(true);
            }
            if (finalComposite) {
                finalComposite->removeFromParentAndCleanup(true);
            }
        }
        void disarm() { armed = false; }
    } rb;

    auto* unifiedTextureRaw = CCRenderTexture::create(
        static_cast<int>(std::ceil(captureSize.width)),
        static_cast<int>(std::ceil(captureSize.height)),
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (!unifiedTextureRaw) {
        return out;
    }
    rb.unifiedTexture = unifiedTextureRaw;
    CCRenderTexture* unifiedTexture = rb.unifiedTexture;

    auto* finalComposite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!finalComposite) {
        return out;
    }
    finalComposite->setID("object-blur-final-composite"_spr);
    finalComposite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    finalComposite->setAnchorPoint({0.0f, 0.0f});
    finalComposite->setPosition({0.0f, 0.0f});
    finalComposite->setVisible(false);
    finalComposite->setFlipY(true);
    {
        float const cw = finalComposite->getContentSize().width;
        float const ch = finalComposite->getContentSize().height;
        finalComposite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        finalComposite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    overlayLayer->addChild(finalComposite, outputZOrder);
    rb.finalComposite = finalComposite;

    auto* whiteFlashSprite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!whiteFlashSprite) {
        return out;
    }
    whiteFlashSprite->setID("object-blur-flash-sprite"_spr);
    whiteFlashSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    whiteFlashSprite->setAnchorPoint({0.0f, 0.0f});
    whiteFlashSprite->setPosition({0.0f, 0.0f});
    whiteFlashSprite->setVisible(false);
    whiteFlashSprite->setFlipY(true);
    {
        float const cw = whiteFlashSprite->getContentSize().width;
        float const ch = whiteFlashSprite->getContentSize().height;
        whiteFlashSprite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        whiteFlashSprite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    overlayLayer->addChild(whiteFlashSprite, outputZOrder);
    rb.whiteFlashSprite = whiteFlashSprite;

    GLint locBlurDir = -1;
    auto* blurProgramRaw = createMotionBlurProgram(&locBlurDir);
    if (!blurProgramRaw || locBlurDir < 0) {
        if (blurProgramRaw) {
            blurProgramRaw->release();
        }
        return out;
    }
    rb.blurProgram = Ref<CCGLProgram>::adopt(blurProgramRaw);
    CCGLProgram* blurProgram = rb.blurProgram;

    auto* whiteFlashProgramRaw = createWhiteFlashProgram();
    if (!whiteFlashProgramRaw) {
        return out;
    }
    rb.whiteFlashProgram = Ref<CCGLProgram>::adopt(whiteFlashProgramRaw);
    CCGLProgram* whiteFlashProgram = rb.whiteFlashProgram;
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    auto* colorInvertProgramRaw = createColorInvertProgram();
    if (!colorInvertProgramRaw) {
        return out;
    }
    rb.colorInvertProgram = Ref<CCGLProgram>::adopt(colorInvertProgramRaw);

    auto* mergeRoot = CCNode::create();
    if (!mergeRoot) {
        return out;
    }
    mergeRoot->setID("object-merge-root"_spr);
    mergeRoot->setPosition({0.0f, 0.0f});
    mergeRoot->setVisible(false);
    overlayLayer->addChild(mergeRoot, outputZOrder - 1);
    rb.mergeRoot = mergeRoot;

    for (int i = 0; i < kMotionBlurObjectCount; ++i) {
        auto const& seed = objectSeeds[static_cast<size_t>(i)];
        MotionBlurObjectCapture capture{};
        capture.id = seed.id;
        capture.sourceRoot = seed.sourceRoot;
        capture.enabled = seed.enabled;
        capture.tuning = seed.tuning;

        auto* rt = CCRenderTexture::create(
            static_cast<int>(std::ceil(captureSize.width)),
            static_cast<int>(std::ceil(captureSize.height)),
            kCCTexture2DPixelFormat_RGBA8888
        );
        if (!rt) {
            out.objects[static_cast<size_t>(i)] = capture;
            continue;
        }
        capture.renderTexture = rt;

        auto* objectBlur = OverlayShaderSprite::createMotionBlur(rt->getSprite()->getTexture(), blurProgram, locBlurDir);
        if (!objectBlur) {
            capture.renderTexture = nullptr;
            capture.enabled = false;
            out.objects[static_cast<size_t>(i)] = capture;
            continue;
        }
        objectBlur->setID("object-motion-blur-sprite"_spr);
        objectBlur->setShaderProgram(blurProgram);
        objectBlur->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
        objectBlur->setAnchorPoint({0.0f, 0.0f});
        objectBlur->setPosition({0.0f, 0.0f});
        objectBlur->setVisible(true);
        objectBlur->setFlipY(true);
        {
            float const cw = objectBlur->getContentSize().width;
            float const ch = objectBlur->getContentSize().height;
            objectBlur->setScaleX(cw > 0.0f ? captureSize.width / cw : captureSize.width);
            objectBlur->setScaleY(ch > 0.0f ? captureSize.height / ch : captureSize.height);
        }
        mergeRoot->addChild(objectBlur, 0);
        capture.blurSprite = objectBlur;
        out.objects[static_cast<size_t>(i)] = capture;
    }

    rb.disarm();
    // ok reflects shared pipeline only, individual object captures may still be nullptr
    out.ok = true;
    out.blurProgram = rb.blurProgram.take();
    out.whiteFlashProgram = rb.whiteFlashProgram.take();
    out.colorInvertProgram = rb.colorInvertProgram.take();
    out.unifiedMergeTexture = rb.unifiedTexture.take();
    out.mergeRoot = mergeRoot;
    out.finalCompositeSprite = finalComposite;
    out.whiteFlashSprite = whiteFlashSprite;
    return out;
}

FireAuraAttachResult attachFireAura(CCNode* playerRoot, float auraDiameterPx) {
    FireAuraAttachResult out{};
    if (!playerRoot || auraDiameterPx <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = createOneByOneWhiteTexture();
    if (!tex) {
        return out;
    }

    GLint locVelocity = -1;
    GLint locTime = -1;
    GLint locIntensity = -1;
    GLint locColorPrimary = -1;
    GLint locColorSecondary = -1;
    CCGLProgram* program = createFireAuraProgram(
        &locVelocity,
        &locTime,
        &locIntensity,
        &locColorPrimary,
        &locColorSecondary
    );
    if (!program || locVelocity < 0 || locTime < 0 || locIntensity < 0 || locColorPrimary < 0
        || locColorSecondary < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    OverlayShaderSprite* sprite = OverlayShaderSprite::createFireAura(
        tex,
        program,
        locVelocity,
        locTime,
        locIntensity,
        locColorPrimary,
        locColorSecondary
    );
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setID("fire-aura-sprite"_spr);
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    float const cw = sprite->getContentSize().width;
    sprite->setScale(cw > 0.0f ? auraDiameterPx / cw : auraDiameterPx);
    sprite->setPosition({0, 0});
    sprite->setVisible(false);
    playerRoot->addChild(sprite, kFireAuraZOrder);

    out.ok = true;
    out.sprite = sprite;
    out.program = program;
    return out;
}

void refreshObjectMotionBlurComposite(ObjectMotionBlurRefreshArgs const& args) {
    auto* objects = args.objects;
    CCNode* const mergeRoot = args.mergeRoot;
    CCRenderTexture* const unifiedMergeTexture = args.unifiedMergeTexture;
    CCSprite* const finalCompositeSprite = args.finalCompositeSprite;
    CCSprite* const whiteFlashSprite = args.whiteFlashSprite;
    CCGLProgram* const whiteFlashProgram = args.whiteFlashProgram;
    CCGLProgram* const colorInvertProgram = args.colorInvertProgram;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;

    if (!objects || !mergeRoot || !unifiedMergeTexture || !finalCompositeSprite) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool needCapture = impactFlashActive;
    for (auto const& object : *objects) {
        if (!object.enabled || !object.sourceRoot) {
            continue;
        }
        if (object.tuning.alwaysCaptureWhenEnabled) {
            needCapture = true;
            break;
        }
        float const speed = std::hypot(object.velocity.vx, object.velocity.vy);
        if (speed >= object.tuning.minBlurSpeedPx) {
            needCapture = true;
            break;
        }
    }

    if (!needCapture) {
        for (auto& object : *objects) {
            resetObjectVisualState(object);
        }
        finalCompositeSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        mergeRoot->setVisible(false);
        return;
    }

    for (auto& object : *objects) {
        if (!object.enabled || !object.sourceRoot || !object.renderTexture || !object.blurSprite) {
            resetObjectVisualState(object);
            continue;
        }
        object.blurSprite->setVisible(true);

        float const speed = std::hypot(object.velocity.vx, object.velocity.vy);
        float const maxSpeed = std::max(object.tuning.maxBlurSpeedPx, object.tuning.minBlurSpeedPx + 1.0f);
        float const normT = std::clamp(speed / maxSpeed, 0.0f, 1.0f);
        float const spreadUv = normT * object.tuning.blurUvSpread;
        float const invSpeed = speed > kMinSpeedForInverse ? 1.0f / speed : 0.0f;
        float const nx = -object.velocity.vx * invSpeed;
        float const ny = -object.velocity.vy * invSpeed;
        int const divisor = std::max(object.tuning.blurStepDivisor, 1);
        float const stepUv = spreadUv * (1.0f / static_cast<float>(divisor));
        object.blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

        object.sourceRoot->setVisible(true);
        object.renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        object.sourceRoot->visit();
        object.renderTexture->end();
        object.sourceRoot->setVisible(object.tuning.keepBaseVisible && !impactFlashActive);
    }

    mergeRoot->setVisible(true);
    unifiedMergeTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    mergeRoot->visit();
    unifiedMergeTexture->end();
    mergeRoot->setVisible(false);

    if (impactFlashMode == ImpactFlashMode::WhiteSilhouette && whiteFlashSprite && whiteFlashProgram) {
        whiteFlashSprite->setShaderProgram(whiteFlashProgram);
        whiteFlashSprite->setVisible(true);
        finalCompositeSprite->setVisible(false);
    } else if (impactFlashMode == ImpactFlashMode::InvertSilhouette && whiteFlashSprite && colorInvertProgram) {
        whiteFlashSprite->setShaderProgram(colorInvertProgram);
        whiteFlashSprite->setVisible(true);
        finalCompositeSprite->setVisible(false);
    } else {
        finalCompositeSprite->setVisible(true);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    }
}

void refreshFireAura(FireAuraRefreshArgs const& args) {
    OverlayShaderSprite* const fireAura = args.fireAura;
    PhysicsWorld* const physics = args.physics;
    float const dt = args.dt;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;
    float* const fireTime = args.fireTime;

    if (!fireAura || !physics || !fireTime) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    if (impactFlashActive) {
        fireAura->setVisible(false);
        return;
    }

    PhysicsVelocity const vel = physics->getPlayerVelocityPixels();
    float const speed = std::hypot(vel.vx, vel.vy);
    float const denom = kMaxFireAuraSpeedPx - kMinFireAuraSpeedPx;
    float intensity = 0.0f;
    if (denom > 1e-5f && speed > kMinFireAuraSpeedPx) {
        intensity = (speed - kMinFireAuraSpeedPx) / denom;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
    }

    if (intensity <= 0.0f) {
        fireAura->setVisible(false);
        return;
    }

    *fireTime += dt;
    float const tWrapped = std::fmod(*fireTime, 1000.0f);

    float const vx = vel.vx * kFireAuraVelocityToShader;
    float const vy = vel.vy * kFireAuraVelocityToShader;

    ccColor3B primaryRgb = {255, 255, 255};
    ccColor3B secondaryRgb = {200, 200, 200};
    if (GameManager* gm = GameManager::get()) {
        primaryRgb = gm->colorForIdx(gm->getPlayerColor());
        secondaryRgb = gm->colorForIdx(gm->getPlayerColor2());
    }
    fireAura->setFireColors(primaryRgb, secondaryRgb);
    fireAura->setFireState(vx, vy, tWrapped, intensity);
    fireAura->setVisible(true);
}

ImpactNoiseAttachResult attachImpactNoise(CCNode* overlayLayer, CCSize winSize) {
    ImpactNoiseAttachResult out{};
    if (!overlayLayer || winSize.width <= 0.0f || winSize.height <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = createOneByOneWhiteTexture();
    if (!tex) {
        return out;
    }

    GLint locTime = -1;
    GLint locAlpha = -1;
    CCGLProgram* program = createImpactNoiseProgram(&locTime, &locAlpha);
    if (!program || locTime < 0 || locAlpha < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    OverlayShaderSprite* sprite = OverlayShaderSprite::createImpactNoise(tex, program, locTime, locAlpha);
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setID("impact-noise-sprite"_spr);
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    sprite->setAnchorPoint({0.0f, 0.0f});
    sprite->setPosition({0.0f, 0.0f});
    sprite->setVisible(false);

    int const rw = std::max(8, static_cast<int>(std::ceil(winSize.width * kImpactNoiseRenderScale)));
    int const rh = std::max(8, static_cast<int>(std::ceil(winSize.height * kImpactNoiseRenderScale)));

    CCRenderTexture* rtRaw = CCRenderTexture::create(rw, rh, kCCTexture2DPixelFormat_RGBA8888);
    Ref<CCRenderTexture> rtHold{};
    if (rtRaw) {
        rtHold = rtRaw;
    }
    CCSprite* composite = nullptr;
    if (rtHold) {
        CCTexture2D* rtTex = rtHold->getSprite()->getTexture();
        ccTexParams texParams{};
        texParams.minFilter = kImpactNoiseCompositeNearestFilter ? GL_NEAREST : GL_LINEAR;
        texParams.magFilter = kImpactNoiseCompositeNearestFilter ? GL_NEAREST : GL_LINEAR;
        texParams.wrapS = GL_CLAMP_TO_EDGE;
        texParams.wrapT = GL_CLAMP_TO_EDGE;
        rtTex->setTexParameters(&texParams);

        composite = CCSprite::createWithTexture(rtTex);
        if (!composite) {
            rtHold = nullptr;
        } else {
            composite->setID("impact-noise-composite"_spr);
            composite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
            composite->setAnchorPoint({0.0f, 0.0f});
            composite->setPosition({0.0f, 0.0f});
            float const ccw = composite->getContentSize().width;
            float const cch = composite->getContentSize().height;
            composite->setScaleX(ccw > 0.0f ? winSize.width / ccw : winSize.width);
            composite->setScaleY(cch > 0.0f ? winSize.height / cch : winSize.height);
            composite->setFlipY(true);
            composite->setVisible(false);
            overlayLayer->addChild(composite, kImpactNoiseZOrder);

            float const cw = sprite->getContentSize().width;
            float const ch = sprite->getContentSize().height;
            sprite->setScaleX(cw > 0.0f ? static_cast<float>(rw) / cw : static_cast<float>(rw));
            sprite->setScaleY(ch > 0.0f ? static_cast<float>(rh) / ch : static_cast<float>(rh));
            overlayLayer->addChild(sprite, kImpactNoiseZOrder - 1);
            out.renderTexture = rtHold.take();
            out.compositeSprite = composite;
        }
    }

    if (!out.renderTexture) {
        float const cw = sprite->getContentSize().width;
        float const ch = sprite->getContentSize().height;
        sprite->setScaleX(cw > 0.0f ? winSize.width / cw : winSize.width);
        sprite->setScaleY(ch > 0.0f ? winSize.height / ch : winSize.height);
        overlayLayer->addChild(sprite, kImpactNoiseZOrder);
    }

    out.ok = true;
    out.sprite = sprite;
    out.program = program;
    return out;
}

void refreshImpactNoise(ImpactNoiseRefreshArgs const& args) {
    OverlayShaderSprite* const sprite = args.sprite;
    float* const timePtr = args.time;
    if (!sprite || !timePtr) {
        return;
    }

    *timePtr += args.extraTimeSkip;
    if (args.visible) {
        *timePtr += args.dt;
    }

    if (!args.visible) {
        sprite->setVisible(false);
        if (args.compositeSprite) {
            args.compositeSprite->setVisible(false);
        }
        return;
    }

    float const tWrapped = std::fmod(*timePtr, 1000.0f);

    sprite->setNoiseState(tWrapped, args.alpha);

    if (args.renderTexture && args.compositeSprite) {
        args.renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        sprite->setVisible(true);
        sprite->visit();
        sprite->setVisible(false);
        args.renderTexture->end();
        args.compositeSprite->setVisible(true);
    } else {
        sprite->setVisible(true);
    }
}

} // namespace overlay_rendering
