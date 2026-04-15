#include "ClickEvents.h"

#include <Geode/cocos/actions/CCActionInstant.h>
#include <Geode/cocos/actions/CCActionInterval.h>
#include <Geode/cocos/cocoa/CCObject.h>

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

namespace {

constexpr double kClickWindowSec = 0.5;
// Minimum delay before scheduling a double-commit, actual schedule uses kDoubleClickScheduledDelaySec
constexpr double kDoubleTapMinCommitDelaySec = 0.15;
// Scheduled delay before DoubleClickEvent fires, must be >= kClickWindowSec so a third tap can still land in the chain window
constexpr double kDoubleClickScheduledDelaySec =
    std::max(kDoubleTapMinCommitDelaySec, kClickWindowSec);

constexpr int kPendingDoubleActionTag = 0x434C4B44; // Means CLKD, deferred double click

// Began vs Ended
constexpr float kTapSlopPx = 12.0f;
// When the icon moves, the finger often moves with it
constexpr float kTrackResidualSlopPx = 20.0f;
// Residual path is only for short gestures, long drags exceed this even if residual is small
constexpr float kMaxFingerForTrackTapPx = 48.0f;
constexpr double kMaxTapGestureSec = 0.24;

double nowSec() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

struct SpriteState {
    int clickCount = 0;
    double lastEndTime = 0.0;
    bool pendingDoubleArmed = false;
};

struct Pending {
    cocos2d::CCSprite* sprite;
    cocos2d::CCPoint startWorld;
    cocos2d::CCPoint spriteCenterWorldStart;
    double beganSec = 0.0;
};

struct ClickTrackerState {
    std::unordered_set<cocos2d::CCSprite*> tracked;
    std::unordered_map<cocos2d::CCSprite*, SpriteState> state;
    std::unordered_map<int, Pending> pending;
    std::unordered_map<cocos2d::CCSprite*, float> hitRadius;
};

static ClickTrackerState s_clickState;

cocos2d::CCPoint spriteWorldCenter(cocos2d::CCSprite* sprite) {
    if (!sprite) {
        return {0.f, 0.f};
    }
    auto* parent = sprite->getParent();
    if (!parent) {
        return sprite->getPosition();
    }
    return parent->convertToWorldSpace(sprite->getPosition());
}

bool isSpriteOnStageVisible(cocos2d::CCSprite* sprite) {
    if (!sprite || !sprite->getParent()) {
        return false;
    }
    for (cocos2d::CCNode* n = sprite; n; n = n->getParent()) {
        if (!n->isVisible()) {
            return false;
        }
    }
    return true;
}

bool hitTest(cocos2d::CCSprite* sprite, cocos2d::CCTouch* touch) {
    if (!isSpriteOnStageVisible(sprite)) {
        return false;
    }
    auto const world = touch->getLocation();
    if (auto it = s_clickState.hitRadius.find(sprite); it != s_clickState.hitRadius.end()) {
        auto* parent = sprite->getParent();
        if (!parent) {
            return false;
        }
        auto const center = parent->convertToWorldSpace(sprite->getPosition());
        float const dx = world.x - center.x;
        float const dy = world.y - center.y;
        float const r = it->second;
        return dx * dx + dy * dy <= r * r;
    }
    auto const local = sprite->convertToNodeSpace(world);
    auto const size = sprite->getContentSize();
    return cocos2d::CCRect(0.f, 0.f, size.width, size.height).containsPoint(local);
}

cocos2d::CCSprite* findHitSprite(cocos2d::CCTouch* touch) {
    // If multiple tracked sprites overlap, the last one in iteration order wins
    cocos2d::CCSprite* hit = nullptr;
    for (auto* s : s_clickState.tracked) {
        if (hitTest(s, touch)) {
            hit = s;
        }
    }
    return hit;
}

void cancelPendingDoubleForSprite(cocos2d::CCSprite* sprite) {
    if (!sprite) {
        return;
    }
    sprite->stopActionByTag(kPendingDoubleActionTag);
    if (auto it = s_clickState.state.find(sprite); it != s_clickState.state.end()) {
        it->second.pendingDoubleArmed = false;
    }
}

void onDeferredDoubleConfirm(cocos2d::CCSprite* sprite) {
    if (!sprite || !s_clickState.tracked.contains(sprite)) {
        return;
    }
    auto it = s_clickState.state.find(sprite);
    if (it == s_clickState.state.end()) {
        return;
    }
    auto& st = it->second;
    if (!st.pendingDoubleArmed || st.clickCount != 2) {
        return;
    }
    st.pendingDoubleArmed = false;
    st.clickCount = 0;
    events::DoubleClickEvent().send(sprite, nullptr);
}

// CCCallFunc has no lambda create
class DeferredDoubleDispatch : public cocos2d::CCObject {
public:
    cocos2d::CCSprite* m_sprite = nullptr;

    static DeferredDoubleDispatch* create(cocos2d::CCSprite* sprite) {
        auto* d = new DeferredDoubleDispatch();
        d->m_sprite = sprite;
        d->autorelease();
        return d;
    }

