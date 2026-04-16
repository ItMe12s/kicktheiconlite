#include "PhysicsOverlay.h"

#include <Geode/Enums.hpp>
#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/utils/cocos.hpp>

#include <cmath>

#include "OverlayRendering.h"
#include "PhysicsWorld.h"
#include "PhysicsMenu.h"
#include "PlayerVisual.h"
#include "events/ClickEvents.h"
#include "vfx/ObjectMotionBlurPipeline.h"
#include "vfx/StarBurst.h"

using namespace geode::prelude;

namespace {

cocos2d::CCNode* overlayLayerRoot(
    std::array<cocos2d::CCNode*, overlay_rendering::kOverlayLayerCount> const& roots,
    overlay_rendering::OverlayLayerId id
) {
    return roots.at(static_cast<size_t>(id));
}

} // namespace

void PhysicsOverlay::tryBuildPlayerVisual() {
    if (m_visualBuilt) {
        return;
    }

    auto pr = player_visual::tryBuildPlayerRoot(
        this,
        m_winSize,
        m_targetSize,
        m_frameId,
        m_iconTypeInt
    );
    if (!pr.ok) {
        return;
    }

    cocos2d::CCSize const blurCaptureSize = m_winSize;
    if (!vfx::object_motion_blur::attach(
        m_objectBlur,
        this,
        blurCaptureSize,
        m_winSize,
        kUnifiedBlurCompositeZOrder,
        {
            overlay_rendering::MotionBlurObjectSeed {
                .id = overlay_rendering::MotionBlurObjectId::Player,
                .sourceRoot = pr.root,
                .enabled = true,
                .tuning = {
                    .minBlurSpeedPx = kPlayerMinBlurSpeedPx,
                    .maxBlurSpeedPx = kPlayerMaxBlurSpeedPx,
                    .blurUvSpread = kPlayerBlurUvSpread,
                    .blurStepDivisor = kPlayerBlurStepDivisor,
                    .keepBaseVisible = kPlayerKeepBaseVisible,
                    .alwaysCaptureWhenEnabled = false,
                },
            },
            overlay_rendering::MotionBlurObjectSeed {
                .id = overlay_rendering::MotionBlurObjectId::PhysicsMenu,
                .sourceRoot = nullptr,
                .enabled = false,
                .tuning = {
                    .minBlurSpeedPx = kMenuMinBlurSpeedPx,
                    .maxBlurSpeedPx = kMenuMaxBlurSpeedPx,
                    .blurUvSpread = kMenuBlurUvSpread,
                    .blurStepDivisor = kMenuBlurStepDivisor,
                    .keepBaseVisible = kMenuKeepBaseVisible,
                    .alwaysCaptureWhenEnabled = false,
                },
            },
        }
    )) {
        pr.root->removeFromParentAndCleanup(true);
        return;
    }

    m_playerRoot = pr.root;
    auto* worldRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::World);
    if (worldRoot) {
        m_playerRoot->retain();
        this->removeChild(m_playerRoot, false);
        worldRoot->addChild(m_playerRoot, kPlayerRootZOrder);
        m_playerRoot->release();
    } else {
        m_playerRoot->setZOrder(kPlayerRootZOrder);
    }
    m_player = pr.player;

    auto const fa = overlay_rendering::attachFireAura(pr.root, m_targetSize * kFireAuraDiameterScale);
    if (fa.ok) {
        m_fireAura.sprite = fa.sprite;
        m_fireAura.program = fa.program;
    }

    vfx::star_burst::createSprites(m_starBurst);

    // Always-visible invisible sprite so hit tests work when SimplePlayer is hidden (like motion blur)
    // ClickTracker::untrack(hitProxy) before destruction (see onExit / teardown paths)
    if (auto* hitProxy = CCSprite::create("img_star1.png"_spr)) {
        hitProxy->setID(std::string(GEODE_MOD_ID) + "/player-hit-proxy");
        hitProxy->setPosition({0.f, 0.f});
        hitProxy->setOpacity(0);
        hitProxy->setVisible(true);
        m_playerRoot->addChild(hitProxy, kHitProxyLocalZOrder);
        m_hitProxy = hitProxy;
        events::ClickTracker::get()->track(m_hitProxy, m_targetSize * kGrabRadiusFraction);
    }

    m_doubleClickListener = events::DoubleClickEvent().listen([this](cocos2d::CCSprite* sprite, cocos2d::CCTouch*) {
        if (sprite != m_hitProxy) {
            return false;
        }
        log::info("double clicked the icon");
        return false;
    });
    m_tripleClickListener = events::TripleClickEvent().listen([this](cocos2d::CCSprite* sprite, cocos2d::CCTouch*) {
        if (sprite != m_hitProxy) {
            return false;
        }
        tryOpenPhysicsMenu();
        return false;
    });

    m_visualBuilt = true;
}

