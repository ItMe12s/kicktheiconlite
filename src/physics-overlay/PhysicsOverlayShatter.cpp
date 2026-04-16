#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "vfx/ObjectMotionBlurPipeline.h"

using namespace geode::prelude;

namespace {

inline float clamp01(float value) {
    return std::max(0.0f, std::min(value, 1.0f));
}

cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> const& roots,
    overlay_rendering::OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
}

} // namespace

void PhysicsOverlay::destroyPhysicsMenuVisual() {
    if (m_physics) {
        m_physics->destroyPanel();
    }
    auto& menuCapture =
        m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::PhysicsMenu)];
    menuCapture.enabled = false;
    menuCapture.sourceRoot = nullptr;
    menuCapture.velocity = {};
    m_physicsMenuVisual.reset();
    m_panelDragActive = false;
}

bool PhysicsOverlay::beginMenuShatter(float impactSpeedPx) {
    if (!m_physics || !m_physicsMenuVisual || !m_physics->hasPanel()) {
        return false;
    }
    auto* root = m_physicsMenuVisual->getRoot();
    if (!root) {
        destroyPhysicsMenuVisual();
        return false;
    }

    clearMenuShatter();

    CCSize panelSize = root->getContentSize();
    if (panelSize.width <= 1.0f || panelSize.height <= 1.0f) {
        CCRect const fallbackBounds = root->boundingBox();
        panelSize = fallbackBounds.size;
    }
    int const texW = std::max(2, static_cast<int>(std::round(panelSize.width)));
    int const texH = std::max(2, static_cast<int>(std::round(panelSize.height)));
    auto* snapshot = CCRenderTexture::create(texW, texH);
    if (!snapshot) {
        destroyPhysicsMenuVisual();
        return false;
    }
    snapshot->retain();
    auto const prevPos = root->getPosition();
    auto const prevRot = root->getRotation();
    auto const prevAnchor = root->getAnchorPoint();
    auto const prevScaleX = root->getScaleX();
    auto const prevScaleY = root->getScaleY();
    bool const prevVisible = root->isVisible();
    root->setPosition({static_cast<float>(texW) * 0.5f, static_cast<float>(texH) * 0.5f});
    root->setRotation(0.0f);
    root->setAnchorPoint({0.5f, 0.5f});
    root->setScaleX(1.0f);
    root->setScaleY(1.0f);
    root->setVisible(true);
    snapshot->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    root->visit();
    snapshot->end();
    root->setPosition(prevPos);
    root->setRotation(prevRot);
    root->setAnchorPoint(prevAnchor);
    root->setScaleX(prevScaleX);
    root->setScaleY(prevScaleY);
    root->setVisible(prevVisible);

    auto* textureSprite = snapshot->getSprite();
    auto* texture = textureSprite ? textureSprite->getTexture() : nullptr;
    if (!texture) {
        snapshot->release();
        destroyPhysicsMenuVisual();
        return false;
    }

    PhysicsState const panelState = m_physics->getPanelState();
    float const panelWidth = panelSize.width;
    float const panelHeight = panelSize.height;
    float const cellW = panelWidth / static_cast<float>(kMenuShardCols);
    float const cellH = panelHeight / static_cast<float>(kMenuShardRows);
    auto* uiLayerRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Ui);
    auto* shardRoot = m_menuShatter.root ? m_menuShatter.root : (uiLayerRoot ? uiLayerRoot : this);
    float const impactBias = std::max(0.0f, impactSpeedPx - kPanelShatterMinImpactSpeed);

    m_menuShatter.active = true;
    m_menuShatter.elapsed = 0.0f;
    m_menuShatter.captureRoot = shardRoot;
    m_menuShatter.snapshot = snapshot;
    m_menuShatter.pieces.clear();
    m_menuShatter.pieces.reserve(static_cast<size_t>(kMenuShardRows * kMenuShardCols));

    for (int row = 0; row < kMenuShardRows; ++row) {
        for (int col = 0; col < kMenuShardCols; ++col) {
            float const x = static_cast<float>(col) * cellW;
            float const y = static_cast<float>(row) * cellH;
            float const centerLocalX = -panelWidth * 0.5f + x + cellW * 0.5f;
            float const centerLocalY = panelHeight * 0.5f - y - cellH * 0.5f;

            float const c = std::cos(panelState.angle);
            float const s = std::sin(panelState.angle);
            float const worldX = panelState.x + (centerLocalX * c - centerLocalY * s);
            float const worldY = panelState.y + (centerLocalX * s + centerLocalY * c);

            float const normCol = (static_cast<float>(col) + 0.5f) / static_cast<float>(kMenuShardCols);
            float const normRow = (static_cast<float>(row) + 0.5f) / static_cast<float>(kMenuShardRows);
            float const spreadX = (normCol - 0.5f) * 2.0f;
            float const spreadY = (0.5f - normRow) * 2.0f;
            float const spreadMag = std::max(0.2f, std::hypot(spreadX, spreadY));
            float const dirX = spreadX / spreadMag;
            float const dirY = spreadY / spreadMag;
            float const launchT = clamp01((spreadMag - 0.2f) / 1.2f);
            float const launchSpeed =
                kMenuShardLaunchSpeedMinPx
                + (kMenuShardLaunchSpeedMaxPx - kMenuShardLaunchSpeedMinPx) * launchT
                + impactBias * kMenuShardExtraImpactVelocityScale;
            float const launchVx = dirX * launchSpeed;
            float const launchVy = dirY * launchSpeed;
            float const angularSign = ((row + col) % 2 == 0) ? 1.0f : -1.0f;
            float const angularT = clamp01((normCol + normRow) * 0.5f);
            float const angularVel = angularSign
                * (kMenuShardAngularVelocityMin
                    + (kMenuShardAngularVelocityMax - kMenuShardAngularVelocityMin) * angularT);

            auto* sprite = CCSprite::createWithTexture(
                texture,
                CCRectMake(x, y, cellW, cellH)
            );
            if (!sprite) {
                continue;
            }
            int const bodyHandle = m_physics->spawnShatterBody({
                .widthPx = cellW,
                .heightPx = cellH,
                .xPx = worldX,
                .yPx = worldY,
                .angleRad = panelState.angle,
                .velocityXPx = launchVx,
                .velocityYPx = launchVy,
                .angularVelocityRad = angularVel,
            });
            sprite->setFlipY(true);
            sprite->setOpacity(255);
            sprite->setPosition({worldX, worldY});
            sprite->setRotation(-panelState.angle * kRadToDeg);
            shardRoot->addChild(sprite, kPhysicsMenuZOrder);
            m_menuShatter.pieces.push_back({bodyHandle, sprite});
        }
    }

    destroyPhysicsMenuVisual();
    if (m_menuShatter.pieces.empty()) {
        clearMenuShatter();
        return false;
    }
    return true;
}

