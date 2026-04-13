#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/Enums.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include "PhysicsWorld.h"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt) {
    if (gm->isIconLoaded(iconId, typeInt))
        return;
    int const requestId = gm->getIconRequestID();
    gm->loadIcon(iconId, typeInt, requestId);
}

CCRect worldBoundsFromNode(CCNode* n) {
    CCRect const bb = n->boundingBox();
    CCNode* parent = n->getParent();
    if (!parent) {
        return bb;
    }
    auto const corner = [&](float x, float y) {
        return parent->convertToWorldSpace(ccp(x, y));
    };
    float const minX = bb.origin.x;
    float const minY = bb.origin.y;
    float const maxX = bb.origin.x + bb.size.width;
    float const maxY = bb.origin.y + bb.size.height;
    CCPoint const c0 = corner(minX, minY);
    CCPoint const c1 = corner(maxX, minY);
    CCPoint const c2 = corner(minX, maxY);
    CCPoint const c3 = corner(maxX, maxY);
    float minWx = std::min({c0.x, c1.x, c2.x, c3.x});
    float maxWx = std::max({c0.x, c1.x, c2.x, c3.x});
    float minWy = std::min({c0.y, c1.y, c2.y, c3.y});
    float maxWy = std::max({c0.y, c1.y, c2.y, c3.y});
    return CCRectMake(minWx, minWy, maxWx - minWx, maxWy - minWy);
}

CCRect unionRects(CCRect const& a, CCRect const& b) {
    float const minX = std::min(a.getMinX(), b.getMinX());
    float const minY = std::min(a.getMinY(), b.getMinY());
    float const maxX = std::max(a.getMaxX(), b.getMaxX());
    float const maxY = std::max(a.getMaxY(), b.getMaxY());
    return CCRectMake(minX, minY, maxX - minX, maxY - minY);
}

CCRect unionWorldBoundsTree(CCNode* n, int depth = 0) {
    if (!n || depth > 64) {
        return CCRectZero;
    }
    CCRect acc = worldBoundsFromNode(n);
    if (CCArray* children = n->getChildren()) {
        for (int i = 0; i < children->count(); ++i) {
            auto* ch = dynamic_cast<CCNode*>(children->objectAtIndex(i));
            if (!ch) {
                continue;
            }
            acc = unionRects(acc, unionWorldBoundsTree(ch, depth + 1));
        }
    }
    return acc;
}

float visualWidthForPlayer(SimplePlayer* player) {
    CCRect const world = unionWorldBoundsTree(player);
    float const ww = std::fabs(world.size.width);
    if (ww > 1.0f) {
        return ww;
    }
    float const cw = player->getContentSize().width;
    return cw > 1.0f ? cw : 1.0f;
}

} // namespace

class PhysicsOverlay : public CCLayer {
    PhysicsWorld* m_physics = nullptr;
    CCNode* m_playerVisual = nullptr;

    int m_frameId = 1;
    int m_iconTypeInt = static_cast<int>(IconType::Cube);
    bool m_visualBuilt = false;
    bool m_grabActive = false;
    float m_targetSize = 0.0f;
    CCSize m_winSize{};

public:
    CREATE_FUNC(PhysicsOverlay);
    bool init() override;
    void update(float dt) override;
    void onExit() override;

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;

private:
    void tryBuildPlayerVisual();
    static void applyGmColorsAndFrame(SimplePlayer* player, int frameId);
    bool tryBeginGrab(CCPoint const& locationInNode);
    void endGrab();
};

void PhysicsOverlay::applyGmColorsAndFrame(SimplePlayer* player, int frameId) {
    auto* gm = GameManager::get();
    if (!gm || !player)
        return;

    player->updatePlayerFrame(frameId, IconType::Cube);
    player->setColors(
        gm->colorForIdx(gm->getPlayerColor()),
        gm->colorForIdx(gm->getPlayerColor2())
    );
    if (gm->getPlayerGlow()) {
        player->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
    } else {
        player->disableGlowOutline();
    }
    player->updateColors();
}

void PhysicsOverlay::tryBuildPlayerVisual() {
    if (m_visualBuilt)
        return;

    auto* gm = GameManager::get();
    if (!gm)
        return;

    if (!gm->isIconLoaded(m_frameId, m_iconTypeInt))
        return;

    auto* player = SimplePlayer::create(m_frameId);
    if (!player)
        return;

    this->addChild(player);

    applyGmColorsAndFrame(player, m_frameId);

    float const w = visualWidthForPlayer(player);
    float const scale = m_targetSize / w;
    player->setScale(scale);
    player->setPosition({m_winSize.width / 2, m_winSize.height / 2});

    m_playerVisual = player;
    m_visualBuilt = true;
}

bool PhysicsOverlay::tryBeginGrab(CCPoint const& locationInNode) {
    if (!m_physics)
        return false;

    auto const state = m_physics->getPlayerState();
    float const dx = locationInNode.x - state.x;
    float const dy = locationInNode.y - state.y;
    float const distSq = dx * dx + dy * dy;
    float const grabRadius = m_targetSize * 0.65f;
    if (distSq > grabRadius * grabRadius)
        return false;

    m_physics->setDragGrabOffsetPixels(dx, dy);
    m_physics->setDragTargetPixels(locationInNode.x, locationInNode.y);
    m_physics->setDragging(true);
    m_grabActive = true;
    return true;
}

void PhysicsOverlay::endGrab() {
    if (!m_grabActive)
        return;
    m_grabActive = false;
    if (m_physics)
        m_physics->setDragging(false);
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (!m_physics)
        return false;
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    return tryBeginGrab(p);
}

void PhysicsOverlay::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_grabActive || !m_physics)
        return;
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    m_physics->setDragTargetPixels(p.x, p.y);
}

void PhysicsOverlay::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endGrab();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)touch;
    (void)event;
    endGrab();
}

bool PhysicsOverlay::init() {
    if (!CCLayer::init())
        return false;

    m_winSize = CCDirector::get()->getWinSize();
    float smaller = m_winSize.width < m_winSize.height ? m_winSize.width : m_winSize.height;
    m_targetSize = smaller * 0.15f;

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

    this->setContentSize(m_winSize);
    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);

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
    endGrab();
    CCDirector::get()->getScheduler()->unscheduleUpdateForTarget(this);
    delete m_physics;
    m_physics = nullptr;
    m_playerVisual = nullptr;
    CCLayer::onExit();
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
