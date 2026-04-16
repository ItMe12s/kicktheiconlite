#include "PhysicsWorld.h"

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Arbiter.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

static constexpr float kPixelsPerMeter = 50.0f;

static constexpr float kEarthGravity = 9.8f;
static constexpr float kGravityScale = 1.75f;
static constexpr int kWorldIterations = 10;

static constexpr float kWallHalfThickness = 0.5f;
static constexpr float kWallLengthPadding = 4.0f;
static constexpr float kWallThickness = 1.0f;
static constexpr float kArenaCenterFrac = 0.5f;

static constexpr float kPlayerDensity = 1.0f;
static constexpr float kPlayerInitialXFrac = 0.25f;
static constexpr float kPlayerInitialYFrac = 0.25f;

static constexpr float kPlayerInitialVelX = 5.0f;
static constexpr float kPlayerInitialVelY = 10.0f;
static constexpr float kPlayerInitialAngularVel = 30.0f;
static constexpr float kPlayerFriction = 0.4f;

static constexpr float kDragSpring = 200.0f;
static constexpr float kDragDamping = 10.0f;
static constexpr float kDragAngularDamping = 0.2f;
static constexpr float kDefaultDragTargetXFrac = 0.5f;
static constexpr float kDefaultDragTargetYFrac = 0.5f;

static constexpr float kPanelDragSpring = 170.0f;
static constexpr float kPanelDragDamping = 15.0f;
static constexpr float kPanelDragAngularDamping = 0.3f;

static constexpr float kOutsideBarrierSlack = 1.2f;

static constexpr float kPanelDensity = 1.25;
static constexpr float kPanelFriction = 0.6f;

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

static float bodySpeedPx(Body const& body) {
    return std::hypot(body.velocity.x, body.velocity.y) * kPixelsPerMeter;
}

static float relativeSpeedPx(Body const& a, Body const& b) {
    float const rvx = a.velocity.x - b.velocity.x;
    float const rvy = a.velocity.y - b.velocity.y;
    return std::hypot(rvx, rvy) * kPixelsPerMeter;
}

struct DragTuning {
    float spring = 0.0f;
    float damping = 0.0f;
    float angularDamping = 0.0f;
};

static std::vector<Vec2> makeBoxPolygon(float widthMeters, float heightMeters) {
    float const hx = widthMeters * 0.5f;
    float const hy = heightMeters * 0.5f;
    return {
        Vec2(-hx, -hy),
        Vec2(hx, -hy),
        Vec2(hx, hy),
        Vec2(-hx, hy),
    };
}

static void bodyWorldExtents(Body const& body, float& outXExtent, float& outYExtent) {
    if (body.vertices.empty()) {
        outXExtent = 0.0f;
        outYExtent = 0.0f;
        return;
    }
    Mat22 const rot(body.rotation);
    float maxX = 0.0f;
    float maxY = 0.0f;
    for (Vec2 const& local : body.vertices) {
        Vec2 const w = rot * local;
        maxX = std::max(maxX, std::fabs(w.x));
        maxY = std::max(maxY, std::fabs(w.y));
    }
    outXExtent = maxX;
    outYExtent = maxY;
}

static void applyDragForces(
    Body& body,
    float grabLocalX,
    float grabLocalY,
    float dragTargetX,
    float dragTargetY,
    DragTuning const& tuning
) {
    float const th = body.rotation;
    float const c = std::cos(th);
    float const s = std::sin(th);
    float const rx = c * grabLocalX - s * grabLocalY;
    float const ry = s * grabLocalX + c * grabLocalY;
    float const gpx = body.position.x + rx;
    float const gpy = body.position.y + ry;
    float const tx = dragTargetX / kPixelsPerMeter;
    float const ty = dragTargetY / kPixelsPerMeter;
    float const ex = tx - gpx;
    float const ey = ty - gpy;
    // Velocity at grab point v + omega x r -> (vx - w*ry, vy + w*rx)
    float const vpx = body.velocity.x - body.angularVelocity * ry;
    float const vpy = body.velocity.y + body.angularVelocity * rx;
    float const forceX = tuning.spring * ex - tuning.damping * vpx;
    float const forceY = tuning.spring * ey - tuning.damping * vpy;
    body.AddForce(Vec2(forceX, forceY));
    body.torque += rx * forceY - ry * forceX;
    body.torque -= tuning.angularDamping * body.angularVelocity;
}

