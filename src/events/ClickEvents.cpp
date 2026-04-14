#include "ClickEvents.h"

#include <Geode/modify/CCTouchDispatcher.hpp>

#include <chrono>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

namespace {

constexpr double kClickWindowSec = 0.5;

double nowSec() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

struct SpriteState {
    int    clickCount  = 0;
    double lastEndTime = 0.0;
};

std::unordered_set<cocos2d::CCSprite*>             g_tracked;
std::unordered_map<cocos2d::CCSprite*, SpriteState> g_state;
std::unordered_map<int, cocos2d::CCSprite*>         g_pending;

bool isSpriteOnStageVisible(cocos2d::CCSprite* sprite) {
    if (!sprite || !sprite->getParent()) return false;
    for (cocos2d::CCNode* n = sprite; n; n = n->getParent()) {
        if (!n->isVisible()) return false;
    }
    return true;
}

bool hitTest(cocos2d::CCSprite* sprite, cocos2d::CCTouch* touch) {
    if (!isSpriteOnStageVisible(sprite)) return false;
    auto const world = touch->getLocation();
    auto const local = sprite->convertToNodeSpace(world);
    auto const size  = sprite->getContentSize();
    return cocos2d::CCRect(0.f, 0.f, size.width, size.height).containsPoint(local);
}

cocos2d::CCSprite* findHitSprite(cocos2d::CCTouch* touch) {
    // If multiple tracked sprites overlap, the last one in iteration order wins
    cocos2d::CCSprite* hit = nullptr;
    for (auto* s : g_tracked) {
        if (hitTest(s, touch)) hit = s;
    }
    return hit;
}

}

namespace events {

ClickTracker* ClickTracker::get() {
    static ClickTracker inst;
    return &inst;
}

void ClickTracker::track(cocos2d::CCSprite* sprite) {
    if (!sprite) return;
    g_tracked.insert(sprite);
}

void ClickTracker::untrack(cocos2d::CCSprite* sprite) {
    if (!sprite) return;
    g_tracked.erase(sprite);
    g_state.erase(sprite);
    for (auto it = g_pending.begin(); it != g_pending.end();) {
        if (it->second == sprite) it = g_pending.erase(it);
        else ++it;
    }
}

void ClickTracker::onTouchBegan(cocos2d::CCTouch* touch) {
    if (!touch) return;
    auto* hit = findHitSprite(touch);
    if (hit) g_pending[touch->getID()] = hit;
}

void ClickTracker::onTouchEnded(cocos2d::CCTouch* touch) {
    if (!touch) return;
    auto it = g_pending.find(touch->getID());
    if (it == g_pending.end()) return;
    auto* began = it->second;
    g_pending.erase(it);

    if (!g_tracked.contains(began)) return;
    if (!hitTest(began, touch)) return;

    auto&        st = g_state[began];
    double const t  = nowSec();
    if (st.clickCount > 0 && (t - st.lastEndTime) <= kClickWindowSec) {
        st.clickCount += 1;
    } else {
        st.clickCount = 1;
    }
    st.lastEndTime = t;

    if (st.clickCount == 2) {
        DoubleClickEvent().send(began, touch);
    } else if (st.clickCount >= 3) {
        TripleClickEvent().send(began, touch);
        st.clickCount = 0;
    }
}

void ClickTracker::onTouchCancelled(cocos2d::CCTouch* touch) {
    if (!touch) return;
    g_pending.erase(touch->getID());
}

}

class $modify(ClickTouchDispatcherHook, cocos2d::CCTouchDispatcher) {
    void touches(cocos2d::CCSet* pTouches, cocos2d::CCEvent* pEvent, unsigned int uIndex) {
        cocos2d::CCTouchDispatcher::touches(pTouches, pEvent, uIndex);
        if (!pTouches) return;

        auto* tracker = events::ClickTracker::get();
        for (auto it = pTouches->begin(); it != pTouches->end(); ++it) {
            auto* touch = static_cast<cocos2d::CCTouch*>(*it);
            if (!touch) continue;
            switch (uIndex) {
                case 0: tracker->onTouchBegan(touch); break;
                case 2: tracker->onTouchEnded(touch); break;
                case 3: tracker->onTouchCancelled(touch); break;
                default: break;
            }
        }
    }
};
