#include "PhysicsWorld.h"

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"

static constexpr float PPM = 50.0f;

struct PhysicsWorld::Impl {
    World world;
    Body wallBottom;
    Body wallTop;
    Body wallLeft;
    Body wallRight;
    Body player;

    Impl(float worldW, float worldH, float bodyW, float bodyH)
        : world(Vec2(0.0f, -9.8f), 10)
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
        player.position.Set(ww * 0.5f, wh * 0.5f);
        player.velocity.Set(10.0f, 5.0f);
        player.angularVelocity = 10.0f;
        player.friction = 0.1f;

        world.Add(&wallBottom);
        world.Add(&wallTop);
        world.Add(&wallLeft);
        world.Add(&wallRight);
        world.Add(&player);
    }
};

PhysicsWorld::PhysicsWorld(float worldW, float worldH, float bodyW, float bodyH)
    : m_impl(new Impl(worldW, worldH, bodyW, bodyH))
{}

PhysicsWorld::~PhysicsWorld() {
    delete m_impl;
}

void PhysicsWorld::step(float dt) {
    // Those who lag:
    if (dt > 1.0f / 30.0f)
        dt = 1.0f / 30.0f;
    m_impl->world.Step(dt);
}

PhysicsState PhysicsWorld::getPlayerState() const {
    return {
        m_impl->player.position.x * PPM,
        m_impl->player.position.y * PPM,
        m_impl->player.rotation
    };
}
