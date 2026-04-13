#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/SimplePlayer.hpp>

namespace player_visual {

constexpr int kMaxWorldBoundsTreeDepth = 64;
constexpr float kMinVisualWidthPx = 1.0f;
constexpr float kPlayerTargetSizeFraction = 0.15f;
constexpr int kMinPlayerFrameId = 1;

constexpr float kPlayerRootAnchorXFrac = 0.5f;
constexpr float kPlayerRootAnchorYFrac = 0.5f;
constexpr int kPlayerVisualLocalZOrder = 0;

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt);

cocos2d::CCRect worldBoundsFromNode(cocos2d::CCNode* n);
cocos2d::CCRect unionRects(cocos2d::CCRect const& a, cocos2d::CCRect const& b);
cocos2d::CCRect unionWorldBoundsTree(cocos2d::CCNode* n, int depth = 0);

float visualWidthForPlayer(SimplePlayer* player);

void applyGmColorsAndFrame(SimplePlayer* player, int frameId);

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

} // namespace player_visual