void PhysicsOverlay::updateMenuShatter(float dt) {
    if (!m_menuShatter.active || !m_physics) {
        return;
    }
    m_menuShatter.elapsed += dt;
    float const fadeStart = kMenuShardHoldSeconds;
    float const fadeEnd = kMenuShardHoldSeconds + kMenuShardFadeSeconds;
    float alpha = 1.0f;
    if (m_menuShatter.elapsed > fadeStart) {
        alpha = 1.0f - clamp01((m_menuShatter.elapsed - fadeStart) / kMenuShardFadeSeconds);
    }
    GLubyte const opacity = static_cast<GLubyte>(std::round(clamp01(alpha) * 255.0f));

    for (auto& piece : m_menuShatter.pieces) {
        if (!piece.sprite) {
            continue;
        }
        PhysicsState state{};
        if (m_physics->getShatterBodyState(piece.bodyHandle, state)) {
            piece.sprite->setPosition({state.x, state.y});
            piece.sprite->setRotation(-state.angle * kRadToDeg);
        }
        piece.sprite->setOpacity(opacity);
    }

    if (m_menuShatter.elapsed > fadeEnd) {
        clearMenuShatter();
    }
}

void PhysicsOverlay::clearMenuShatter() {
    if (m_menuShatter.snapshot) {
        m_menuShatter.snapshot->release();
        m_menuShatter.snapshot = nullptr;
    }
    for (auto& piece : m_menuShatter.pieces) {
        if (piece.sprite) {
            piece.sprite->removeFromParentAndCleanup(true);
            piece.sprite = nullptr;
        }
    }
    m_menuShatter.pieces.clear();
    m_menuShatter.captureRoot = nullptr;
    if (m_physics) {
        m_physics->clearShatterBodies();
    }
    m_menuShatter.elapsed = 0.0f;
    m_menuShatter.active = false;
}
