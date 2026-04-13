#include "OverlayRendering.h"

#include <Geode/cocos/actions/CCActionInterval.h>
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

char const* kMotionBlurVert = R"(attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoord;
#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif
void main() {
    gl_Position = CC_MVPMatrix * a_position;
    v_fragmentColor = a_color;
    v_texCoord = a_texCoord;
}
)";

char const* kMotionBlurFrag = R"(#ifdef GL_ES
precision lowp float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
uniform vec2 u_blurDir;
void main() {
    vec4 s =
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -4.0) * 0.05 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -3.0) * 0.09 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -2.0) * 0.12 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * -1.0) * 0.15 +
        texture2D(CC_Texture0, v_texCoord) * 0.18 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 1.0) * 0.15 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 2.0) * 0.12 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 3.0) * 0.09 +
        texture2D(CC_Texture0, v_texCoord + u_blurDir * 4.0) * 0.05;
    gl_FragColor = s * v_fragmentColor;
}
)";

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
    if (!p->initWithVertexShaderByteArray(kMotionBlurVert, kMotionBlurFrag)) {
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

    out.ok = true;
    out.renderTexture = renderTexture;
    out.blurProgram = blurProgram;
    out.locBlurDir = locBlurDir;
    out.blurSprite = blurSprite;
    return out;
}

void runOverlayWhiteFlash(CCLayerColor* layer, float duration, unsigned char peakOpacity) {
    if (!layer) {
        return;
    }
    layer->stopAllActions();
    layer->setOpacity(0);
    float const up = duration * 0.35f;
    float const down = duration * 0.65f;
    CCSequence* seq = CCSequence::create(
        CCFadeTo::create(up, static_cast<GLubyte>(peakOpacity)),
        CCFadeTo::create(down, 0),
        nullptr
    );
    layer->runAction(seq);
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
    PhysicsWorld* physics,
    int captureSize
) {
    (void)dt;
    if (!player || !playerRoot || !renderTexture || !blurSprite || !physics) {
        return;
    }

    PhysicsVelocity const vel = physics->getPlayerVelocityPixels();
    float const speed = std::hypot(vel.vx, vel.vy);

    if (speed < kMinBlurSpeedPx) {
        player->setVisible(true);
        blurSprite->setVisible(false);
        return;
    }

    blurSprite->setVisible(true);

    float const t = std::min(speed / kMaxBlurSpeedPx, 1.0f);
    float const spreadUv = t * kBlurUvSpread;
    float invSpeed = 1.0f / speed;
    float const nx = -vel.vx * invSpeed;
    float const ny = -vel.vy * invSpeed;
    float const stepUv = spreadUv * (1.0f / 4.0f);
    blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

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
    player->setVisible(false);
    player->release();
}

} // namespace overlay_rendering
