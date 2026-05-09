#include "OverlayRendering.h"
#include "ModTuning.h"
#include "PhysicsTypes.h"

#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace overlay_rendering {

namespace {

void resetPlayerMotionBlurVisualState(PlayerMotionBlurCapture& capture) {
    if (capture.sourceRoot) {
        capture.sourceRoot->setVisible(true);
    }
    if (capture.blurSprite) {
        capture.blurSprite->setVisible(false);
    }
}

} // namespace

// Renders player icon to a render texture, stacks white/invert flash passes, outputs one composite sprite.

PlayerMotionBlurAttachResult attachPlayerMotionBlur(
    CCNode* overlayLayer,
    CCSize captureSize,
    CCSize outputSize,
    int outputZOrder,
    CCNode* sourceRoot
) {
    PlayerMotionBlurAttachResult out{};
    if (!overlayLayer || !sourceRoot || captureSize.width <= 0.0f || captureSize.height <= 0.0f || outputSize.width <= 0.0f
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

    PlayerMotionBlurCapture capture{};
    capture.sourceRoot = sourceRoot;
    capture.enabled = true;

    auto* rt = CCRenderTexture::create(
        static_cast<int>(std::ceil(captureSize.width)),
        static_cast<int>(std::ceil(captureSize.height)),
        kCCTexture2DPixelFormat_RGBA8888
    );
    if (rt) {
        capture.renderTexture = rt;
        auto* objectBlur = OverlayShaderSprite::createMotionBlur(rt->getSprite()->getTexture(), blurProgram, locBlurDir);
        if (objectBlur) {
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
        } else {
            capture.renderTexture = nullptr;
            capture.enabled = false;
        }
    }

    out.capture = capture;

    rb.disarm();
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

void refreshPlayerMotionBlurComposite(
    PlayerMotionBlurCapture* capture,
    CCNode* mergeRoot,
    CCRenderTexture* unifiedMergeTexture,
    CCSprite* finalCompositeSprite,
    CCSprite* whiteFlashSprite,
    CCGLProgram* whiteFlashProgram,
    CCGLProgram* colorInvertProgram,
    ImpactFlashMode impactFlashMode
) {
    if (!capture || !mergeRoot || !unifiedMergeTexture || !finalCompositeSprite) {
        return;
    }

    bool const impactFlashActive = impactFlashMode != ImpactFlashMode::None;
    bool needCapture = impactFlashActive;
    float speed = 0.0f;
    if (capture->enabled && capture->sourceRoot) {
        speed = std::hypot(capture->velocity.vx, capture->velocity.vy);
        if (speed >= kPlayerMinBlurSpeedPx) {
            needCapture = true;
        }
    }

    if (!needCapture) {
        resetPlayerMotionBlurVisualState(*capture);
        finalCompositeSprite->setVisible(false);
        if (whiteFlashSprite) {
            whiteFlashSprite->setVisible(false);
        }
        mergeRoot->setVisible(false);
        return;
    }

    if (!capture->enabled || !capture->sourceRoot || !capture->renderTexture || !capture->blurSprite) {
        resetPlayerMotionBlurVisualState(*capture);
    } else {
        capture->blurSprite->setVisible(true);

        float const maxSpeed = std::max(kPlayerMaxBlurSpeedPx, kPlayerMinBlurSpeedPx + 1.0f);
        float const normT = std::clamp(speed / maxSpeed, 0.0f, 1.0f);
        float const spreadUv = normT * kPlayerBlurUvSpread;
        float const invSpeed = speed > kMinSpeedForInverse ? 1.0f / speed : 0.0f;
        float const nx = -capture->velocity.vx * invSpeed;
        float const ny = -capture->velocity.vy * invSpeed;
        int const divisor = std::max(kPlayerBlurStepDivisor, 1);
        float const stepUv = spreadUv * (1.0f / static_cast<float>(divisor));
        capture->blurSprite->setBlurStep(nx * stepUv, ny * stepUv);

        capture->sourceRoot->setVisible(true);
        capture->renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
        capture->sourceRoot->visit();
        capture->renderTexture->end();
        capture->sourceRoot->setVisible(kPlayerKeepBaseVisible && !impactFlashActive);
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

} // namespace overlay_rendering
