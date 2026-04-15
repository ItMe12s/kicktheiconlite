#include "ObjectMotionBlurPipeline.h"

namespace vfx::object_motion_blur {

bool attach(
    ObjectMotionBlurPipelineState& state,
    cocos2d::CCNode* overlayLayer,
    cocos2d::CCSize captureSize,
    cocos2d::CCSize outputSize,
    int outputZOrder,
    std::array<overlay_rendering::MotionBlurObjectSeed, overlay_rendering::kMotionBlurObjectCount> const& seeds
) {
    auto const result = overlay_rendering::attachObjectMotionBlur(
        overlayLayer,
        captureSize,
        outputSize,
        outputZOrder,
        seeds
    );
    if (!result.ok) {
        return false;
    }

    state.objects = result.objects;
    state.mergeRoot = result.mergeRoot;
    state.unifiedMergeTexture = result.unifiedMergeTexture;
    state.finalCompositeSprite = result.finalCompositeSprite;
    state.whiteFlashSprite = result.whiteFlashSprite;
    state.blurProgram = result.blurProgram;
    state.whiteFlashProgram = result.whiteFlashProgram;
    state.colorInvertProgram = result.colorInvertProgram;
    return true;
}

void refresh(
    ObjectMotionBlurPipelineState& state,
    overlay_rendering::ImpactFlashMode flashMode
) {
    overlay_rendering::refreshObjectMotionBlurComposite({
        .objects = &state.objects,
        .mergeRoot = state.mergeRoot,
        .unifiedMergeTexture = state.unifiedMergeTexture,
        .finalCompositeSprite = state.finalCompositeSprite,
        .whiteFlashSprite = state.whiteFlashSprite,
        .whiteFlashProgram = state.whiteFlashProgram,
        .colorInvertProgram = state.colorInvertProgram,
        .impactFlashMode = flashMode,
    });
}

void release(ObjectMotionBlurPipelineState& state) {
    if (state.blurProgram) {
        state.blurProgram->release();
        state.blurProgram = nullptr;
    }
    if (state.whiteFlashProgram) {
        state.whiteFlashProgram->release();
        state.whiteFlashProgram = nullptr;
    }
    if (state.colorInvertProgram) {
        state.colorInvertProgram->release();
        state.colorInvertProgram = nullptr;
    }

    state.mergeRoot = nullptr;
    state.finalCompositeSprite = nullptr;
    state.whiteFlashSprite = nullptr;

    for (auto& object : state.objects) {
        if (object.renderTexture) {
            object.renderTexture->release();
            object.renderTexture = nullptr;
        }
        object.blurSprite = nullptr;
        object.sourceRoot = nullptr;
        object.enabled = false;
        object.velocity = {};
    }

    if (state.unifiedMergeTexture) {
        state.unifiedMergeTexture->release();
        state.unifiedMergeTexture = nullptr;
    }
}

} // namespace vfx::object_motion_blur
