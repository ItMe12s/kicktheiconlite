#pragma once

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
