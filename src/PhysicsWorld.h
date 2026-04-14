#pragma once

#include <memory>

struct PhysicsState {
    float x, y, angle;
};

struct PhysicsVelocity {
    float vx;
    float vy;
};

class PhysicsWorld {
public:
    PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH);
    ~PhysicsWorld();
    void step(float dt);
    PhysicsState getPlayerState() const;
    PhysicsState getPlayerRenderState(float alpha) const;

    float getPlayerSpeed() const;
    PhysicsVelocity getPlayerVelocityPixels() const;

    float getPreStepPlayerSpeedPx() const;

    void setDragging(bool on);
    void setDragTargetPixels(float x, float y);
    void setDragGrabOffsetPixels(float offsetX, float offsetY);

    bool consumeWallImpact();

private:
    void clampPlayerToScreenBoarder();
    bool hasPlayerWallContact() const;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    float m_worldW = 0.0f;
    float m_worldH = 0.0f;
    bool m_dragging = false;
    float m_dragTargetX = 0.0f;
    float m_dragTargetY = 0.0f;
    float m_grabLocalX = 0.0f;
    float m_grabLocalY = 0.0f;
    bool m_wasPlayerAgainstWall = false;
    float m_preStepSpeedPx = 0.0f;
    PhysicsState m_playerPrevRender{};
};
