#include "PhysicsWorld.h"

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Arbiter.h"

#include <cmath>

static constexpr float PPM = 50.0f;

static constexpr float kDragSpring = 250.0f;
static constexpr float kDragDamping = 15.0f;
static constexpr float kDragAngularDamping = 0.5f;

struct PhysicsWorld::Impl {
    World world;
    Body wallBottom;
    Body wallTop;
    Body wallLeft;
    Body wallRight;
    Body player;

    Impl(float worldW, float worldH, float bodyW, float bodyH)
        : world(Vec2(0.0f, -9.8f * 2.0f), 10) // I'm making all this physics configurable later
    {
        float ww = worldW / PPM;
        float wh = worldH / PPM;
        float bw = bodyW / PPM;
        float bh = bodyH / PPM;

        wallBottom.position.Set(ww * 0.5f, -0.5f);
        wallBottom.width.Set(ww + 4.0f, 1.0f);

        wallTop.position.Set(ww * 0.5f, wh + 0.5f);
        wallTop.width.Set(ww + 4.0f, 1.0f);

        wallLeft.position.Set(-0.5f, wh * 0.5f);
        wallLeft.width.Set(1.0f, wh + 4.0f);

        wallRight.position.Set(ww + 0.5f, wh * 0.5f);
        wallRight.width.Set(1.0f, wh + 4.0f);

        player.Set(Vec2(bw, bh), 1.0f);
        player.position.Set(ww * 0.25f, wh * 0.75f);
        player.velocity.Set(10.0f, 5.0f);
        player.angularVelocity = 30.0f;
        player.friction = 0.3f;

        world.Add(&wallBottom);
        world.Add(&wallTop);
        world.Add(&wallLeft);
        world.Add(&wallRight);
        world.Add(&player);
    }
};

PhysicsWorld::PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH)
    : m_impl(new Impl(worldW, worldH, bodyW, bodyH))
    , m_worldW(worldW)
    , m_worldH(worldH)
    , m_dragging(false)
    , m_dragTargetX(worldW * 0.5f)
    , m_dragTargetY(worldH * 0.5f)
{}

PhysicsWorld::~PhysicsWorld() {
    delete m_impl;
}

void PhysicsWorld::setDragging(bool on) {
    m_dragging = on;
}

void PhysicsWorld::setDragGrabOffsetPixels(float offsetX, float offsetY) {
    Body const& p = m_impl->player;
    float const wx = offsetX / PPM;
    float const wy = offsetY / PPM;
    float const c = std::cos(p.rotation);
    float const s = std::sin(p.rotation);
    // World offset (meters) -> body-local: R(-theta) * w
    m_grabLocalX = c * wx + s * wy;
    m_grabLocalY = -s * wx + c * wy;
}

void PhysicsWorld::setDragTargetPixels(float x, float y) {
    float const minX = 0.0f;
    float const minY = 0.0f;
    float const maxX = m_worldW;
    float const maxY = m_worldH;
    if (x < minX) {
        x = minX;
    } else if (x > maxX) {
        x = maxX;
    }
    if (y < minY) {
        y = minY;
    } else if (y > maxY) {
        y = maxY;
    }
    m_dragTargetX = x;
    m_dragTargetY = y;
}

void PhysicsWorld::step(float dt) {
    // Those who lag:
    if (dt > 1.0f / 30.0f)
        dt = 1.0f / 30.0f;

    if (m_dragging) {
        Body& p = m_impl->player;
        float const th = p.rotation;
        float const c = std::cos(th);
        float const s = std::sin(th);
        // Grab point in world (meters): center + R(theta) * local
        float const rx = c * m_grabLocalX - s * m_grabLocalY;
        float const ry = s * m_grabLocalX + c * m_grabLocalY;
        float const gpx = p.position.x + rx;
        float const gpy = p.position.y + ry;
        float const tx = m_dragTargetX / PPM;
        float const ty = m_dragTargetY / PPM;
        float const ex = tx - gpx;
        float const ey = ty - gpy;
        // Velocity at grab point v + omega x r  -> (vx - w*ry, vy + w*rx)
        float const vpx = p.velocity.x - p.angularVelocity * ry;
        float const vpy = p.velocity.y + p.angularVelocity * rx;
        float const Fx = kDragSpring * ex - kDragDamping * vpx;
        float const Fy = kDragSpring * ey - kDragDamping * vpy;
        p.AddForce(Vec2(Fx, Fy));
        p.torque += rx * Fy - ry * Fx;
        p.torque -= kDragAngularDamping * p.angularVelocity;
    }

    m_impl->world.Step(dt);
}

PhysicsState PhysicsWorld::getPlayerState() const {
    return {
        m_impl->player.position.x * PPM,
        m_impl->player.position.y * PPM,
        m_impl->player.rotation
    };
}

float PhysicsWorld::getPlayerSpeed() const {
    Vec2 const& v = m_impl->player.velocity;
    return std::hypot(v.x, v.y) * PPM;
}

bool PhysicsWorld::hasPlayerWallContact() const {
    Body* const player = &m_impl->player;
    for (auto const& kv : m_impl->world.arbiters) {
        Arbiter const& arb = kv.second;
        if (arb.numContacts <= 0) {
            continue;
        }
        Body* const a = arb.body1;
        Body* const b = arb.body2;
        auto const isWall = [&](Body* w) {
            return w == &m_impl->wallBottom || w == &m_impl->wallTop || w == &m_impl->wallLeft
                || w == &m_impl->wallRight;
        };
        if (a == player && isWall(b)) {
            return true;
        }
        if (b == player && isWall(a)) {
            return true;
        }
    }
    return false;
}

bool PhysicsWorld::consumeWallImpact() {
    bool const now = hasPlayerWallContact();
    bool const impact = now && !m_wasPlayerAgainstWall;
    m_wasPlayerAgainstWall = now;
    return impact;
}
