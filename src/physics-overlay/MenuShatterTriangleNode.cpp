#include "MenuShatterTriangleNode.h"

#include <Geode/cocos/include/ccMacros.h>
#include <Geode/cocos/platform/CCGL.h>
#include <Geode/cocos/shaders/CCGLProgram.h>
#include <Geode/cocos/shaders/CCShaderCache.h>
#include <Geode/cocos/shaders/ccGLStateCache.h>
#include <Geode/cocos/textures/CCTexture2D.h>

#include <cstddef>
#include <cstring>

using namespace cocos2d;

MenuShatterTriangleNode* MenuShatterTriangleNode::create(
    CCTexture2D* texture,
    CCPoint const& local0,
    CCPoint const& local1,
    CCPoint const& local2,
    ccTex2F const& uv0,
    ccTex2F const& uv1,
    ccTex2F const& uv2,
    bool flipTextureY
) {
    auto* node = new MenuShatterTriangleNode();
    if (node && node->init(texture, local0, local1, local2, uv0, uv1, uv2, flipTextureY)) {
        node->autorelease();
        return node;
    }
    delete node;
    return nullptr;
}

bool MenuShatterTriangleNode::init(
    CCTexture2D* texture,
    CCPoint const& local0,
    CCPoint const& local1,
    CCPoint const& local2,
    ccTex2F const& uv0,
    ccTex2F const& uv1,
    ccTex2F const& uv2,
    bool flipTextureY
) {
    if (!CCNodeRGBA::init()) {
        return false;
    }
    if (!texture) {
        return false;
    }
    texture->retain();
    m_texture = texture;
    m_flipTextureY = flipTextureY;

    if (m_texture->hasPremultipliedAlpha()) {
        m_blendFunc.src = CC_BLEND_SRC;
        m_blendFunc.dst = CC_BLEND_DST;
        setOpacityModifyRGB(true);
    } else {
        m_blendFunc.src = GL_SRC_ALPHA;
        m_blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
        setOpacityModifyRGB(false);
    }

    m_verts[0].vertices = {local0.x, local0.y, 0.0f};
    m_verts[0].texCoords = uv0;

    m_verts[1].vertices = {local1.x, local1.y, 0.0f};
    m_verts[1].texCoords = uv1;

    m_verts[2].vertices = {local2.x, local2.y, 0.0f};
    m_verts[2].texCoords = uv2;

    if (m_flipTextureY) {
        for (auto& v : m_verts) {
            v.texCoords.v = 1.0f - v.texCoords.v;
        }
    }

    setCascadeOpacityEnabled(true);
    syncVertexColorsFromDisplayState();

    setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
    setAnchorPoint(ccp(0.5f, 0.5f));
    return true;
}

MenuShatterTriangleNode::~MenuShatterTriangleNode() {
    if (m_texture) {
        m_texture->release();
        m_texture = nullptr;
    }
}

void MenuShatterTriangleNode::setOpacity(GLubyte opacity) {
    CCNodeRGBA::setOpacity(opacity);
    syncVertexColorsFromDisplayState();
}

void MenuShatterTriangleNode::updateDisplayedOpacity(GLubyte parentOpacity) {
    CCNodeRGBA::updateDisplayedOpacity(parentOpacity);
    syncVertexColorsFromDisplayState();
}

void MenuShatterTriangleNode::syncVertexColorsFromDisplayState() {
    ccColor3B rgb = getDisplayedColor();
    GLubyte const a = getDisplayedOpacity();
    if (isOpacityModifyRGB()) {
        float const f = static_cast<float>(a) / 255.0f;
        rgb.r = static_cast<GLubyte>(static_cast<float>(rgb.r) * f);
        rgb.g = static_cast<GLubyte>(static_cast<float>(rgb.g) * f);
        rgb.b = static_cast<GLubyte>(static_cast<float>(rgb.b) * f);
    }
    ccColor4B const col = {rgb.r, rgb.g, rgb.b, a};
    m_verts[0].colors = col;
    m_verts[1].colors = col;
    m_verts[2].colors = col;
}

void MenuShatterTriangleNode::draw() {
    syncVertexColorsFromDisplayState();
    CC_NODE_DRAW_SETUP();

    ccGLBlendFunc(m_blendFunc.src, m_blendFunc.dst);
    ccGLBindTexture2D(m_texture->getName());

    ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

    GLsizei const stride = static_cast<GLsizei>(sizeof(ccV3F_C4B_T2F));
    auto const* base = static_cast<unsigned char*>(static_cast<void*>(m_verts));

    glVertexAttribPointer(
        kCCVertexAttrib_Position,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        base + offsetof(ccV3F_C4B_T2F, vertices)
    );
    glVertexAttribPointer(
        kCCVertexAttrib_Color,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        base + offsetof(ccV3F_C4B_T2F, colors)
    );
    glVertexAttribPointer(
        kCCVertexAttrib_TexCoords,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        base + offsetof(ccV3F_C4B_T2F, texCoords)
    );

    glDrawArrays(GL_TRIANGLES, 0, 3);
    CC_INCREMENT_GL_DRAWS(1);
}
