#pragma once

namespace cocos2d {
class CCNode;
}

class PhysicsTestPanel {
    cocos2d::CCNode* m_root = nullptr;
    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_titleBarHeight = 0.0f;

public:
    PhysicsTestPanel() = default;
    ~PhysicsTestPanel();

    PhysicsTestPanel(PhysicsTestPanel const&) = delete;
    PhysicsTestPanel& operator=(PhysicsTestPanel const&) = delete;

    bool build(float width, float height);
    cocos2d::CCNode* getRoot() const { return m_root; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    float getTitleBarHeight() const { return m_titleBarHeight; }

    bool isInsideTitleBar(float localX, float localY) const;
};
