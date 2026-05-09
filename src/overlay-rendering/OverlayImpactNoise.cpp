#include "OverlayRendering.h"
#include "ModTuning.h"

#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace overlay_rendering {

ImpactNoiseAttachResult attachImpactNoise(CCNode* overlayLayer, CCSize winSize) {
    ImpactNoiseAttachResult out{};
    if (!overlayLayer || winSize.width <= 0.0f || winSize.height <= 0.0f) {
        return out;
    }

    CCTexture2D* tex = detail::createOneByOneWhiteTexture();
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

void refreshImpactNoise(
    OverlayShaderSprite* sprite,
    CCRenderTexture* renderTexture,
    CCSprite* compositeSprite,
    float dt,
    float extraTimeSkip,
    float* time,
    float alpha,
    bool visible
) {
    float* const timePtr = time;
    if (!sprite || !timePtr) {
        return;
    }

    *timePtr += extraTimeSkip;
    if (visible) {
        *timePtr += dt;
    }

    if (!visible) {
        sprite->setVisible(false);
        if (compositeSprite) {
            compositeSprite->setVisible(false);
        }
        return;
    }

    float const tWrapped = std::fmod(*timePtr, 1000.0f);

    sprite->setNoiseState(tWrapped, alpha);

    if (renderTexture && compositeSprite) {
        renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        sprite->setVisible(true);
        sprite->visit();
        sprite->setVisible(false);
        renderTexture->end();
        compositeSprite->setVisible(true);
    } else {
        sprite->setVisible(true);
    }
}

} // namespace overlay_rendering
