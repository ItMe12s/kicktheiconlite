#include "PhysicsMenu.h"

#include "ModTuning.h"

#include <Geode/Geode.hpp>
#include <functional>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCScale9Sprite.h>

using namespace cocos2d;
using namespace geode::prelude;

PhysicsMenu::~PhysicsMenu() {
    if (m_root) {
        m_root->removeFromParentAndCleanup(true);
        m_root = nullptr;
    }
}

bool PhysicsMenu::build(float width, float height, std::function<void()>&& onSoggyPressed) {
    if (m_root) {
        return false;
    }

    m_width = width;
    m_height = height;

    auto* root = CCNode::create();
    if (!root) {
        return false;
    }
    root->setID("physics-menu"_spr);
    root->setContentSize({width, height});
    root->setAnchorPoint({0.5f, 0.5f});
    root->ignoreAnchorPointForPosition(false);

    float const hw = width * 0.5f;
    float const hh = height * 0.5f;

    auto* popupBg = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    if (popupBg) {
        popupBg->setContentSize({width, height});
        popupBg->setAnchorPoint({0.5f, 0.5f});
        popupBg->setPosition({hw, hh});
        popupBg->setOpacity(static_cast<GLubyte>(kPhysicsMenuPopupOpacity));
        root->addChild(popupBg, 0);
    }

    // auto* popupFrame = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    // if (popupFrame) {
    //     popupFrame->setContentSize({width - 8.0f, height - 8.0f}); // 4px inset each side
    //     popupFrame->setAnchorPoint({0.5f, 0.5f});
    //     popupFrame->setPosition({hw, hh});
    //     root->addChild(popupFrame, 1);
    // }

    auto* title = CCLabelBMFont::create("Physics Menu", "goldFont.fnt");
    if (title) {
        title->setScale(kPhysicsMenuTitleLabelScale);
        title->setAnchorPoint({0.5f, 1.0f});
        title->setPosition({hw, height - kPhysicsMenuTitleTopInset});
        root->addChild(title, 2);
    }

    auto* menu = CCMenu::create();
    if (!menu) {
        root->removeFromParentAndCleanup(true);
        return false;
    }
    menu->ignoreAnchorPointForPosition(false);
    menu->setContentSize({width, height});
    menu->setAnchorPoint({0.5f, 0.5f});
    menu->setPosition({hw, hh});

    auto* btnSoggy = CCMenuItemExt::createSpriteExtra(
        ButtonSprite::create("Soggy", "goldFont.fnt", "GJ_button_01.png", kPhysicsMenuButtonScale),
        [soggy = std::move(onSoggyPressed)](CCMenuItemSpriteExtra*) mutable {
            if (soggy) {
                soggy();
            }
        }
    );

    if (btnSoggy) {
        float const y = height * kPhysicsMenuMenuYFrac;
        btnSoggy->setPosition({width * 0.5f, y});
        menu->addChild(btnSoggy);
    }

    root->addChild(menu, 3);

    m_root = root;
    return true;
}

bool PhysicsMenu::containsLocalPoint(float localX, float localY) const {
    float const hw = m_width * 0.5f;
    float const hh = m_height * 0.5f;
    return localX >= -hw && localX <= hw && localY >= -hh && localY <= hh;
}
