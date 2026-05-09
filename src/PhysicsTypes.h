#pragma once

// Shared physics snapshots (all use pixel space and radians for angle).
// PhysicsImpactEvent: per-substep collision feedback, PhysicsOverlay merges multiple
// substeps into m_lastPlayerImpact via max speeds (see mergeImpactSnapshot).
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
