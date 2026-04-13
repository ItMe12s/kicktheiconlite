#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>
#include "PhysicsWorld.h"

using namespace geode::prelude;

class $modify(MyScene, CCScene) {
    struct Fields {
        PhysicsWorld* m_physics = nullptr;
        CCSprite* m_sprite = nullptr;
    };

    bool init() {
        if (!CCScene::init())
            return false;
        if (typeinfo_cast<CCTransitionScene*>(this))
            return true;

        auto winSize = CCDirector::get()->getWinSize();
        float smaller = winSize.width < winSize.height ? winSize.width : winSize.height;
        float targetSize = smaller * 0.1f;

        auto sprite = CCSprite::createWithSpriteFrameName("player_01_001.png");
        if (!sprite)
            return true;

        float scale = targetSize / sprite->getContentSize().width;
        sprite->setScale(scale);
        sprite->setPosition({winSize.width / 2, winSize.height / 2});
        sprite->setZOrder(1000);
        this->addChild(sprite);

        m_fields->m_sprite = sprite;
        m_fields->m_physics = new PhysicsWorld(
            winSize.width, winSize.height,
            targetSize, targetSize
        );

        this->scheduleUpdate();
        return true;
    }

    void update(float dt) {
        CCScene::update(dt);
        if (!m_fields->m_physics || !m_fields->m_sprite)
            return;

        m_fields->m_physics->step(dt);
        auto state = m_fields->m_physics->getPlayerState();

        m_fields->m_sprite->setPosition({state.x, state.y});
        // Cocos2d rotation: degrees, clockwise-positive — negate physics angle
        m_fields->m_sprite->setRotation(-state.angle * (180.0f / 3.14159265f));
    }
};