    void fire(cocos2d::CCNode* node) {
        (void)node;
        onDeferredDoubleConfirm(m_sprite);
    }
};

void armPendingDouble(cocos2d::CCSprite* sprite) {
    if (!sprite) {
        return;
    }
    cancelPendingDoubleForSprite(sprite);
    auto it = s_clickState.state.find(sprite);
    if (it == s_clickState.state.end()) {
        return;
    }
    auto& st = it->second;
    st.pendingDoubleArmed = true;
    float const delay = static_cast<float>(kDoubleClickScheduledDelaySec);
    auto* dispatch = DeferredDoubleDispatch::create(sprite);
    auto* call =
        cocos2d::CCCallFuncN::create(dispatch, callfuncN_selector(DeferredDoubleDispatch::fire));
    auto* seq = cocos2d::CCSequence::create(
        cocos2d::CCDelayTime::create(delay),
        call,
        nullptr
    );
    seq->setTag(kPendingDoubleActionTag);
    sprite->runAction(seq);
}

} // namespace

namespace events {

ClickTracker* ClickTracker::get() {
    static ClickTracker inst;
    return &inst;
}

void ClickTracker::track(cocos2d::CCSprite* sprite, float hitRadiusPx) {
    if (!sprite) {
        return;
    }
    s_clickState.tracked.insert(sprite);
    if (hitRadiusPx > 0.0f) {
        s_clickState.hitRadius[sprite] = hitRadiusPx;
    } else {
        s_clickState.hitRadius.erase(sprite);
    }
}

void ClickTracker::untrack(cocos2d::CCSprite* sprite) {
    if (!sprite) {
        return;
    }
    cancelPendingDoubleForSprite(sprite);
    s_clickState.tracked.erase(sprite);
    s_clickState.state.erase(sprite);
    s_clickState.hitRadius.erase(sprite);
    for (auto it = s_clickState.pending.begin(); it != s_clickState.pending.end();) {
        if (it->second.sprite == sprite) {
            it = s_clickState.pending.erase(it);
        } else {
            ++it;
        }
    }
}

bool ClickTracker::onTouchBegan(cocos2d::CCTouch* touch) {
    if (!touch) {
        return false;
    }
    auto* hit = findHitSprite(touch);
    if (hit) {
        if (auto sit = s_clickState.state.find(hit);
            sit != s_clickState.state.end() && sit->second.pendingDoubleArmed) {
            cancelPendingDoubleForSprite(hit);
        }
        s_clickState.pending[touch->getID()] = {
            hit,
            touch->getLocation(),
            spriteWorldCenter(hit),
            nowSec(),
        };
        return true;
    }
    return false;
}

void ClickTracker::onTouchEnded(cocos2d::CCTouch* touch) {
    if (!touch) {
        return;
    }
    auto it = s_clickState.pending.find(touch->getID());
    if (it == s_clickState.pending.end()) {
        return;
    }
    Pending const p = it->second;
    s_clickState.pending.erase(it);

    if (!s_clickState.tracked.contains(p.sprite)) {
        return;
    }

    auto const end = touch->getLocation();
    float const fdx = end.x - p.startWorld.x;
    float const fdy = end.y - p.startWorld.y;
    float const fingerSq = fdx * fdx + fdy * fdy;
    float const tapSlopSq = kTapSlopPx * kTapSlopPx;

    if (fingerSq > tapSlopSq) {
        // Finger moved in screen space, allow if it mostly matched the sprite
        // (tracking a moving body) and the gesture was short, so it is not a drag
        auto const c1 = spriteWorldCenter(p.sprite);
        float const sdx = c1.x - p.spriteCenterWorldStart.x;
        float const sdy = c1.y - p.spriteCenterWorldStart.y;
        float const rdx = fdx - sdx;
        float const rdy = fdy - sdy;
        float const residualSq = rdx * rdx + rdy * rdy;
        double const duration = nowSec() - p.beganSec;
        float const trackSlopSq = kTrackResidualSlopPx * kTrackResidualSlopPx;
        float const maxFingerSq = kMaxFingerForTrackTapPx * kMaxFingerForTrackTapPx;
        if (duration > kMaxTapGestureSec) {
            return;
        }
        if (residualSq > trackSlopSq) {
            return;
        }
        if (fingerSq > maxFingerSq) {
            return;
        }
    }

    auto& st = s_clickState.state[p.sprite];
    double const t = nowSec();
    if (st.clickCount > 0 && (t - st.lastEndTime) <= kClickWindowSec) {
        st.clickCount += 1;
    } else {
        st.clickCount = 1;
    }
    st.lastEndTime = t;

    if (st.clickCount == 2) {
        armPendingDouble(p.sprite);
    } else if (st.clickCount >= 3) {
        cancelPendingDoubleForSprite(p.sprite);
        TripleClickEvent().send(p.sprite, touch);
        st.clickCount = 0;
    }
}

void ClickTracker::onTouchCancelled(cocos2d::CCTouch* touch) {
    if (!touch) {
        return;
    }
    s_clickState.pending.erase(touch->getID());
}

} // namespace events
