#include "OverlayRendering.h"
#include "OverlayShaders.h"
#include "PhysicsOverlayTuning.h"

#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCScene.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
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

void FireAuraSprite::setFireUniforms(
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity
) {
    m_fireProg = prog;
    m_locVelocity = locVelocity;
    m_locTime = locTime;
    m_locIntensity = locIntensity;
}

void FireAuraSprite::setFireState(float velX, float velY, float time, float intensity) {
    m_velX = velX;
    m_velY = velY;
    m_time = time;
    m_intensity = intensity;
}

void FireAuraSprite::draw() {
    if (m_fireProg && m_locVelocity >= 0 && m_locTime >= 0 && m_locIntensity >= 0) {
        m_fireProg->use();
        m_fireProg->setUniformLocationWith2f(m_locVelocity, m_velX, m_velY);
        m_fireProg->setUniformLocationWith1f(m_locTime, m_time);
        m_fireProg->setUniformLocationWith1f(m_locIntensity, m_intensity);
    }
    CCSprite::draw();
}

FireAuraSprite* FireAuraSprite::create(
    CCTexture2D* tex,
    CCGLProgram* prog,
    GLint locVelocity,
    GLint locTime,
    GLint locIntensity
) {
    auto* s = new FireAuraSprite();
    s->setFireUniforms(prog, locVelocity, locTime, locIntensity);
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

CCGLProgram* createFireAuraProgram(GLint* outVelocity, GLint* outTime, GLint* outIntensity) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kFireAuraFrag);
    if (!p) {
        return nullptr;
    }
    *outVelocity = p->getUniformLocationForName("u_velocity");
    *outTime = p->getUniformLocationForName("u_time");
    *outIntensity = p->getUniformLocationForName("u_intensity");
    return p;
}

int captureSizeForTarget(float targetSize) {
    int captureSize = static_cast<int>(std::ceil(targetSize * kBlurCaptureScale));
    if (captureSize < kMinCaptureTextureSize) {
        captureSize = kMinCaptureTextureSize;
    }
    return captureSize;
}

MotionBlurAttachResult attachMotionBlur(CCNode* playerRoot, int captureSize) {
    MotionBlurAttachResult out{};

    auto* renderTexture = CCRenderTexture::create(
        captureSize,
        captureSize,
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (!renderTexture) {
        playerRoot->removeFromParentAndCleanup(true);
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
        playerRoot->removeFromParentAndCleanup(true);
        return out;
    }

    CCTexture2D* rtTex = renderTexture->getSprite()->getTexture();
    auto* blurSprite = MotionBlurSprite::create(rtTex, blurProgram, locBlurDir);
    if (!blurSprite) {
        blurProgram->release();
        renderTexture->release();
        playerRoot->removeFromParentAndCleanup(true);
        return out;
    }
    blurSprite->setShaderProgram(blurProgram);
    blurSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    {
        float const cw = blurSprite->getContentSize().width;
        blurSprite->setScale(cw > 0.0f ? static_cast<float>(captureSize) / cw : 1.0f);
    }
    blurSprite->setPosition({0, 0});
    blurSprite->setVisible(false);
    blurSprite->setFlipY(true);
    playerRoot->addChild(blurSprite, kMotionBlurOverlayLocalZ);

    auto* whiteFlashSprite = CCSprite::createWithTexture(rtTex);
    if (!whiteFlashSprite) {
        blurProgram->release();
        renderTexture->release();
        playerRoot->removeFromParentAndCleanup(true);
        return out;
    }
    whiteFlashSprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
    {
        float const cw = whiteFlashSprite->getContentSize().width;
        whiteFlashSprite->setScale(cw > 0.0f ? static_cast<float>(captureSize) / cw : 1.0f);
    }
    whiteFlashSprite->setPosition({0, 0});
    whiteFlashSprite->setVisible(false);
    whiteFlashSprite->setFlipY(true);
    playerRoot->addChild(whiteFlashSprite, kWhiteFlashOverlayLocalZ);

    auto* whiteFlashProgram = createWhiteFlashProgram();
    if (!whiteFlashProgram) {
        blurProgram->release();
        renderTexture->release();
        playerRoot->removeFromParentAndCleanup(true);
        return out;
    }
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    auto* colorInvertProgram = createColorInvertProgram();
    if (!colorInvertProgram) {
        whiteFlashProgram->release();
        blurProgram->release();
        renderTexture->release();
        playerRoot->removeFromParentAndCleanup(true);
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
    CCGLProgram* program = createFireAuraProgram(&locVelocity, &locTime, &locIntensity);
    if (!program || locVelocity < 0 || locTime < 0 || locIntensity < 0) {
        if (program) {
            program->release();
        }
        return out;
    }

    FireAuraSprite* sprite = FireAuraSprite::create(tex, program, locVelocity, locTime, locIntensity);
    if (!sprite) {
        program->release();
        return out;
    }
    sprite->setShaderProgram(program);
    sprite->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
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

void refreshPlayerMotionBlur(MotionBlurRefreshArgs const& args) {
    SimplePlayer* const player = args.player;
    CCLayer* const hostLayer = args.hostLayer;
    CCRenderTexture* const renderTexture = args.renderTexture;
    MotionBlurSprite* const blurSprite = args.blurSprite;
    CCSprite* const whiteFlashSprite = args.whiteFlashSprite;
    CCGLProgram* const whiteFlashProgram = args.whiteFlashProgram;
    CCGLProgram* const colorInvertProgram = args.colorInvertProgram;
    PhysicsWorld* const physics = args.physics;
    int const captureSize = args.captureSize;
    ImpactFlashMode const impactFlashMode = args.impactFlashMode;

    if (!player || !renderTexture || !blurSprite || !physics) {
        return;
    }

    PhysicsVelocity const vel = physics->getPlayerVelocityPixels();
    float const speed = std::hypot(vel.vx, vel.vy);

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool const needCapture = (speed >= kMinBlurSpeedPx) || impactFlashActive;

    if (!needCapture) {
        player->setVisible(true);
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

    player->retain();
    CCNode* const parent = player->getParent();
    if (parent) {
        parent->removeChild(player, false);
    }
    hostLayer->addChild(player);
    player->setVisible(true);

    player->setPosition({0, 0});
    CCRect const bb = player->boundingBox();
    float const cx = bb.origin.x + bb.size.width * kBoundsCenterFrac;
    float const cy = bb.origin.y + bb.size.height * kBoundsCenterFrac;
    float const h = static_cast<float>(captureSize) * kBoundsCenterFrac;
    player->setPosition({h - cx, h - cy});

    renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    player->visit();
    renderTexture->end();

    hostLayer->removeChild(player, false);
    if (parent) {
        parent->addChild(player);
    }
    player->setPosition({0, 0});

    if (impactFlashMode == ImpactFlashMode::WhiteSilhouette && whiteFlashSprite && whiteFlashProgram) {
        whiteFlashSprite->setShaderProgram(whiteFlashProgram);
        player->setVisible(false);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (impactFlashMode == ImpactFlashMode::InvertSilhouette && whiteFlashSprite && colorInvertProgram) {
        whiteFlashSprite->setShaderProgram(colorInvertProgram);
        player->setVisible(false);
        blurSprite->setVisible(false);
        whiteFlashSprite->setVisible(true);
    } else if (speed >= kMinBlurSpeedPx) {
        blurSprite->setVisible(true);
        player->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    } else {
        player->setVisible(true);
        blurSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
    }
    player->release();
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

    fireAura->setFireState(vx, vy, tWrapped, intensity);
    fireAura->setVisible(true);
}

} // namespace overlay_rendering
