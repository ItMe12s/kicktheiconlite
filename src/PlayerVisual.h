#pragma once

#include "ModTuning.h"

#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/SimplePlayer.hpp>

namespace player_visual {

// SimplePlayer tree setup and trail ghosts: typeinfo_cast walk for blend, ghost ids use GEODE_MOD_ID + serial.

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt);

struct PlayerRootResult {
    bool ok = false;
    cocos2d::CCNode* root = nullptr;
    SimplePlayer* player = nullptr;
};

PlayerRootResult tryBuildPlayerRoot(
    cocos2d::CCLayer* overlay,
    cocos2d::CCSize const& winSize,
    float targetSize,
    int frameId,
    int iconTypeInt
);

bool spawnFadingGhost(
    cocos2d::CCNode* parent,
    cocos2d::CCPoint const& position,
    float rotationDeg,
    float targetSize,
    int frameId,
    int iconTypeInt,
    float fadeSec,
    unsigned char startOpacity
);

} // namespace player_visual
