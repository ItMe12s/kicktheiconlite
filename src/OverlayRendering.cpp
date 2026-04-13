#include "OverlayRendering.h"
#include "OverlayShaders.h"

#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCScene.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>

#include <algorithm>
#include <cmath>
#include <random>

using namespace geode::prelude;

namespace overlay_rendering {

namespace {

constexpr int kScreenShakeIntervals = 10;
constexpr float kScreenShakeSampleMin = -1.0f;
constexpr float kScreenShakeSampleMax = 1.0f;

} // namespace

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
    s->m_blurProg = prog;
    s->m_locBlurDir = locBlurDir;
    if (s->initWithTexture(tex)) {
        s->autorelease();
        return s;
    }
    delete s;
    return nullptr;
}

CCGLProgram* createMotionBlurProgram(GLint* outBlurDir) {
    auto* p = new CCGLProgram();
    if (!p->initWithVertexShaderByteArray(shaders::kMotionBlurVert, shaders::kMotionBlurFrag)) {
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
    *outBlurDir = p->getUniformLocationForName("u_blurDir");
    p->retain();
    return p;
}

CCGLProgram* createWhiteFlashProgram() {
    auto* p = new CCGLProgram();
    if (!p->initWithVertexShaderByteArray(shaders::kMotionBlurVert, shaders::kWhiteFlashFrag)) {
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

int captureSizeForTarget(float targetSize) {
    int captureSize = static_cast<int>(std::ceil(targetSize * kBlurCaptureScale));
    if (captureSize < 32) {
        captureSize = 32;
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
    playerRoot->addChild(blurSprite, 1);

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
    playerRoot->addChild(whiteFlashSprite, 2);

    auto* whiteFlashProgram = createWhiteFlashProgram();
    if (!whiteFlashProgram) {
        blurProgram->release();
        renderTexture->release();
        playerRoot->removeFromParentAndCleanup(true);
        return out;
    }
    whiteFlashSprite->setShaderProgram(whiteFlashProgram);

    out.ok = true;
    out.renderTexture = renderTexture;
    out.blurProgram = blurProgram;
    out.locBlurDir = locBlurDir;
    out.blurSprite = blurSprite;
    out.whiteFlashSprite = whiteFlashSprite;
    out.whiteFlashProgram = whiteFlashProgram;
    return out;
}

void globalScreenShake(float duration, float strength) {
    CCScene* scene = CCScene::get();
    if (!scene) {
        return;
    }

    CCPoint const base = scene->getPosition();
    scene->stopAllActions();

    int const intervals = kScreenShakeIntervals;
    float const stepDuration = duration / static_cast<float>(intervals);

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
    actions->addObject(CCMoveTo::create(stepDuration, ccp(0.0f, 0.0f)));
    scene->runAction(CCSequence::create(actions));
}

void refreshPlayerMotionBlur(
    float dt,
    SimplePlayer* player,
    CCNode* playerRoot,
    CCLayer* hostLayer,
    CCRenderTexture* renderTexture,
    MotionBlurSprite* blurSprite,
    CCSprite* whiteFlashSprite,
    PhysicsWorld* physics,
    int captureSize,
    bool whiteFlashActive
) {
    (void)dt;
    (void)playerRoot;
    if (!player || !renderTexture || !blurSprite || !physics) {
        return;
    }

    PhysicsVelocity const vel = physics->getPlayerVelocityPixels();
    float const speed = std::hypot(vel.vx, vel.vy);

    bool const needCapture = (speed >= kMinBlurSpeedPx) || whiteFlashActive;

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
    float const invSpeed = speed > 1e-6f ? 1.0f / speed : 0.0f;
    float const nx = -vel.vx * invSpeed;
    float const ny = -vel.vy * invSpeed;
    float const stepUv = spreadUv * (1.0f / 4.0f);
    blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

    if (!whiteFlashActive) {
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
    float const cx = bb.origin.x + bb.size.width * 0.5f;
    float const cy = bb.origin.y + bb.size.height * 0.5f;
    float const h = static_cast<float>(captureSize) * 0.5f;
    player->setPosition({h - cx, h - cy});

    renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    player->visit();
    renderTexture->end();

    hostLayer->removeChild(player, false);
    if (parent) {
        parent->addChild(player);
    }
    player->setPosition({0, 0});

    if (whiteFlashActive && whiteFlashSprite) {
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