static void clampBodyToScreenBorder(Body& body, float worldWidthMeters, float worldHeightMeters) {
    float ex = 0.0f;
    float ey = 0.0f;
    bodyWorldExtents(body, ex, ey);

    float const left = body.position.x - ex;
    float const right = body.position.x + ex;
    float const bottom = body.position.y - ey;
    float const top = body.position.y + ey;

    float const padX = worldWidthMeters * (kOutsideBarrierSlack - 1.0f);
    float const padY = worldHeightMeters * (kOutsideBarrierSlack - 1.0f);

    float const snapMinX = ex;
    float const snapMaxX = worldWidthMeters - ex;
    float const snapMinY = ey;
    float const snapMaxY = worldHeightMeters - ey;

    if (snapMinX <= snapMaxX) {
        if (left < -padX) {
            body.position.x = snapMinX;
        } else if (right > worldWidthMeters + padX) {
            body.position.x = snapMaxX;
        }
    } else {
        body.position.x = worldWidthMeters * 0.5f;
    }

    if (snapMinY <= snapMaxY) {
        if (bottom < -padY) {
            body.position.y = snapMinY;
        } else if (top > worldHeightMeters + padY) {
            body.position.y = snapMaxY;
        }
    } else {
        body.position.y = worldHeightMeters * 0.5f;
    }
}

struct PhysicsWorld::Impl {
    World world;
    Body wallBottom;
    Body wallTop;
    Body wallLeft;
    Body wallRight;
    Body player;
    std::unique_ptr<Body> panel;
    std::vector<std::unique_ptr<Body>> shatterBodies;

