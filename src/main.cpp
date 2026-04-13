#include <Geode/Geode.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include "PhysicsWorld.h"

using namespace geode::prelude;

class PhysicsOverlay : public CCNode {
    PhysicsWorld* m_physics = nullptr;
    CCSprite* m_sprite = nullptr;

public:
    CREATE_FUNC(PhysicsOverlay);
    bool init();
    void update(float dt) override;
    void onExit() override;
};

bool PhysicsOverlay::init() {
    if (!CCNode::init())
        return false;

    auto winSize = CCDirector::get()->getWinSize();
    float smaller = winSize.width < winSize.height ? winSize.width : winSize.height;
    float targetSize = smaller * 0.1f;

    auto sprite = CCSprite::createWithSpriteFrameName("player_01_001.png");
    if (!sprite)
        return false;

    float scale = targetSize / sprite->getContentSize().width;
    sprite->setScale(scale);
    sprite->setPosition({winSize.width / 2, winSize.height / 2});
    this->addChild(sprite);

    m_sprite = sprite;
    m_physics = new PhysicsWorld(
        winSize.width, winSize.height,
        targetSize, targetSize
    );

    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, 0, false);
    return true;
}

void PhysicsOverlay::update(float dt) {
    if (!m_physics || !m_sprite)
        return;

    m_physics->step(dt);
    auto state = m_physics->getPlayerState();

    m_sprite->setPosition({state.x, state.y});
    // cocos2d rotation
    m_sprite->setRotation(-state.angle * (180.0f / 3.14159265f));
}

void PhysicsOverlay::onExit() {
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    delete m_physics;
    m_physics = nullptr;
    CCNode::onExit();
}

$on_mod(Loaded) {
    geode::queueInMainThread([] {
        auto overlay = PhysicsOverlay::create();
        if (!overlay)
            return;
        overlay->setZOrder(1000);
        geode::OverlayManager::get()->addChild(overlay);
    });
}
