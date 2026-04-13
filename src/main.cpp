#include <Geode/Geode.hpp>
#include <Geode/ui/OverlayManager.hpp>

#include "PhysicsOverlay.h"

using namespace geode::prelude;

$on_mod(Loaded) {
    geode::queueInMainThread([] {
        auto overlay = PhysicsOverlay::create();
        if (!overlay) {
            return;
        }
        overlay->setZOrder(kPhysicsOverlayZOrder);
        geode::OverlayManager::get()->addChild(overlay);
    });
}
