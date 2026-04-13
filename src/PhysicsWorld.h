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

private:
    struct Impl;
    Impl* m_impl;
};
