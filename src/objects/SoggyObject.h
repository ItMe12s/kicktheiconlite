#pragma once

#include "ModTuning.h"

namespace cocos2d {
class CCNode;
class CCSprite;
}

namespace soggy_object {

bool tryLoadSoggyAtlas();

cocos2d::CCNode* tryCreateRoot(cocos2d::CCSprite** outSprite);

void advanceAnimation(cocos2d::CCSprite* sprite, float timeSeconds);

// Max side of fitted sprite in px (same as kSoggyHitboxSizePx; for grab radius)
float visualSpanPx(cocos2d::CCNode* root);
float hitboxSizePx();

} // namespace soggy_object
