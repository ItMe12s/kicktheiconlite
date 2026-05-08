#include "OverlayRendering.h"
#include "ModTuning.h"

#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/textures/CCTexture2D.h>

using namespace geode::prelude;

namespace overlay_rendering {

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

} // namespace overlay_rendering
