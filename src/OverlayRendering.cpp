#include "OverlayRendering.h"
#include "OverlayShaders.h"
#include "PhysicsOverlayTuning.h"
#include "PhysicsWorld.h"

#include <Geode/binding/GameManager.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCScene.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

using namespace geode::prelude;

namespace overlay_rendering {

namespace {

CCGLProgram* createLinkedProgram(char const* vert, char const* frag) {
    auto* p = new CCGLProgram();
    if (!p->initWithVertexShaderByteArray(vert, frag)) {
        delete p;
        return nullptr;
    }
    p->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    p->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    p->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    if (!p->link()) {
        delete p;
        return nullptr;
    }
    p->updateUniforms();
    p->retain();
    return p;
}

CCTexture2D* createOneByOneWhiteTexture() {
    static unsigned char const pixels[4] = {255, 255, 255, 255};
    auto* tex = new CCTexture2D();
    if (!tex->initWithData(
            pixels,
            kCCTexture2DPixelFormat_RGBA8888,
            1,
            1,
            CCSizeMake(1, 1)
        )) {
        delete tex;
        return nullptr;
    }
    tex->autorelease();
    return tex;
}

int layerCompositeOrder(OverlayLayerId id) {
    switch (id) {
        case OverlayLayerId::Trail: return 0;
        case OverlayLayerId::World: return 1;
        case OverlayLayerId::Ui: return 2;
        default: return 1;
    }
}

int objectCompositeOrder(MotionBlurObjectId id) {
    switch (id) {
        case MotionBlurObjectId::Player: return 0;
        case MotionBlurObjectId::PhysicsMenu: return 1;
        default: return 0;
    }
}

} // namespace

void MotionBlurSprite::setBlurUniforms(CCGLProgram* prog, GLint locBlurDir) {
    m_blurProg = prog;
    m_locBlurDir = locBlurDir;
}

void MotionBlurSprite::setBlurStep(float x, float y) {
    m_stepX = x;
    m_stepY = y;
}

void MotionBlurSprite::draw() {
    if (m_blurProg && m_locBlurDir >= 0) {
        m_blurProg->use();
        m_blurProg->setUniformLocationWith2f(m_locBlurDir, m_stepX, m_stepY);
    }
    CCSprite::draw();
}

MotionBlurSprite* MotionBlurSprite::create(CCTexture2D* tex, CCGLProgram* prog, GLint locBlurDir) {
    auto* s = new MotionBlurSprite();
    s->setBlurUniforms(prog, locBlurDir);
    if (s->initWithTexture(tex)) {
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

void ImpactNoiseSprite::setNoiseUniforms(CCGLProgram* prog, GLint locTime, GLint locAlpha) {
    m_noiseProg = prog;
    m_locTime = locTime;
    m_locAlpha = locAlpha;
}

void ImpactNoiseSprite::setNoiseState(float time, float alpha) {
    m_time = time;
    m_alpha = alpha;
}

void ImpactNoiseSprite::draw() {
    if (m_noiseProg && m_locTime >= 0 && m_locAlpha >= 0) {
        m_noiseProg->use();
        m_noiseProg->setUniformLocationWith1f(m_locTime, m_time);
        m_noiseProg->setUniformLocationWith1f(m_locAlpha, m_alpha);
    }
    CCSprite::draw();
}

ImpactNoiseSprite* ImpactNoiseSprite::create(CCTexture2D* tex, CCGLProgram* prog, GLint locTime, GLint locAlpha) {
    auto* s = new ImpactNoiseSprite();
    s->setNoiseUniforms(prog, locTime, locAlpha);
    if (s->initWithTexture(tex)) {
        s->setColor(ccc3(255, 255, 255));
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

void FireAuraSprite::setFireUniforms(
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity,
    GLint locColorPrimary,
    GLint locColorSecondary
) {
    m_fireProg = prog;
    m_locVelocity = locVelocity;
    m_locTime = locTime;
    m_locIntensity = locIntensity;
    m_locColorPrimary = locColorPrimary;
    m_locColorSecondary = locColorSecondary;
}

void FireAuraSprite::setFireState(float velX, float velY, float time, float intensity) {
    m_velX = velX;
    m_velY = velY;
    m_time = time;
    m_intensity = intensity;
}

void FireAuraSprite::setFireColors(ccColor3B primaryRgb, ccColor3B secondaryRgb) {
    constexpr float kInv = 1.0f / 255.0f;
    m_colorPrimaryR = static_cast<float>(primaryRgb.r) * kInv;
    m_colorPrimaryG = static_cast<float>(primaryRgb.g) * kInv;
    m_colorPrimaryB = static_cast<float>(primaryRgb.b) * kInv;
    m_colorSecondaryR = static_cast<float>(secondaryRgb.r) * kInv;
    m_colorSecondaryG = static_cast<float>(secondaryRgb.g) * kInv;
    m_colorSecondaryB = static_cast<float>(secondaryRgb.b) * kInv;
}

void FireAuraSprite::draw() {
    if (m_fireProg && m_locVelocity >= 0 && m_locTime >= 0 && m_locIntensity >= 0 && m_locColorPrimary >= 0
        && m_locColorSecondary >= 0) {
        m_fireProg->use();
        m_fireProg->setUniformLocationWith2f(m_locVelocity, m_velX, m_velY);
        m_fireProg->setUniformLocationWith1f(m_locTime, m_time);
        m_fireProg->setUniformLocationWith1f(m_locIntensity, m_intensity);
        m_fireProg->setUniformLocationWith3f(
            m_locColorPrimary,
            m_colorPrimaryR,
            m_colorPrimaryG,
            m_colorPrimaryB
        );
        m_fireProg->setUniformLocationWith3f(
            m_locColorSecondary,
            m_colorSecondaryR,
            m_colorSecondaryG,
            m_colorSecondaryB
        );
    }
    CCSprite::draw();
}

FireAuraSprite* FireAuraSprite::create(
    CCTexture2D* tex,
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity,
    GLint locColorPrimary,
    GLint locColorSecondary
) {
    auto* s = new FireAuraSprite();
    s->setFireUniforms(prog, locVelocity, locTime, locIntensity, locColorPrimary, locColorSecondary);
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

int captureSizeForTarget(float targetSize) {
    int captureSize = static_cast<int>(std::ceil(targetSize * kBlurCaptureScale));
    if (captureSize < kMinCaptureTextureSize) {
        captureSize = kMinCaptureTextureSize;
    }
    return captureSize;
}

MotionBlurAttachResult attachMotionBlur(
    CCNode* overlayLayer,
    CCSize captureSize,
    CCSize outputSize,
    int outputZOrder
) {
    MotionBlurAttachResult out{};
    if (!overlayLayer || captureSize.width <= 0.0f || captureSize.height <= 0.0f || outputSize.width <= 0.0f
        || outputSize.height <= 0.0f) {
        return out;
    }

    auto* renderTexture = CCRenderTexture::create(
        static_cast<int>(std::ceil(captureSize.width)),
        static_cast<int>(std::ceil(captureSize.height)),
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (!renderTexture) {
        return out;
    }
    renderTexture->retain();

    GLint locBlurDir = -1;
    auto* blurProgram = createMotionBlurProgram(&locBlurDir);
    if (!blurProgram || locBlurDir < 0) {
        if (blurProgram) {
            blurProgram->release();
        }
        renderTexture->release();
        return out;
    }

    CCTexture2D* rtTex = renderTexture->getSprite()->getTexture();
    auto* blurSprite = MotionBlurSprite::create(rtTex, blurProgram, locBlurDir);
    if (!blurSprite) {
        blurProgram->release();
        renderTexture->release();
        return out;
    }
    blurSprite->setID("motion-blur-sprite"_spr);
    blurSprite->setShaderProgram(blurProgram);
    blurSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    {
        float const cw = blurSprite->getContentSize().width;
        float const ch = blurSprite->getContentSize().height;
        blurSprite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        blurSprite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    blurSprite->setAnchorPoint({0.0f, 0.0f});
    blurSprite->setPosition({0.0f, 0.0f});
    blurSprite->setVisible(false);
    blurSprite->setFlipY(true);
    overlayLayer->addChild(blurSprite, outputZOrder);

    auto* whiteFlashSprite = CCSprite::createWithTexture(rtTex);
    if (!whiteFlashSprite) {
        blurProgram->release();
        renderTexture->release();
        return out;
    }
    whiteFlashSprite->setID("white-flash-sprite"_spr);
    whiteFlashSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    {
        float const cw = whiteFlashSprite->getContentSize().width;
        float const ch = whiteFlashSprite->getContentSize().height;
        whiteFlashSprite->setScaleX(cw > 0.0f ? outputSize.width / cw : outputSize.width);
        whiteFlashSprite->setScaleY(ch > 0.0f ? outputSize.height / ch : outputSize.height);
    }
    whiteFlashSprite->setAnchorPoint({0.0f, 0.0f});
    whiteFlashSprite->setPosition({0.0f, 0.0f});
    whiteFlashSprite->setVisible(false);
    whiteFlashSprite->setFlipY(true);
    overlayLayer->addChild(whiteFlashSprite, outputZOrder);

    auto* whiteFlashProgram = createWhiteFlashProgram();
    if (!whiteFlashProgram) {
        blurProgram->release();
        renderTexture->release();
        return out;
    }
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    auto* colorInvertProgram = createColorInvertProgram();
    if (!colorInvertProgram) {
        whiteFlashProgram->release();
        blurProgram->release();
        renderTexture->release();
        return out;
    }

    out.ok = true;
    out.renderTexture = renderTexture;
    out.blurProgram = blurProgram;
    out.locBlurDir = locBlurDir;
    out.blurSprite = blurSprite;
    out.whiteFlashSprite = whiteFlashSprite;
    out.whiteFlashProgram = whiteFlashProgram;
    out.colorInvertProgram = colorInvertProgram;
    return out;
}

LayeredMotionBlurAttachResult attachLayeredMotionBlur(
    CCNode* overlayLayer,
    CCSize captureSize,
    CCSize outputSize,
    int outputZOrder,
    std::array<CCNode*, kOverlayLayerCount> const& layerRoots
) {
    LayeredMotionBlurAttachResult out{};
    auto const base = attachMotionBlur(overlayLayer, captureSize, outputSize, outputZOrder);
    if (!base.ok || !base.renderTexture) {
        return out;
    }

    out.renderTexture = base.renderTexture;
    out.blurProgram = base.blurProgram;
    out.locBlurDir = base.locBlurDir;
    out.blurSprite = base.blurSprite;
    out.whiteFlashSprite = base.whiteFlashSprite;
    out.whiteFlashProgram = base.whiteFlashProgram;
    out.colorInvertProgram = base.colorInvertProgram;
    out.unifiedMergeTexture = base.renderTexture;

    auto* mergeRoot = CCNode::create();
    if (!mergeRoot) {
        out.ok = true;
        return out;
    }
    mergeRoot->setID("layered-merge-root"_spr);
    mergeRoot->setPosition({0.0f, 0.0f});
    mergeRoot->setVisible(false);
    overlayLayer->addChild(mergeRoot, outputZOrder - 1);
    out.mergeRoot = mergeRoot;

    for (int i = 0; i < kOverlayLayerCount; ++i) {
        OverlayLayerCapture capture{};
        capture.id = static_cast<OverlayLayerId>(i);
        capture.sourceRoot = layerRoots[static_cast<size_t>(i)];
        capture.enabled = capture.sourceRoot != nullptr;
        if (!capture.enabled) {
            out.layers[static_cast<size_t>(i)] = capture;
            continue;
        }

        auto* layerTexture = CCRenderTexture::create(
            static_cast<int>(std::ceil(captureSize.width)),
            static_cast<int>(std::ceil(captureSize.height)),
            kCCTexture2DPixelFormat_RGBA8888
        );
        if (!layerTexture) {
            capture.enabled = false;
            out.layers[static_cast<size_t>(i)] = capture;
            continue;
        }
        layerTexture->retain();
        capture.renderTexture = layerTexture;

        auto* layerComposite = CCSprite::createWithTexture(layerTexture->getSprite()->getTexture());
        if (!layerComposite) {
            layerTexture->release();
            capture.renderTexture = nullptr;
            capture.enabled = false;
            out.layers[static_cast<size_t>(i)] = capture;
            continue;
        }
        layerComposite->setID("layer-capture-composite"_spr);
        layerComposite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
        layerComposite->setAnchorPoint({0.0f, 0.0f});
        layerComposite->setPosition({0.0f, 0.0f});
        layerComposite->setFlipY(true);
        float const cw = layerComposite->getContentSize().width;
        float const ch = layerComposite->getContentSize().height;
        layerComposite->setScaleX(cw > 0.0f ? captureSize.width / cw : captureSize.width);
        layerComposite->setScaleY(ch > 0.0f ? captureSize.height / ch : captureSize.height);
        mergeRoot->addChild(layerComposite, layerCompositeOrder(capture.id));
        capture.compositeSprite = layerComposite;
        out.layers[static_cast<size_t>(i)] = capture;
    }

    out.ok = true;
    return out;
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

    auto* unifiedTexture = CCRenderTexture::create(
        static_cast<int>(std::ceil(captureSize.width)),
        static_cast<int>(std::ceil(captureSize.height)),
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (!unifiedTexture) {
        return out;
    }
    unifiedTexture->retain();

    auto* finalComposite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!finalComposite) {
        unifiedTexture->release();
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

    auto* whiteFlashSprite = CCSprite::createWithTexture(unifiedTexture->getSprite()->getTexture());
    if (!whiteFlashSprite) {
        unifiedTexture->release();
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

    GLint locBlurDir = -1;
    auto* blurProgram = createMotionBlurProgram(&locBlurDir);
    if (!blurProgram || locBlurDir < 0) {
        if (blurProgram) {
            blurProgram->release();
        }
        unifiedTexture->release();
        return out;
    }

    auto* whiteFlashProgram = createWhiteFlashProgram();
    if (!whiteFlashProgram) {
        blurProgram->release();
        unifiedTexture->release();
        return out;
    }
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    auto* colorInvertProgram = createColorInvertProgram();
    if (!colorInvertProgram) {
        whiteFlashProgram->release();
        blurProgram->release();
        unifiedTexture->release();
        return out;
    }

    auto* mergeRoot = CCNode::create();
    if (!mergeRoot) {
        colorInvertProgram->release();
        whiteFlashProgram->release();
        blurProgram->release();
        unifiedTexture->release();
        return out;
    }
    mergeRoot->setID("object-merge-root"_spr);
    mergeRoot->setPosition({0.0f, 0.0f});
    mergeRoot->setVisible(false);
    overlayLayer->addChild(mergeRoot, outputZOrder - 1);

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
        rt->retain();
        capture.renderTexture = rt;

        auto* objectBlur = MotionBlurSprite::create(rt->getSprite()->getTexture(), blurProgram, locBlurDir);
        if (!objectBlur) {
            rt->release();
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
        mergeRoot->addChild(objectBlur, objectCompositeOrder(capture.id));
        capture.blurSprite = objectBlur;
        out.objects[static_cast<size_t>(i)] = capture;
    }

    out.ok = true;
    out.blurProgram = blurProgram;
    out.locBlurDir = locBlurDir;
    out.whiteFlashProgram = whiteFlashProgram;
    out.colorInvertProgram = colorInvertProgram;
    out.unifiedMergeTexture = unifiedTexture;
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

    FireAuraSprite* sprite = FireAuraSprite::create(
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

void globalScreenShake(float duration, float strength) {
    using clock = std::chrono::steady_clock;
    static double s_nextShakeAllowedSec = 0.0;
    auto const now = clock::now();
    double const nowSec =
        std::chrono::duration<double>(now.time_since_epoch()).count();
    if (nowSec < s_nextShakeAllowedSec) {
        return;
    }

    CCScene* scene = CCScene::get();
    if (!scene) {
        return;
    }

    CCPoint const base = scene->getPosition();
    scene->stopAllActions();

    int const intervals = kScreenShakeIntervals;
    float const stepDuration = duration / static_cast<float>(intervals);
    float const totalShakeSec =
        stepDuration * static_cast<float>(intervals + 1);
    s_nextShakeAllowedSec =
        nowSec + static_cast<double>(totalShakeSec + kScreenShakeCooldownExtraSeconds);

    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(kScreenShakeSampleMin, kScreenShakeSampleMax);

    CCArray* actions = CCArray::create();
    for (int i = 0; i < intervals; ++i) {
        float const t = static_cast<float>(i) / static_cast<float>(intervals);
        float const falloff = (1.0f - t) * (1.0f - t);
        float const offX = dist(rng) * strength * falloff;
        float const offY = dist(rng) * strength * falloff;
        actions->addObject(CCMoveTo::create(stepDuration, ccp(base.x + offX, base.y + offY)));
    }
    actions->addObject(CCMoveTo::create(stepDuration, ccp(base.x, base.y)));
    scene->runAction(CCSequence::create(actions));
}

void refreshMotionBlurComposite(MotionBlurRefreshArgs const& args) {
    CCNode* const captureRoot = args.captureRoot;
    CCRenderTexture* const renderTexture = args.renderTexture;
    MotionBlurSprite* const blurSprite = args.blurSprite;
    CCSprite* const whiteFlashSprite = args.whiteFlashSprite;
    CCGLProgram* const whiteFlashProgram = args.whiteFlashProgram;
    CCGLProgram* const colorInvertProgram = args.colorInvertProgram;
    PhysicsVelocity const vel = args.velocity;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;

    if (!captureRoot || !renderTexture || !blurSprite) {
        return;
    }

    float const speed = std::hypot(vel.vx, vel.vy);

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool const needCapture = (speed >= kMinBlurSpeedPx) || impactFlashActive;

    if (!needCapture) {
        captureRoot->setVisible(true);
        blurSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        return;
    }

    float const t = std::min(speed / kMaxBlurSpeedPx, 1.0f);
    float const spreadUv = t * kBlurUvSpread;
    float const invSpeed = speed > kMinSpeedForInverse ? 1.0f / speed : 0.0f;
    float const nx = -vel.vx * invSpeed;
    float const ny = -vel.vy * invSpeed;
    float const stepUv = spreadUv * (1.0f / static_cast<float>(kBlurStepDivisor));
    blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

    if (!impactFlashActive) {
        blurSprite->setVisible(true);
    }

    // Capture source must be visible every frame before visiting the subtree
    captureRoot->setVisible(true);
    renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    captureRoot->visit();
    renderTexture->end();
    captureRoot->setVisible(false);

    if (impactFlashMode == ImpactFlashMode::WhiteSilhouette && whiteFlashSprite && whiteFlashProgram) {
        whiteFlashSprite->setShaderProgram(whiteFlashProgram);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (impactFlashMode == ImpactFlashMode::InvertSilhouette && whiteFlashSprite && colorInvertProgram) {
        whiteFlashSprite->setShaderProgram(colorInvertProgram);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (speed >= kMinBlurSpeedPx) {
        blurSprite->setVisible(true);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    } else {
        captureRoot->setVisible(true);
        blurSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    }
}

void refreshLayeredMotionBlurComposite(LayeredMotionBlurRefreshArgs const& args) {
    auto const* layers = args.layers;
    CCNode* const mergeRoot = args.mergeRoot;
    CCRenderTexture* const unifiedMergeTexture = args.unifiedMergeTexture;
    MotionBlurSprite* const blurSprite = args.blurSprite;
    CCSprite* const whiteFlashSprite = args.whiteFlashSprite;
    CCGLProgram* const whiteFlashProgram = args.whiteFlashProgram;
    CCGLProgram* const colorInvertProgram = args.colorInvertProgram;
    PhysicsVelocity const vel = args.velocity;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;

    if (!layers || !mergeRoot || !unifiedMergeTexture || !blurSprite) {
        return;
    }

    float const speed = std::hypot(vel.vx, vel.vy);
    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool const needCapture = (speed >= kMinBlurSpeedPx) || impactFlashActive;

    if (!needCapture) {
        for (auto const& layer : *layers) {
            if (layer.sourceRoot) {
                layer.sourceRoot->setVisible(true);
            }
        }
        blurSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        mergeRoot->setVisible(false);
        return;
    }

    float const t = std::min(speed / kMaxBlurSpeedPx, 1.0f);
    float const spreadUv = t * kBlurUvSpread;
    float const invSpeed = speed > kMinSpeedForInverse ? 1.0f / speed : 0.0f;
    float const nx = -vel.vx * invSpeed;
    float const ny = -vel.vy * invSpeed;
    float const stepUv = spreadUv * (1.0f / static_cast<float>(kBlurStepDivisor));
    blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

    for (auto const& layer : *layers) {
        if (!layer.sourceRoot) {
            continue;
        }
        if (!layer.enabled || !layer.renderTexture) {
            // Keep non-captured layers hidden during unified capture to avoid direct-draw bleed-through.
            layer.sourceRoot->setVisible(false);
            continue;
        }
        layer.sourceRoot->setVisible(true);
        layer.renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        layer.sourceRoot->visit();
        layer.renderTexture->end();
        layer.sourceRoot->setVisible(false);
    }

    mergeRoot->setVisible(true);
    unifiedMergeTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    mergeRoot->visit();
    unifiedMergeTexture->end();
    mergeRoot->setVisible(false);

    if (impactFlashMode == ImpactFlashMode::WhiteSilhouette && whiteFlashSprite && whiteFlashProgram) {
        whiteFlashSprite->setShaderProgram(whiteFlashProgram);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (impactFlashMode == ImpactFlashMode::InvertSilhouette && whiteFlashSprite && colorInvertProgram) {
        whiteFlashSprite->setShaderProgram(colorInvertProgram);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (speed >= kMinBlurSpeedPx) {
        blurSprite->setVisible(true);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    } else {
        for (auto const& layer : *layers) {
            if (layer.sourceRoot) {
                layer.sourceRoot->setVisible(true);
            }
        }
        blurSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    }
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
        float const speed = std::hypot(object.velocity.vx, object.velocity.vy);
        if (speed >= object.tuning.minBlurSpeedPx) {
            needCapture = true;
            break;
        }
    }

    if (!needCapture) {
        for (auto& object : *objects) {
            if (object.sourceRoot) {
                object.sourceRoot->setVisible(true);
            }
        }
        finalCompositeSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        mergeRoot->setVisible(false);
        return;
    }

    for (auto& object : *objects) {
        if (!object.sourceRoot) {
            continue;
        }
        if (!object.enabled || !object.renderTexture || !object.blurSprite) {
            object.sourceRoot->setVisible(true);
            continue;
        }

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
    FireAuraSprite* const fireAura = args.fireAura;
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

    ImpactNoiseSprite* sprite = ImpactNoiseSprite::create(tex, program, locTime, locAlpha);
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

    CCRenderTexture* rt = CCRenderTexture::create(rw, rh, kCCTexture2DPixelFormat_RGBA8888);
    CCSprite* composite = nullptr;
    if (rt) {
        rt->retain();
        CCTexture2D* rtTex = rt->getSprite()->getTexture();
        ccTexParams texParams{};
        texParams.minFilter = kImpactNoiseCompositeNearestFilter ? GL_NEAREST : GL_LINEAR;
        texParams.magFilter = kImpactNoiseCompositeNearestFilter ? GL_NEAREST : GL_LINEAR;
        texParams.wrapS = GL_CLAMP_TO_EDGE;
        texParams.wrapT = GL_CLAMP_TO_EDGE;
        rtTex->setTexParameters(&texParams);

        composite = CCSprite::createWithTexture(rtTex);
        if (!composite) {
            rt->release();
            rt = nullptr;
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
            out.renderTexture = rt;
            out.compositeSprite = composite;
        }
    }

    if (!rt) {
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
    ImpactNoiseSprite* const sprite = args.sprite;
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
