#pragma once

#include <cstdint>
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

enum class PhysicsShatterBodyShape : std::uint8_t { Box, Triangle };

struct PhysicsShatterBodyInit {
    PhysicsShatterBodyShape shape = PhysicsShatterBodyShape::Box;
    float widthPx = 0.0f;
    float heightPx = 0.0f;
    float cornerWorldPx[3][2] = {};
    float xPx = 0.0f;
    float yPx = 0.0f;
    float angleRad = 0.0f;
    float velocityXPx = 0.0f;
    float velocityYPx = 0.0f;
    float angularVelocityRad = 0.0f;
    float friction = 0.45f;
    float density = 0.85f;
    float linearDampingPerSecond = 0.0f;
    float angularDampingPerSecond = 0.0f;
};

struct PhysicsDynamicObjectInit {
    float widthPx = 0.0f;
    float heightPx = 0.0f;
    float xPx = 0.0f;
    float yPx = 0.0f;
    float angleRad = 0.0f;
    float velocityXPx = 0.0f;
    float velocityYPx = 0.0f;
    float angularVelocityRad = 0.0f;
    float density = 1.0f;
    float friction = 0.4f;
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
    int spawnShatterBody(PhysicsShatterBodyInit const& init);
    bool getShatterBodyState(int handle, PhysicsState& out) const;
    void clearShatterBodies();
    int getShatterBodyCount() const;

    int spawnDynamicObject(PhysicsDynamicObjectInit const& init);
    void destroyDynamicObject(int handle);
    bool hasDynamicObject(int handle) const;
    void clearAllDynamicObjects();
    PhysicsState getDynamicObjectState(int handle) const;
    PhysicsState getDynamicObjectRenderState(int handle, float alpha) const;
    PhysicsVelocity getDynamicObjectVelocityPixels(int handle) const;
    void setDynamicObjectDragging(int handle, bool on);
    void setDynamicObjectDragTargetPixels(int handle, float x, float y);
    void setDynamicObjectDragGrabOffsetPixels(int handle, float offsetX, float offsetY);
    int dynamicObjectCount() const;

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
