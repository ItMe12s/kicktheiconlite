#include "OverlayRendering.h"
#include "OverlayShaders.h"

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

constexpr int kScreenShakeIntervals = 10;
constexpr float kScreenShakeSampleMin = -1.0f;
constexpr float kScreenShakeSampleMax = 1.0f;

constexpr int kMotionBlurOverlayLocalZ = 1;
constexpr int kWhiteFlashOverlayLocalZ = 2;

constexpr int kMinCaptureTextureSize = 32;

constexpr int kBlurStepDivisor = 4;
constexpr float kMinSpeedForInverse = 1e-6f;

constexpr float kBoundsCenterFrac = 0.5f;

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

CCGLProgram* createStarburstFrameProgram(
    GLint* outPhase,
    GLint* outOrigin,
    GLint* outAspect,
    GLint* outFocusInner,
    GLint* outFocusOuter
) {
    auto* p = createLinkedProgram(shaders::kMotionBlurVert, shaders::kStarburstFrameFrag);
    if (!p) {
        return nullptr;
    }
    *outPhase = p->getUniformLocationForName("u_phase");
    *outOrigin = p->getUniformLocationForName("u_origin");
    *outAspect = p->getUniformLocationForName("u_aspect");
    *outFocusInner = p->getUniformLocationForName("u_focus_inner");
    *outFocusOuter = p->getUniformLocationForName("u_focus_outer");
    return p;
}

namespace {

CCTexture2D* sharedWhite1x1Texture() {
    static CCRenderTexture* s_rt = nullptr;
    if (!s_rt) {
        s_rt = CCRenderTexture::create(1, 1, kCCTexture2DPixelFormat_RGBA8888);
        if (!s_rt) {
            return nullptr;
        }
        s_rt->beginWithClear(1.0f, 1.0f, 1.0f, 1.0f);
        s_rt->end();
        s_rt->retain();
    }
    return s_rt->getSprite()->getTexture();
}

} // namespace

void StarburstFrameSprite::setStarburstParams(
    float phase,
    float originX,
    float originY,
    float aspect,
    float focusInner,
    float focusOuter
) {
    m_phase = phase;
    m_originX = originX;
    m_originY = originY;
    m_aspect = aspect;
    m_focusInner = focusInner;
    m_focusOuter = focusOuter;
}

void StarburstFrameSprite::draw() {
    if (m_prog && m_locPhase >= 0 && m_locOrigin >= 0 && m_locAspect >= 0 && m_locFocusInner >= 0 &&
        m_locFocusOuter >= 0) {
        m_prog->use();
        m_prog->setUniformLocationWith1f(m_locPhase, m_phase);
        m_prog->setUniformLocationWith2f(m_locOrigin, m_originX, m_originY);
        m_prog->setUniformLocationWith1f(m_locAspect, m_aspect);
        m_prog->setUniformLocationWith1f(m_locFocusInner, m_focusInner);
        m_prog->setUniformLocationWith1f(m_locFocusOuter, m_focusOuter);
    }
    CCSprite::draw();
}

StarburstFrameSprite* StarburstFrameSprite::create(
    CCGLProgram* prog,
    GLint locPhase,
    GLint locOrigin,
    GLint locAspect,
    GLint locFocusInner,
    GLint locFocusOuter
) {
    CCTexture2D* tex = sharedWhite1x1Texture();
    if (!tex || !prog) {
        return nullptr;
    }
    auto* s = new StarburstFrameSprite();
    s->m_prog = prog;
    s->m_locPhase = locPhase;
    s->m_locOrigin = locOrigin;
    s->m_locAspect = locAspect;
    s->m_locFocusInner = locFocusInner;
    s->m_locFocusOuter = locFocusOuter;
    if (s->initWithTexture(tex)) {
        s->setShaderProgram(prog);
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
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
    (void)args.dt;
    (void)args.playerRoot;
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

} // namespace overlay_rendering
