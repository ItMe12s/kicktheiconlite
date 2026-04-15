#pragma once

namespace cocos2d {
class CCNode;
}

class PhysicsMenu {
    cocos2d::CCNode* m_root = nullptr;
    float m_width = 0.0f;
    float m_height = 0.0f;

public:
    PhysicsMenu() = default;
    ~PhysicsMenu();

    PhysicsMenu(PhysicsMenu const&) = delete;
    PhysicsMenu& operator=(PhysicsMenu const&) = delete;

    bool build(float width, float height);
    cocos2d::CCNode* getRoot() const { return m_root; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }

    bool containsLocalPoint(float localX, float localY) const;
};
