#pragma once

#include <Geode/loader/Event.hpp>
#include <cocos2d.h>

namespace events {

struct DoubleClickMarker {};
struct TripleClickMarker {};

using DoubleClickEvent = geode::Event<DoubleClickMarker, void(cocos2d::CCSprite*, cocos2d::CCTouch*)>;
using TripleClickEvent = geode::Event<TripleClickMarker, void(cocos2d::CCSprite*, cocos2d::CCTouch*)>;

// Caller must untrack before sprite is destroyed (e.g. onExit) if you don't then we all die
class ClickTracker {
public:
    static ClickTracker *get();

    void track(cocos2d::CCSprite *sprite);
    void untrack(cocos2d::CCSprite *sprite);

    void onTouchBegan(cocos2d::CCTouch *touch);
    void onTouchEnded(cocos2d::CCTouch *touch);
    void onTouchCancelled(cocos2d::CCTouch *touch);

private:
    ClickTracker() = default;
};

} // namespace events
