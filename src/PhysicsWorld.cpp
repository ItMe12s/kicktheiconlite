#include "PhysicsWorld.h"

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Arbiter.h"

#include <cmath>
#include <memory>

static constexpr float PPM = 50.0f;

static constexpr float kDragSpring = 250.0f;
static constexpr float kDragDamping = 10.0f;
static constexpr float kDragAngularDamping = 0.25f;

static constexpr float kEarthGravity = 9.8f;
static constexpr float kGravityScale = 2.0f;
static constexpr int kWorldIterations = 10;

static constexpr float kWallHalfThickness = 0.5f;
static constexpr float kWallLengthPadding = 4.0f;
static constexpr float kWallThickness = 1.0f;
static constexpr float kArenaCenterFrac = 0.5f;

static constexpr float kPlayerDensity = 1.0f;
static constexpr float kPlayerInitialXFrac = 0.25f;
static constexpr float kPlayerInitialYFrac = 0.75f;
static constexpr float kPlayerInitialVelX = 10.0f;
static constexpr float kPlayerInitialVelY = 5.0f;
static constexpr float kPlayerInitialAngularVel = 30.0f;
static constexpr float kPlayerFriction = 0.3f;

static constexpr float kDefaultDragTargetXFrac = 0.5f;
static constexpr float kDefaultDragTargetYFrac = 0.5f;

static constexpr float kMaxDeltaTime = 1.0f / 30.0f;

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
        float ww = worldW / PPM;
        float wh = worldH / PPM;
        float bw = bodyW / PPM;
        float bh = bodyH / PPM;

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
{}

PhysicsWorld::~PhysicsWorld() = default;

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
    if (dt > kMaxDeltaTime)
        dt = kMaxDeltaTime;

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

    m_preStepSpeedPx = getPlayerSpeed();

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

float PhysicsWorld::getPreStepPlayerSpeedPx() const {
    return m_preStepSpeedPx;
}

PhysicsVelocity PhysicsWorld::getPlayerVelocityPixels() const {
    Vec2 const& v = m_impl->player.velocity;
    return { v.x * PPM, v.y * PPM };
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
