#include "PhysicsOverlay.h"

#include "MenuShatterTriangleNode.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/textures/CCTexture2D.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "vfx/ObjectMotionBlurPipeline.h"

using namespace geode::prelude;

namespace {

inline float clamp01(float value) {
    return std::max(0.0f, std::min(value, 1.0f));
}

void ensureCCWPanel(float lx[3], float ly[3]) {
    float const sa = (lx[1] - lx[0]) * (ly[2] - ly[0]) - (ly[1] - ly[0]) * (lx[2] - lx[0]);
    if (sa < 0.0f) {
        std::swap(lx[1], lx[2]);
        std::swap(ly[1], ly[2]);
    }
}

/// UVs are normalized to captured RT texture dimensions
cocos2d::ccTex2F uvForSnapshotCorner(
    float lx,
    float ly,
    float panelW,
    float panelH,
    float texW,
    float texH
) {
    float const u = (lx + panelW * 0.5f) / texW;
    float const v = (ly + panelH * 0.5f) / texH;
    return {u, v};
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
    root->setPosition({0.0f, 0.0f});
    root->setRotation(0.0f);
    root->setAnchorPoint({0.0f, 0.0f});
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

    cocos2d::CCTexture2D* shatterTex = nullptr;
    if (auto* rtSprite = snapshot->getSprite()) {
        shatterTex = rtSprite->getTexture();
    }
    if (!shatterTex) {
        snapshot->release();
        snapshot = nullptr;
        destroyPhysicsMenuVisual();
        return false;
    }

    float const shatterBaseAngleRad = -prevRot * (static_cast<float>(M_PI) / 180.0f);
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
    m_menuShatter.pieces.reserve(static_cast<size_t>(kMenuShardTrianglesTotal));

    float const texWf = static_cast<float>(texW);
    float const texHf = static_cast<float>(texH);
    auto const panelToWorld = [&](float lx, float ly) {
        return root->convertToWorldSpace(
            CCPoint{lx + panelWidth * 0.5f, ly + panelHeight * 0.5f}
        );
    };

    for (int row = 0; row < kMenuShardRows; ++row) {
        for (int col = 0; col < kMenuShardCols; ++col) {
            float const left = -panelWidth * 0.5f + static_cast<float>(col) * cellW;
            float const right = left + cellW;
            float const top = panelHeight * 0.5f - static_cast<float>(row) * cellH;
            float const bottom = panelHeight * 0.5f - static_cast<float>(row + 1) * cellH;

            for (int tri = 0; tri < 2; ++tri) {
                float lx[3]{};
                float ly[3]{};
                if (tri == 0) {
                    // Diagonal BL–TR: BL, BR, TR
                    lx[0] = left;
                    ly[0] = bottom;
                    lx[1] = right;
                    ly[1] = bottom;
                    lx[2] = right;
                    ly[2] = top;
                } else {
                    lx[0] = left;
                    ly[0] = bottom;
                    lx[1] = right;
                    ly[1] = top;
                    lx[2] = left;
                    ly[2] = top;
                }
                ensureCCWPanel(lx, ly);

                float const cx = (lx[0] + lx[1] + lx[2]) / 3.0f;
                float const cy = (ly[0] + ly[1] + ly[2]) / 3.0f;

                float const normCol = (cx + panelWidth * 0.5f) / panelWidth;
                float const normRow = (panelHeight * 0.5f - cy) / panelHeight;
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
                float const angularSign = ((row + col + tri) % 2 == 0) ? 1.0f : -1.0f;
                float const angularT = clamp01((normCol + normRow) * 0.5f);
                float const angularVel = angularSign
                    * (kMenuShardAngularVelocityMin
                        + (kMenuShardAngularVelocityMax - kMenuShardAngularVelocityMin) * angularT);

                CCPoint const w0 = panelToWorld(lx[0], ly[0]);
                CCPoint const w1 = panelToWorld(lx[1], ly[1]);
                CCPoint const w2 = panelToWorld(lx[2], ly[2]);
                CCPoint const cWorld = panelToWorld(cx, cy);

                PhysicsShatterBodyInit shardInit{};
                shardInit.shape = PhysicsShatterBodyShape::Triangle;
                shardInit.cornerWorldPx[0][0] = w0.x;
                shardInit.cornerWorldPx[0][1] = w0.y;
                shardInit.cornerWorldPx[1][0] = w1.x;
                shardInit.cornerWorldPx[1][1] = w1.y;
                shardInit.cornerWorldPx[2][0] = w2.x;
                shardInit.cornerWorldPx[2][1] = w2.y;
                shardInit.xPx = cWorld.x;
                shardInit.yPx = cWorld.y;
                shardInit.angleRad = shatterBaseAngleRad;
                shardInit.velocityXPx = launchVx;
                shardInit.velocityYPx = launchVy;
                shardInit.angularVelocityRad = angularVel;

                int const bodyHandle = m_physics->spawnShatterBody(shardInit);

                cocos2d::ccTex2F const uvs[3] = {
                    uvForSnapshotCorner(lx[0], ly[0], panelWidth, panelHeight, texWf, texHf),
                    uvForSnapshotCorner(lx[1], ly[1], panelWidth, panelHeight, texWf, texHf),
                    uvForSnapshotCorner(lx[2], ly[2], panelWidth, panelHeight, texWf, texHf),
                };

                float const vertLocalX[3] = {
                    lx[0] - cx,
                    lx[1] - cx,
                    lx[2] - cx,
                };
                float const vertLocalY[3] = {
                    ly[0] - cy,
                    ly[1] - cy,
                    ly[2] - cy,
                };

                auto* shard = MenuShatterTriangleNode::create(
                    shatterTex,
                    cocos2d::CCPoint{vertLocalX[0], vertLocalY[0]},
                    cocos2d::CCPoint{vertLocalX[1], vertLocalY[1]},
                    cocos2d::CCPoint{vertLocalX[2], vertLocalY[2]},
                    uvs[0],
                    uvs[1],
                    uvs[2],
                    false
                );
                if (!shard) {
                    continue;
                }
                shard->setOpacity(255);
                CCPoint const shardPosInRoot =
                    shardRoot ? shardRoot->convertToNodeSpace(cWorld) : cWorld;
                shard->setPosition(shardPosInRoot);
                shard->setRotation(prevRot);
                shardRoot->addChild(shard, kPhysicsMenuZOrder);
                m_menuShatter.pieces.push_back({bodyHandle, shard});
            }
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
        if (!piece.shard) {
            continue;
        }
        PhysicsState state{};
        if (m_physics->getShatterBodyState(piece.bodyHandle, state)) {
            CCPoint const worldPos{state.x, state.y};
            CCPoint const posInRoot = m_menuShatter.captureRoot
                ? m_menuShatter.captureRoot->convertToNodeSpace(worldPos)
                : worldPos;
            piece.shard->setPosition(posInRoot);
            piece.shard->setRotation(-state.angle * kRadToDeg);
        }
        piece.shard->setOpacity(opacity);
    }

    if (m_menuShatter.elapsed > fadeEnd) {
        clearMenuShatter();
    }
}

void PhysicsOverlay::clearMenuShatter() {
    for (auto& piece : m_menuShatter.pieces) {
        if (piece.shard) {
            piece.shard->removeFromParentAndCleanup(true);
            piece.shard = nullptr;
        }
    }
    m_menuShatter.pieces.clear();
    if (m_menuShatter.snapshot) {
        m_menuShatter.snapshot->release();
        m_menuShatter.snapshot = nullptr;
    }
    m_menuShatter.captureRoot = nullptr;
    if (m_physics) {
        m_physics->clearShatterBodies();
    }
    m_menuShatter.elapsed = 0.0f;
    m_menuShatter.active = false;
}
