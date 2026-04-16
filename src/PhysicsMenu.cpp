#include "PhysicsMenu.h"

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCScale9Sprite.h>

using namespace cocos2d;
using namespace geode::prelude;

namespace {
constexpr float kButtonSpacingPx = 8.0f;
constexpr float kButtonScale = 0.55f;
constexpr float kTitleLabelScale = 0.5f;
constexpr float kTitleTopInset = 14.0f;
constexpr float kMenuYFrac = 0.45;
constexpr float kPopupOpacity = 192.0f;
} // namespace

PhysicsMenu::~PhysicsMenu() {
    if (m_root) {
        m_root->removeFromParentAndCleanup(true);
        m_root = nullptr;
    }
}

bool PhysicsMenu::build(float width, float height) {
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
        popupBg->setOpacity(static_cast<GLubyte>(kPopupOpacity));
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
        title->setScale(kTitleLabelScale);
        title->setAnchorPoint({0.5f, 1.0f});
        title->setPosition({hw, height - kTitleTopInset});
        root->addChild(title, 2);
    }

    auto* menu = CCMenu::create();
    if (menu) {
        menu->ignoreAnchorPointForPosition(false);
        menu->setContentSize({width, height});
        menu->setAnchorPoint({0.5f, 0.5f});
        menu->setPosition({hw, hh});

        auto* btnA = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Aa123", "goldFont.fnt", "GJ_button_01.png", kButtonScale),
            [](CCMenuItemSpriteExtra*) {
                log::info("physics menu: button A pressed");
            }
        );
        auto* btnB = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Bb456", "goldFont.fnt", "GJ_button_01.png", kButtonScale),
            [](CCMenuItemSpriteExtra*) {
                log::info("physics menu: button B pressed");
            }
        );

        if (btnA && btnB) {
            float const wA = btnA->getScaledContentSize().width;
            float const wB = btnB->getScaledContentSize().width;
            float const totalW = wA + kButtonSpacingPx + wB;
            float const xStart = (width - totalW) * 0.5f;
            float const y = height * kMenuYFrac;

            btnA->setPosition({xStart + wA * 0.5f, y});
            btnB->setPosition({xStart + wA + kButtonSpacingPx + wB * 0.5f, y});
            menu->addChild(btnA);
            menu->addChild(btnB);
        }

        root->addChild(menu, 3);
    }

    m_root = root;
    return true;
}

bool PhysicsMenu::containsLocalPoint(float localX, float localY) const {
    float const hw = m_width * 0.5f;
    float const hh = m_height * 0.5f;
    return localX >= -hw && localX <= hw && localY >= -hh && localY <= hh;
}
