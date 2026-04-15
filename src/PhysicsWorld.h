#pragma once

#include <memory>

struct PhysicsState {
    float x, y, angle;
};

struct PhysicsVelocity {
    float vx;
    float vy;
};

struct PhysicsImpactEvent {
    bool triggered = false;
    float preSpeedPx = 0.0f;
    float postSpeedPx = 0.0f;
    float impactSpeedPx = 0.0f;
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

    void setDragging(bool on);
    void setDragTargetPixels(float x, float y);
    void setDragGrabOffsetPixels(float offsetX, float offsetY);

    PhysicsImpactEvent consumePlayerImpactAny();
    PhysicsImpactEvent consumePanelImpactAny();

    void spawnPanel(float bodyW, float bodyH, float xPx, float yPx);
    void destroyPanel();
    bool hasPanel() const;
    PhysicsState getPanelState() const;
    PhysicsState getPanelRenderState(float alpha) const;
    PhysicsVelocity getPanelVelocityPixels() const;
    int getBodyCount() const;
    int getJointCount() const;
    int getArbiterCount() const;

    void setPanelDragging(bool on);
    void setPanelDragTargetPixels(float x, float y);
    void setPanelDragGrabOffsetPixels(float offsetX, float offsetY);

private:
    void clampPlayerToScreenBorder();
    void clampPanelToScreenBorder();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    float m_worldW = 0.0f;
    float m_worldH = 0.0f;
    bool m_dragging = false;
    float m_dragTargetX = 0.0f;
    float m_dragTargetY = 0.0f;
    float m_grabLocalX = 0.0f;
    float m_grabLocalY = 0.0f;
    bool m_wasPlayerAgainstAnyBody = false;
    float m_preStepSpeedPx = 0.0f;
    PhysicsState m_playerPrevRender{};

    bool m_panelDragging = false;
    float m_panelDragTargetX = 0.0f;
    float m_panelDragTargetY = 0.0f;
    float m_panelGrabLocalX = 0.0f;
    float m_panelGrabLocalY = 0.0f;
    bool m_wasPanelAgainstAnyBody = false;
    float m_preStepPanelSpeedPx = 0.0f;
    PhysicsState m_panelPrevRender{};
};