    Impl(float worldW, float worldH, float bodyW, float bodyH)
        : world(Vec2(0.0f, -kEarthGravity * kGravityScale), kWorldIterations)
    {
        float ww = worldW / kPixelsPerMeter;
        float wh = worldH / kPixelsPerMeter;
        float bw = bodyW / kPixelsPerMeter;
        float bh = bodyH / kPixelsPerMeter;
        std::vector<Vec2> const horizontalWallPoly = makeBoxPolygon(ww + kWallLengthPadding, kWallThickness);
        std::vector<Vec2> const verticalWallPoly = makeBoxPolygon(kWallThickness, wh + kWallLengthPadding);
        std::vector<Vec2> const playerPoly = makeBoxPolygon(bw, bh);

        wallBottom.Set(horizontalWallPoly.data(), static_cast<int>(horizontalWallPoly.size()), FLT_MAX);
        wallBottom.position.Set(ww * kArenaCenterFrac, -kWallHalfThickness);

        wallTop.Set(horizontalWallPoly.data(), static_cast<int>(horizontalWallPoly.size()), FLT_MAX);
        wallTop.position.Set(ww * kArenaCenterFrac, wh + kWallHalfThickness);

        wallLeft.Set(verticalWallPoly.data(), static_cast<int>(verticalWallPoly.size()), FLT_MAX);
        wallLeft.position.Set(-kWallHalfThickness, wh * kArenaCenterFrac);

        wallRight.Set(verticalWallPoly.data(), static_cast<int>(verticalWallPoly.size()), FLT_MAX);
        wallRight.position.Set(ww + kWallHalfThickness, wh * kArenaCenterFrac);

        player.Set(playerPoly.data(), static_cast<int>(playerPoly.size()), kPlayerDensity);
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
    clampBodyToScreenBorder(m_impl->player, ww, wh);
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
    if (m_impl->panel) {
        m_panelPrevRender = getPanelState();
    }

    if (m_dragging) {
        applyDragForces(
            m_impl->player,
            m_grabLocalX,
            m_grabLocalY,
            m_dragTargetX,
            m_dragTargetY,
            DragTuning{kDragSpring, kDragDamping, kDragAngularDamping}
        );
    }

    if (m_panelDragging && m_impl->panel) {
        applyDragForces(
            *m_impl->panel,
            m_panelGrabLocalX,
            m_panelGrabLocalY,
            m_panelDragTargetX,
            m_panelDragTargetY,
            DragTuning{kPanelDragSpring, kPanelDragDamping, kPanelDragAngularDamping}
        );
    }

    m_preStepSpeedPx = bodySpeedPx(m_impl->player);
    m_preStepPanelSpeedPx = m_impl->panel ? bodySpeedPx(*m_impl->panel) : 0.0f;

    m_impl->world.Step(dt);
    clampPlayerToScreenBorder();
    clampPanelToScreenBorder();
    float const ww = m_worldW / kPixelsPerMeter;
    float const wh = m_worldH / kPixelsPerMeter;
    for (auto const& shard : m_impl->shatterBodies) {
        if (shard) {
            clampBodyToScreenBorder(*shard, ww, wh);
        }
    }
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
    return bodySpeedPx(m_impl->player);
}

PhysicsVelocity PhysicsWorld::getPlayerVelocityPixels() const {
    Vec2 const& v = m_impl->player.velocity;
    return { v.x * kPixelsPerMeter, v.y * kPixelsPerMeter };
}

PhysicsImpactEvent PhysicsWorld::consumePlayerImpactAny() {
    Body* const player = &m_impl->player;
    bool now = false;
    float maxRelativeSpeedPx = 0.0f;
    for (auto const& kv : m_impl->world.arbiters) {
        Arbiter const& arb = kv.second;
        if (arb.numContacts <= 0) {
            continue;
        }
        if (arb.body1 == player || arb.body2 == player) {
            now = true;
            Body* const other = arb.body1 == player ? arb.body2 : arb.body1;
            if (other) {
                maxRelativeSpeedPx = std::max(maxRelativeSpeedPx, relativeSpeedPx(*player, *other));
            }
        }
    }

    PhysicsImpactEvent event{};
    event.triggered = now && !m_wasPlayerAgainstAnyBody;
    event.preSpeedPx = m_preStepSpeedPx;
    event.postSpeedPx = getPlayerSpeed();
    event.impactSpeedPx = std::max({m_preStepSpeedPx, event.postSpeedPx, maxRelativeSpeedPx});
    m_wasPlayerAgainstAnyBody = now;
    return event;
}

PhysicsImpactEvent PhysicsWorld::consumePanelImpactAny() {
    if (!m_impl->panel) {
        m_wasPanelAgainstAnyBody = false;
        return {};
    }

    Body* const panel = m_impl->panel.get();
    bool now = false;
    float maxRelativeSpeedPx = 0.0f;
    for (auto const& kv : m_impl->world.arbiters) {
        Arbiter const& arb = kv.second;
        if (arb.numContacts <= 0) {
            continue;
        }
        if (arb.body1 == panel || arb.body2 == panel) {
            now = true;
            Body* const other = arb.body1 == panel ? arb.body2 : arb.body1;
            if (other) {
                maxRelativeSpeedPx = std::max(maxRelativeSpeedPx, relativeSpeedPx(*panel, *other));
            }
        }
    }

    PhysicsImpactEvent event{};
    event.triggered = now && !m_wasPanelAgainstAnyBody;
    event.preSpeedPx = m_preStepPanelSpeedPx;
    event.postSpeedPx = bodySpeedPx(*panel);
    event.impactSpeedPx = std::max({m_preStepPanelSpeedPx, event.postSpeedPx, maxRelativeSpeedPx});
    m_wasPanelAgainstAnyBody = now;
    return event;
}

void PhysicsWorld::spawnPanel(float bodyW, float bodyH, float xPx, float yPx) {
    if (m_impl->panel) {
        return;
    }
    auto panel = std::make_unique<Body>();
    float const bw = bodyW / kPixelsPerMeter;
    float const bh = bodyH / kPixelsPerMeter;
    std::vector<Vec2> const panelPoly = makeBoxPolygon(bw, bh);
    panel->Set(panelPoly.data(), static_cast<int>(panelPoly.size()), kPanelDensity);
    panel->position.Set(xPx / kPixelsPerMeter, yPx / kPixelsPerMeter);
    panel->velocity.Set(0.0f, 0.0f);
    panel->angularVelocity = 0.0f;
    panel->friction = kPanelFriction;
    m_impl->world.Add(panel.get());
    m_impl->panel = std::move(panel);
    m_panelDragging = false;
    m_panelDragTargetX = xPx;
    m_panelDragTargetY = yPx;
    m_panelGrabLocalX = 0.0f;
    m_panelGrabLocalY = 0.0f;
    m_preStepPanelSpeedPx = 0.0f;
    m_wasPanelAgainstAnyBody = false;
    m_panelPrevRender = getPanelState();
}

void PhysicsWorld::destroyPanel() {
    if (!m_impl->panel) {
        return;
    }
    m_impl->world.Remove(m_impl->panel.get());
    m_impl->panel.reset();
    m_panelDragging = false;
    m_preStepPanelSpeedPx = 0.0f;
    m_wasPanelAgainstAnyBody = false;
}

bool PhysicsWorld::hasPanel() const {
    return m_impl->panel != nullptr;
}

PhysicsState PhysicsWorld::getPanelState() const {
    if (!m_impl->panel) {
        return {0.0f, 0.0f, 0.0f};
    }
    Body const& p = *m_impl->panel;
    return {
        p.position.x * kPixelsPerMeter,
        p.position.y * kPixelsPerMeter,
        p.rotation
    };
}

PhysicsState PhysicsWorld::getPanelRenderState(float alpha) const {
    PhysicsState const curr = getPanelState();
    float const a = std::clamp(alpha, 0.0f, 1.0f);
    float const om = 1.0f - a;
    return {
        m_panelPrevRender.x * om + curr.x * a,
        m_panelPrevRender.y * om + curr.y * a,
        lerpAngleRad(m_panelPrevRender.angle, curr.angle, a),
    };
}

PhysicsVelocity PhysicsWorld::getPanelVelocityPixels() const {
    if (!m_impl->panel) {
        return {0.0f, 0.0f};
    }
    Vec2 const& v = m_impl->panel->velocity;
    return {v.x * kPixelsPerMeter, v.y * kPixelsPerMeter};
}

int PhysicsWorld::getBodyCount() const {
    return static_cast<int>(m_impl->world.bodies.size());
}

int PhysicsWorld::getJointCount() const {
    return static_cast<int>(m_impl->world.joints.size());
}

int PhysicsWorld::getArbiterCount() const {
    return static_cast<int>(m_impl->world.arbiters.size());
}

void PhysicsWorld::setPanelDragging(bool on) {
    m_panelDragging = on;
}

void PhysicsWorld::setPanelDragTargetPixels(float x, float y) {
    float const maxX = m_worldW;
    float const maxY = m_worldH;
    if (x < 0.0f) x = 0.0f;
    else if (x > maxX) x = maxX;
    if (y < 0.0f) y = 0.0f;
    else if (y > maxY) y = maxY;
    m_panelDragTargetX = x;
    m_panelDragTargetY = y;
}

void PhysicsWorld::setPanelDragGrabOffsetPixels(float offsetX, float offsetY) {
    if (!m_impl->panel) {
        m_panelGrabLocalX = 0.0f;
        m_panelGrabLocalY = 0.0f;
        return;
    }
    Body const& p = *m_impl->panel;
    float const wx = offsetX / kPixelsPerMeter;
    float const wy = offsetY / kPixelsPerMeter;
    float const c = std::cos(p.rotation);
    float const s = std::sin(p.rotation);
    m_panelGrabLocalX = c * wx + s * wy;
    m_panelGrabLocalY = -s * wx + c * wy;
}

int PhysicsWorld::spawnShatterBody(PhysicsShatterBodyInit const& init) {
    auto body = std::make_unique<Body>();
    std::vector<Vec2> const shardPoly = makeBoxPolygon(
        init.widthPx / kPixelsPerMeter,
        init.heightPx / kPixelsPerMeter
    );
    body->Set(shardPoly.data(), static_cast<int>(shardPoly.size()), init.density);
    body->position.Set(init.xPx / kPixelsPerMeter, init.yPx / kPixelsPerMeter);
    body->rotation = init.angleRad;
    body->velocity.Set(init.velocityXPx / kPixelsPerMeter, init.velocityYPx / kPixelsPerMeter);
    body->angularVelocity = init.angularVelocityRad;
    body->friction = init.friction;
    m_impl->world.Add(body.get());
    m_impl->shatterBodies.push_back(std::move(body));
    return static_cast<int>(m_impl->shatterBodies.size() - 1);
}

bool PhysicsWorld::getShatterBodyState(int handle, PhysicsState& out) const {
    if (handle < 0) {
        return false;
    }
    size_t const index = static_cast<size_t>(handle);
    if (index >= m_impl->shatterBodies.size()) {
        return false;
    }
    auto const& body = m_impl->shatterBodies[index];
    if (!body) {
        return false;
    }
    out = {
        body->position.x * kPixelsPerMeter,
        body->position.y * kPixelsPerMeter,
        body->rotation,
    };
    return true;
}

void PhysicsWorld::clearShatterBodies() {
    for (auto& body : m_impl->shatterBodies) {
        if (body) {
            m_impl->world.Remove(body.get());
            body.reset();
        }
    }
    m_impl->shatterBodies.clear();
}

int PhysicsWorld::getShatterBodyCount() const {
    return static_cast<int>(m_impl->shatterBodies.size());
}

void PhysicsWorld::clampPanelToScreenBorder() {
    if (!m_impl->panel) {
        return;
    }
    float const ww = m_worldW / kPixelsPerMeter;
    float const wh = m_worldH / kPixelsPerMeter;
    clampBodyToScreenBorder(*m_impl->panel, ww, wh);
}
