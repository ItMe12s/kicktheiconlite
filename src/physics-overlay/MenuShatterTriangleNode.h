#pragma once

#include <Geode/cocos/base_nodes/CCNode.h>
#include <Geode/cocos/include/ccTypes.h>

namespace cocos2d {
class CCTexture2D;
}

/// Textured triangle shard (CCSprite has no polygon path in this engine)
class MenuShatterTriangleNode : public cocos2d::CCNodeRGBA {
public:
    using CCNodeRGBA::init;

    static MenuShatterTriangleNode* create(
        cocos2d::CCTexture2D* texture,
        cocos2d::CCPoint const& local0,
        cocos2d::CCPoint const& local1,
        cocos2d::CCPoint const& local2,
        cocos2d::ccTex2F const& uv0,
        cocos2d::ccTex2F const& uv1,
        cocos2d::ccTex2F const& uv2,
        bool flipTextureY = false
    );

    bool init(
        cocos2d::CCTexture2D* texture,
        cocos2d::CCPoint const& local0,
        cocos2d::CCPoint const& local1,
        cocos2d::CCPoint const& local2,
        cocos2d::ccTex2F const& uv0,
        cocos2d::ccTex2F const& uv1,
        cocos2d::ccTex2F const& uv2,
        bool flipTextureY = false
    );

    ~MenuShatterTriangleNode() override;

    void draw() override;
    void setOpacity(GLubyte opacity) override;
    void updateDisplayedOpacity(GLubyte parentOpacity) override;

private:
    void syncVertexColorsFromDisplayState();

    cocos2d::CCTexture2D* m_texture = nullptr;
    cocos2d::ccV3F_C4B_T2F m_verts[3]{};
    cocos2d::ccBlendFunc m_blendFunc{};
    bool m_flipTextureY = false;
};
