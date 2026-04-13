#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/Enums.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <Geode/cocos/sprite_nodes/CCSpriteFrameCache.h>
#include <cstdio>
#include "PhysicsWorld.h"

using namespace geode::prelude;

namespace {

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt) {
    if (gm->isIconLoaded(iconId, typeInt))
        return;
    int const requestId = gm->getIconRequestID();
    gm->loadIcon(iconId, typeInt, requestId);
}

bool spriteFrameExists(char const* name) {
    return CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(name) != nullptr;
}

} // namespace

class PhysicsOverlay : public CCNode {
    PhysicsWorld* m_physics = nullptr;
    CCNode* m_playerVisual = nullptr;

    int m_frameId = 1;
    int m_iconTypeInt = static_cast<int>(IconType::Cube);
    bool m_visualBuilt = false;
    float m_targetSize = 0.0f;
    CCSize m_winSize{};

public:
    CREATE_FUNC(PhysicsOverlay);
    bool init();
    void update(float dt) override;
    void onExit() override;

private:
    void tryBuildPlayerVisual();
};

static CCNode* createPlayerIconForFrame(int frameId) {
    auto* gm = GameManager::get();
    if (!gm)
        return nullptr;

    if (frameId < 1)
        frameId = 1;

    char mainName[48];
    std::snprintf(mainName, sizeof(mainName), "player_%02d_001.png", frameId);

    CCSprite* main = CCSprite::createWithSpriteFrameName(mainName);
    if (!main)
        main = CCSprite::createWithSpriteFrameName("player_01_001.png");
    if (!main)
        main = CCSprite::create("player_01_001.png");
    if (!main)
        return nullptr;

    main->setColor(gm->colorForIdx(gm->getPlayerColor()));

    auto* root = CCNode::create();
    CCSize const sz = main->getContentSize();
    root->setContentSize(sz);
    root->setAnchorPoint({0.5f, 0.5f});

    CCPoint const center = ccp(sz.width * 0.5f, sz.height * 0.5f);

    if (gm->getPlayerGlow()) {
        char glowName[48];
        std::snprintf(glowName, sizeof(glowName), "player_%02d_glow_001.png", frameId);
        if (spriteFrameExists(glowName)) {
            if (auto* glow = CCSprite::createWithSpriteFrameName(glowName)) {
                glow->setColor(gm->colorForIdx(gm->getPlayerGlowColor()));
                glow->setPosition(center);
                root->addChild(glow, -1);
            }
        }
    }

    main->setPosition(center);
    root->addChild(main, 0);

    char detailName[48];
    std::snprintf(detailName, sizeof(detailName), "player_%02d_002.png", frameId);
    if (spriteFrameExists(detailName)) {
        if (auto* detail = CCSprite::createWithSpriteFrameName(detailName)) {
            detail->setColor(gm->colorForIdx(gm->getPlayerColor2()));
            detail->setPosition(center);
            root->addChild(detail, 1);
        }
    }

    return root;
}

void PhysicsOverlay::tryBuildPlayerVisual() {
    if (m_visualBuilt)
        return;

    auto* gm = GameManager::get();
    if (!gm)
        return;

    if (!gm->isIconLoaded(m_frameId, m_iconTypeInt))
        return;

    auto* visual = createPlayerIconForFrame(m_frameId);
    if (!visual)
        return;

    float const w = visual->getContentSize().width;
    float const scale = m_targetSize / (w > 1.0f ? w : 1.0f);
    visual->setScale(scale);
    visual->setPosition({m_winSize.width / 2, m_winSize.height / 2});

    this->addChild(visual);
    m_playerVisual = visual;
    m_visualBuilt = true;
}

bool PhysicsOverlay::init() {
    if (!CCNode::init())
        return false;

    m_winSize = CCDirector::get()->getWinSize();
    float smaller = m_winSize.width < m_winSize.height ? m_winSize.width : m_winSize.height;
    m_targetSize = smaller * 0.125f;

    auto* gm = GameManager::get();
    if (!gm)
        return false;

    m_frameId = gm->getPlayerFrame();
    if (m_frameId < 1)
        m_frameId = 1;

    requestCubeIconLoad(gm, m_frameId, m_iconTypeInt);

    m_physics = new PhysicsWorld(
        m_winSize.width, m_winSize.height,
        m_targetSize, m_targetSize
    );

    CCDirector::get()->getScheduler()->scheduleUpdateForTarget(this, 0, false);
    return true;
}

void PhysicsOverlay::update(float dt) {
    if (!m_physics)
        return;

    if (!m_visualBuilt)
        tryBuildPlayerVisual();

    m_physics->step(dt);

    if (!m_playerVisual)
        return;

    auto state = m_physics->getPlayerState();
    m_playerVisual->setPosition({state.x, state.y});
    m_playerVisual->setRotation(-state.angle * (180.0f / 3.14159265f));
}

void PhysicsOverlay::onExit() {
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    delete m_physics;
    m_physics = nullptr;
    m_playerVisual = nullptr;
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
