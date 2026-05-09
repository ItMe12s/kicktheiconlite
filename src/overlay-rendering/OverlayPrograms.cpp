#include "OverlayRendering.h"
#include "OverlayShaders.h"

#include <Geode/cocos/shaders/CCGLProgram.h>
#include <Geode/cocos/textures/CCTexture2D.h>

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

} // namespace

namespace detail {

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

} // namespace detail

CCGLProgram* createMotionBlurProgram(GLint* outBlurDir) {
    auto* p = createLinkedProgram(kMotionBlurVert, kMotionBlurFrag);
    if (!p) {
        return nullptr;
    }
    *outBlurDir = p->getUniformLocationForName("u_blurDir");
    return p;
}

CCGLProgram* createWhiteFlashProgram() {
    return createLinkedProgram(kMotionBlurVert, kWhiteFlashFrag);
}

CCGLProgram* createColorInvertProgram() {
    return createLinkedProgram(kMotionBlurVert, kColorInvertFrag);
}

CCGLProgram* createImpactNoiseProgram(GLint* outTime, GLint* outAlpha) {
    auto* p = createLinkedProgram(kMotionBlurVert, kImpactNoiseFrag);
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
    auto* p = createLinkedProgram(kMotionBlurVert, kFireAuraFrag);
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

} // namespace overlay_rendering
