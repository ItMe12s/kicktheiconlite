#include "PhysicsTestPanel.h"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>

using namespace cocos2d;
using namespace geode::prelude;

static constexpr float kTitleBarHeightPx = 24.0f;
static constexpr float kTitleBarInsetPx = 1.0f;
static constexpr float kButtonSpacingPx = 8.0f;
static constexpr float kButtonScale = 0.55f;
static constexpr float kTitleLabelScale = 0.5f;
static constexpr float kTitleLabelTopInset = 4.0f;

static ccColor4F panelBodyFill() { return {0.10f, 0.10f, 0.14f, 0.92f}; }
static ccColor4F panelTitleBarFill() { return {0.25f, 0.25f, 0.35f, 0.95f}; }
static ccColor4F panelBorder() { return {1.0f, 1.0f, 1.0f, 0.35f}; }

PhysicsTestPanel::~PhysicsTestPanel() {
    if (m_root) {
        m_root->removeFromParentAndCleanup(true);
        m_root = nullptr;
    }
}

bool PhysicsTestPanel::build(float width, float height) {
    if (m_root) {
        return false;
    }

    m_width = width;
    m_height = height;
    m_titleBarHeight = kTitleBarHeightPx;

    auto* root = CCNode::create();
    if (!root) {
        return false;
    }
    root->setID("physics-test-panel"_spr);
    root->setContentSize({width, height});
    root->setAnchorPoint({0.5f, 0.5f});
    root->ignoreAnchorPointForPosition(false);

    float const hw = width * 0.5f;
    float const hh = height * 0.5f;

    auto* body = CCDrawNode::create();
    if (body) {
        body->drawRect(
            CCRectMake(-hw, -hh, width, height - kTitleBarHeightPx),
            panelBodyFill(),
            1.0f,
            panelBorder()
        );
        body->drawRect(
            CCRectMake(-hw, hh - kTitleBarHeightPx, width, kTitleBarHeightPx),
            panelTitleBarFill(),
            1.0f,
            panelBorder()
        );
        body->setPosition({hw, hh});
        root->addChild(body, 0);
    }

    auto* title = CCLabelBMFont::create("Test Panel", "bigFont.fnt");
    if (title) {
        title->setScale(kTitleLabelScale);
        title->setAnchorPoint({0.5f, 1.0f});
        title->setPosition({hw, height - kTitleLabelTopInset});
        root->addChild(title, 2);
    }

    auto* menu = CCMenu::create();
    if (menu) {
        menu->setPosition({hw, hh - kTitleBarHeightPx * 0.25f});

        auto* btnA = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("A", "goldFont.fnt", "GJ_button_01.png", kButtonScale),
            [](CCMenuItemSpriteExtra*) {
                log::info("physics panel: button A pressed");
            }
        );
        auto* btnB = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("B", "goldFont.fnt", "GJ_button_01.png", kButtonScale),
            [](CCMenuItemSpriteExtra*) {
                log::info("physics panel: button B pressed");
            }
        );

        if (btnA && btnB) {
            float const wA = btnA->getScaledContentSize().width;
            float const wB = btnB->getScaledContentSize().width;
            float const totalW = wA + kButtonSpacingPx + wB;
            btnA->setPosition({-totalW * 0.5f + wA * 0.5f, 0.0f});
            btnB->setPosition({ totalW * 0.5f - wB * 0.5f, 0.0f});

            menu->addChild(btnA);
            menu->addChild(btnB);
        }

        root->addChild(menu, 3);
    }

    m_root = root;
    return true;
}

bool PhysicsTestPanel::isInsideTitleBar(float localX, float localY) const {
    float const hw = m_width * 0.5f;
    float const hh = m_height * 0.5f;
    float const barBottom = hh - m_titleBarHeight - kTitleBarInsetPx;
    return localX >= -hw && localX <= hw && localY >= barBottom && localY <= hh;
}
