#pragma once

struct PhysicsState {
    float x, y, angle;
};

class PhysicsWorld {
public:
    PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH);
    ~PhysicsWorld();
    void step(float dt);
    PhysicsState getPlayerState() const;

    void setDragging(bool on);
    void setDragTargetPixels(float x, float y);
    void setDragGrabOffsetPixels(float offsetX, float offsetY);

private:
    struct Impl;
    Impl* m_impl;
    float m_worldW = 0.0f;
    float m_worldH = 0.0f;
    bool m_dragging = false;
    float m_dragTargetX = 0.0f;
    float m_dragTargetY = 0.0f;
    float m_grabLocalX = 0.0f;
    float m_grabLocalY = 0.0f;
};