bool PhysicsOverlay::tryBeginGrab(CCPoint const& locationInNode) {
    if (!m_physics) {
        return false;
    }

    auto const state = m_physics->getPlayerState();
    float const dx = locationInNode.x - state.x;
    float const dy = locationInNode.y - state.y;
    float const distSq = dx * dx + dy * dy;
    float const grabRadius = m_targetSize * kGrabRadiusFraction;
    if (distSq > grabRadius * grabRadius) {
        return false;
    }

    m_physics->setDragGrabOffsetPixels(dx, dy);
    m_physics->setDragTargetPixels(locationInNode.x, locationInNode.y);
    m_physics->setDragging(true);
    m_grabActive = true;

    m_impactFlash.hitstopRemaining = 0.0f;
    m_impactFlash.whiteFlashRemaining = 0.0f;
    vfx::star_burst::reset(m_starBurst);
    if (m_objectBlur.whiteFlashSprite) {
        m_objectBlur.whiteFlashSprite->stopAllActions();
    }
    return true;
}

void PhysicsOverlay::endGrab() {
    if (!m_grabActive) {
        return;
    }
    m_grabActive = false;
    if (m_physics) {
        m_physics->setDragging(false);
    }
}

void PhysicsOverlay::tryOpenPhysicsMenu() {
    if (!m_physics) {
        return;
    }
    if (m_physicsMenuVisual) {
        return;
    }
    clearMenuShatter();

    float const w = m_winSize.width * kPanelDefaultWFrac;
    float const h = m_winSize.height * kPanelDefaultHFrac;
    float const x = m_winSize.width * kPanelDefaultXFrac;
    float const y = m_winSize.height * kPanelDefaultYFrac;

    auto panel = std::make_unique<PhysicsMenu>();
    if (!panel->build(w, h)) {
        return;
    }
    if (auto* root = panel->getRoot()) {
        auto* uiLayerRoot = overlayLayerRoot(m_layerRoots, overlay_rendering::OverlayLayerId::Ui);
        if (uiLayerRoot) {
            uiLayerRoot->addChild(root, kPhysicsMenuZOrder);
        } else {
            log::warn("world capture root missing; physics menu will bypass blur capture");
            this->addChild(root, kPhysicsMenuZOrder);
        }
        auto& menuCapture =
            m_objectBlur.objects[static_cast<size_t>(overlay_rendering::MotionBlurObjectId::PhysicsMenu)];
        menuCapture.sourceRoot = root;
        menuCapture.enabled = true;
    }
    m_physics->spawnPanel(w, h, x, y);
    m_physicsMenuVisual = std::move(panel);
    log::info("physics menu spawned");
}

bool PhysicsOverlay::tryBeginPanelGrab(CCPoint const& locationInNode) {
    if (!m_physicsMenuVisual || !m_physics || !m_physics->hasPanel()) {
        return false;
    }
    PhysicsState const st = m_physics->getPanelState();
    float const dx = locationInNode.x - st.x;
    float const dy = locationInNode.y - st.y;
    float const c = std::cos(st.angle);
    float const s = std::sin(st.angle);
    float const localX = c * dx + s * dy;
    float const localY = -s * dx + c * dy;
    if (!m_physicsMenuVisual->containsLocalPoint(localX, localY)) {
        return false;
    }
    m_physics->setPanelDragGrabOffsetPixels(dx, dy);
    m_physics->setPanelDragTargetPixels(locationInNode.x, locationInNode.y);
    m_physics->setPanelDragging(true);
    m_panelDragActive = true;
    return true;
}

void PhysicsOverlay::syncPanelNodeFromPhysics(float alpha) {
    if (!m_physicsMenuVisual || !m_physics || !m_physics->hasPanel()) {
        return;
    }
    auto* root = m_physicsMenuVisual->getRoot();
    if (!root) {
        return;
    }
    PhysicsState const st = m_physics->getPanelRenderState(alpha);
    root->setPosition({st.x, st.y});
    root->setRotation(-st.angle * kRadToDeg);
}

void PhysicsOverlay::endTouchInteraction() {
    if (m_panelDragActive && m_physics) {
        m_physics->setPanelDragging(false);
        m_panelDragActive = false;
    }
    endGrab();
}

bool PhysicsOverlay::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    (void)event;
    if (!m_physics) {
        return false;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    if (m_physicsMenuVisual && m_physics->hasPanel() && tryBeginPanelGrab(p)) {
        events::ClickTracker::get()->onTouchBegan(touch);
        return true;
    }
    bool const clickHit = events::ClickTracker::get()->onTouchBegan(touch);
    bool const grabbed = tryBeginGrab(p);
    return clickHit || grabbed;
}

void PhysicsOverlay::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    (void)event;
    if (!m_physics) {
        return;
    }
    CCPoint const p = this->convertTouchToNodeSpace(touch);
    if (m_panelDragActive) {
        m_physics->setPanelDragTargetPixels(p.x, p.y);
        return;
    }
    if (!m_grabActive) {
        return;
    }
    m_physics->setDragTargetPixels(p.x, p.y);
}

void PhysicsOverlay::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    (void)event;
    events::ClickTracker::get()->onTouchEnded(touch);
    endTouchInteraction();
}

void PhysicsOverlay::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    (void)event;
    events::ClickTracker::get()->onTouchCancelled(touch);
    endTouchInteraction();
}
