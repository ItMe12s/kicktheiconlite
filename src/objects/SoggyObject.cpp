#include "SoggyObject.h"

#include <Geode/cocos/platform/CCFileUtils.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/sprite_nodes/CCSpriteFrameCache.h>
#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

using namespace geode::prelude;

namespace soggy_object {
namespace {

bool s_atlasLoaded = false;

std::array<std::string, 2> candidatePlists() {
    return {
        std::string("soggy_atlas.plist"_spr),
    };
}

std::array<std::string, 2> candidatePngs() {
    return {
        std::string("soggy_atlas.png"_spr),
    };
}

cocos2d::CCSpriteFrame* spriteFrameForIndex(int frame1Based) {
    int const fc = std::max(1, kSoggyFrameCount);
    if (frame1Based < 1 || frame1Based > fc) {
        return nullptr;
    }
    char raw[32];
    std::snprintf(raw, sizeof(raw), "frame_%03d.png", frame1Based);
    std::string const rawStr(raw);
    auto* cache = cocos2d::CCSpriteFrameCache::sharedSpriteFrameCache();
    if (auto* f = cache->spriteFrameByName(rawStr.c_str())) {
        return f;
    }
    std::string const qualified = Mod::get()->getID() + "/" + rawStr;
    return cache->spriteFrameByName(qualified.c_str());
}

} // namespace

bool tryLoadSoggyAtlas() {
    if (s_atlasLoaded) {
        return true;
    }
    auto* files = cocos2d::CCFileUtils::sharedFileUtils();
    auto plists = candidatePlists();
    auto pngs = candidatePngs();
    for (size_t i = 0; i < plists.size(); ++i) {
        std::string const& plist = plists[i];
        if (plist.empty() || files->fullPathForFilename(plist.c_str(), false).empty()) {
            continue;
        }
        std::string const& png = pngs[i];
        cocos2d::CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(
            plist.c_str(),
            png.c_str()
        );
        if (spriteFrameForIndex(1)) {
            s_atlasLoaded = true;
            return true;
        }
    }
    return false;
}

cocos2d::CCNode* tryCreateRoot(cocos2d::CCSprite** outSprite) {
    if (outSprite) {
        *outSprite = nullptr;
    }
    if (!tryLoadSoggyAtlas()) {
        return nullptr;
    }
    auto* first = spriteFrameForIndex(1);
    if (!first) {
        return nullptr;
    }
    auto* spr = cocos2d::CCSprite::createWithSpriteFrame(first);
    if (!spr) {
        return nullptr;
    }
    spr->setAnchorPoint({0.5f, 0.5f});
    float const ow = std::max(spr->getContentSize().width, 0.1f);
    float const oh = std::max(spr->getContentSize().height, 0.1f);
    float const target = kSoggyHitboxSizePx;
    float const scale = target / std::max(ow, oh);
    spr->setScale(scale);
    float const w = ow * scale;
    float const h = oh * scale;
    auto* root = cocos2d::CCNode::create();
    if (!root) {
        return nullptr;
    }
    root->setContentSize({w, h});
    root->setAnchorPoint({0.5f, 0.5f});
    spr->setPosition({w * 0.5f, h * 0.5f});
    root->addChild(spr, 0);
    if (outSprite) {
        *outSprite = spr;
    }
    return root;
}

void advanceAnimation(cocos2d::CCSprite* sprite, float timeSeconds) {
    if (!sprite) {
        return;
    }
    int const fc = std::max(1, kSoggyFrameCount);
    int const idx = 1 + (static_cast<int>(timeSeconds * kSoggyAnimFps) % fc);
    if (auto* f = spriteFrameForIndex(idx)) {
        sprite->setDisplayFrame(f);
        float const ow = std::max(sprite->getContentSize().width, 0.1f);
        float const oh = std::max(sprite->getContentSize().height, 0.1f);
        float const target = kSoggyHitboxSizePx;
        float const scale = target / std::max(ow, oh);
        sprite->setScale(scale);
        cocos2d::CCNode* parent = sprite->getParent();
        if (parent) {
            float const w = ow * scale;
            float const h = oh * scale;
            parent->setContentSize({w, h});
            sprite->setPosition({w * 0.5f, h * 0.5f});
        }
    }
}

float visualSpanPx(cocos2d::CCNode* root) {
    if (!root) {
        return kSoggyHitboxSizePx;
    }
    auto const s = root->getContentSize();
    return std::max(s.width, s.height);
}

float hitboxSizePx() {
    return kSoggyHitboxSizePx;
}

} // namespace soggy_object
