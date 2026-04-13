#include "PlayerVisual.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>

#include <algorithm>

using namespace geode::prelude;

namespace player_visual {

void requestCubeIconLoad(GameManager* gm, int iconId, int typeInt) {
    if (!gm->isIconLoaded(iconId, typeInt)) {
        int const requestId = gm->getIconRequestID();
        gm->loadIcon(iconId, typeInt, requestId);
    }
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

CCRect unionWorldBoundsTree(CCNode* n, int depth) {
    if (!n || depth > kMaxWorldBoundsTreeDepth) {
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
    if (ww > kMinVisualWidthPx) {
        return ww;
    }
    float const cw = player->getContentSize().width;
    return cw > kMinVisualWidthPx ? cw : kMinVisualWidthPx;
}

void applyGmColorsAndFrame(SimplePlayer* player, int frameId) {
    auto* gm = GameManager::get();
    if (!gm || !player) {
        return;
    }

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

PlayerRootResult tryBuildPlayerRoot(
    CCLayer* overlay,
    CCSize const& winSize,
    float targetSize,
    int frameId,
    int iconTypeInt
) {
    PlayerRootResult out{};
    auto* gm = GameManager::get();
    if (!gm) {
        return out;
    }

    if (!gm->isIconLoaded(frameId, iconTypeInt)) {
        return out;
    }

    auto* player = SimplePlayer::create(frameId);
    if (!player) {
        return out;
    }

    auto* root = CCNode::create();
    root->setPosition({winSize.width / 2, winSize.height / 2});
    root->addChild(player, 0);
    overlay->addChild(root);

    applyGmColorsAndFrame(player, frameId);

    float const w = visualWidthForPlayer(player);
    float const scale = targetSize / w;
    player->setScale(scale);
    player->setPosition({0, 0});

    out.ok = true;
    out.root = root;
    out.player = player;
    return out;
}

} // namespace player_visual
