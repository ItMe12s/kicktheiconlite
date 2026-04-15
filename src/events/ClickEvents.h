#pragma once

#include <Geode/loader/Event.hpp>

namespace cocos2d {
class CCSprite;
class CCTouch;
}

namespace events {

struct DoubleClickMarker {};
struct TripleClickMarker {};

using DoubleClickEvent = geode::Event<DoubleClickMarker, bool(cocos2d::CCSprite*, cocos2d::CCTouch*)>;
using TripleClickEvent = geode::Event<TripleClickMarker, bool(cocos2d::CCSprite*, cocos2d::CCTouch*)>;

// Call untrack() before the sprite is destroyed, because the tracker will hold a dangling pointer
class ClickTracker {
public:
    static ClickTracker* get();

    void track(cocos2d::CCSprite* sprite, float hitRadiusPx = 0.0f);
    void untrack(cocos2d::CCSprite* sprite);

    bool onTouchBegan(cocos2d::CCTouch* touch);
    void onTouchEnded(cocos2d::CCTouch* touch);
    void onTouchCancelled(cocos2d::CCTouch* touch);

private:
    ClickTracker() = default;
};

} // namespace events
