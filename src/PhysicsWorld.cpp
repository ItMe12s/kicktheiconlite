#include "PhysicsWorld.h"

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Arbiter.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>

static constexpr float kPixelsPerMeter = 50.0f;

static constexpr float kEarthGravity = 9.8f;
static constexpr float kGravityScale = 2.0f;
static constexpr int kWorldIterations = 10;

static constexpr float kWallHalfThickness = 0.5f;
static constexpr float kWallLengthPadding = 4.0f;
static constexpr float kWallThickness = 1.0f;
static constexpr float kArenaCenterFrac = 0.5f;

static constexpr float kPlayerDensity = 1.0f;
static constexpr float kPlayerInitialXFrac = 0.25f;
static constexpr float kPlayerInitialYFrac = 0.25f;
static constexpr float kPlayerInitialVelX = 10.0f;
static constexpr float kPlayerInitialVelY = 5.0f;
static constexpr float kPlayerInitialAngularVel = 30.0f;
static constexpr float kPlayerFriction = 0.4f;

static constexpr float kDragSpring = 250.0f;
static constexpr float kDragDamping = 10.0f;
static constexpr float kDragAngularDamping = 0.25f;
static constexpr float kDefaultDragTargetXFrac = 0.5f;
static constexpr float kDefaultDragTargetYFrac = 0.5f;

static constexpr float kOutsideBarrierSlack = 1.2f;

static float lerpAngleRad(float aRad, float bRad, float t) {
    float const pi = std::numbers::pi_v<float>;
    float d = bRad - aRad;
    while (d > pi) {
        d -= 2.0f * pi;
    }
    while (d < -pi) {
        d += 2.0f * pi;
    }
    return aRad + d * t;
}

struct PhysicsWorld::Impl {
    World world;
    Body wallBottom;
    Body wallTop;
    Body wallLeft;
    Body wallRight;
    Body player;

    Impl(float worldW, float worldH, float bodyW, float bodyH)
        : world(Vec2(0.0f, -kEarthGravity * kGravityScale), kWorldIterations)
    {
        float ww = worldW / kPixelsPerMeter;
        float wh = worldH / kPixelsPerMeter;
        float bw = bodyW / kPixelsPerMeter;
        float bh = bodyH / kPixelsPerMeter;

        wallBottom.position.Set(ww * kArenaCenterFrac, -kWallHalfThickness);
        wallBottom.width.Set(ww + kWallLengthPadding, kWallThickness);

        wallTop.position.Set(ww * kArenaCenterFrac, wh + kWallHalfThickness);
        wallTop.width.Set(ww + kWallLengthPadding, kWallThickness);

        wallLeft.position.Set(-kWallHalfThickness, wh * kArenaCenterFrac);
        wallLeft.width.Set(kWallThickness, wh + kWallLengthPadding);

        wallRight.position.Set(ww + kWallHalfThickness, wh * kArenaCenterFrac);
        wallRight.width.Set(kWallThickness, wh + kWallLengthPadding);

        player.Set(Vec2(bw, bh), kPlayerDensity);
        player.position.Set(ww * kPlayerInitialXFrac, wh * kPlayerInitialYFrac);
        player.velocity.Set(kPlayerInitialVelX, kPlayerInitialVelY);
        player.angularVelocity = kPlayerInitialAngularVel;
        player.friction = kPlayerFriction;

        world.Add(&wallBottom);
        world.Add(&wallTop);
        world.Add(&wallLeft);
        world.Add(&wallRight);
        world.Add(&player);
    }
};

PhysicsWorld::PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH)
    : m_impl(std::make_unique<Impl>(worldW, worldH, bodyW, bodyH))
    , m_worldW(worldW)
    , m_worldH(worldH)
    , m_dragging(false)
    , m_dragTargetX(worldW * kDefaultDragTargetXFrac)
    , m_dragTargetY(worldH * kDefaultDragTargetYFrac)
{
    m_playerPrevRender = getPlayerState();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::clampPlayerToScreenBorder() {
    float const ww = m_worldW / kPixelsPerMeter;
    float const wh = m_worldH / kPixelsPerMeter;
    Body& p = m_impl->player;
    float const a = p.width.x * 0.5f;
    float const b = p.width.y * 0.5f;
    float const c = std::cos(p.rotation);
    float const s = std::sin(p.rotation);
    float const ex = std::fabs(c) * a + std::fabs(s) * b;
    float const ey = std::fabs(s) * a + std::fabs(c) * b;

    float const left = p.position.x - ex;
    float const right = p.position.x + ex;
    float const bottom = p.position.y - ey;
    float const top = p.position.y + ey;

    float const padX = ww * (kOutsideBarrierSlack - 1.0f);
    float const padY = wh * (kOutsideBarrierSlack - 1.0f);

    float const snapMinX = ex;
    float const snapMaxX = ww - ex;
    float const snapMinY = ey;
    float const snapMaxY = wh - ey;

    if (snapMinX <= snapMaxX) {
        if (left < -padX) {
            p.position.x = snapMinX;
        } else if (right > ww + padX) {
            p.position.x = snapMaxX;
        }
    } else {
        p.position.x = ww * 0.5f;
    }

    if (snapMinY <= snapMaxY) {
        if (bottom < -padY) {
            p.position.y = snapMinY;
        } else if (top > wh + padY) {
            p.position.y = snapMaxY;
        }
    } else {
        p.position.y = wh * 0.5f;
    }
}

void PhysicsWorld::setDragging(bool on) {
    m_dragging = on;
}

void PhysicsWorld::setDragGrabOffsetPixels(float offsetX, float offsetY) {
    Body const& p = m_impl->player;
    float const wx = offsetX / kPixelsPerMeter;
    float const wy = offsetY / kPixelsPerMeter;
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
    m_playerPrevRender = getPlayerState();

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
        float const tx = m_dragTargetX / kPixelsPerMeter;
        float const ty = m_dragTargetY / kPixelsPerMeter;
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

    m_preStepSpeedPx = getPlayerSpeed();

    m_impl->world.Step(dt);
    clampPlayerToScreenBorder();
}

PhysicsState PhysicsWorld::getPlayerState() const {
    return {
        m_impl->player.position.x * kPixelsPerMeter,
        m_impl->player.position.y * kPixelsPerMeter,
        m_impl->player.rotation
    };
}

PhysicsState PhysicsWorld::getPlayerRenderState(float alpha) const {
    PhysicsState const curr = getPlayerState();
    float const a = std::clamp(alpha, 0.0f, 1.0f);
    float const om = 1.0f - a;
    return {
        m_playerPrevRender.x * om + curr.x * a,
        m_playerPrevRender.y * om + curr.y * a,
        lerpAngleRad(m_playerPrevRender.angle, curr.angle, a),
    };
}

float PhysicsWorld::getPlayerSpeed() const {
    Vec2 const& v = m_impl->player.velocity;
    return std::hypot(v.x, v.y) * kPixelsPerMeter;
}

float PhysicsWorld::getPreStepPlayerSpeedPx() const {
    return m_preStepSpeedPx;
}

PhysicsVelocity PhysicsWorld::getPlayerVelocityPixels() const {
    Vec2 const& v = m_impl->player.velocity;
    return { v.x * kPixelsPerMeter, v.y * kPixelsPerMeter };
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
