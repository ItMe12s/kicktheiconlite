#include "RuntimeRestart.h"

#include <Geode/Geode.hpp>
#include <Geode/ui/OverlayManager.hpp>
#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>
#endif

#include <atomic>
#include <string>

#include "PhysicsOverlay.h"

using namespace geode::prelude;

namespace runtime_restart {

namespace {

struct RestartState {
    std::atomic_bool restartRequired = false;
    std::atomic_bool teardownQueued = false;
    PhysicsOverlay* overlay = nullptr;
};

RestartState& state() {
    static RestartState s;
    return s;
}

void performOverlayTeardown(char const* source) {
    std::string const reason = source ? source : "fullscreen toggle";
    log::warn("requesting Kick the Icon self-destruct after {}", reason);

    queueInMainThread([] {
        if (state().overlay) {
            state().overlay->beginFullscreenSelfDestruct();
        }
    });
}
} // namespace

void installPhysicsOverlay() {
    if (state().restartRequired.load()) {
        return;
    }

    queueInMainThread([] {
        if (state().restartRequired.load() || state().overlay) {
            return;
        }
        if (!GameManager::get()) {
            return;
        }
        auto* manager = OverlayManager::get();
        if (!manager) {
            return;
        }
        auto* overlay = PhysicsOverlay::create();
        if (!overlay) {
            return;
        }
        if (state().restartRequired.load()) {
            unregisterPhysicsOverlay(overlay);
            return;
        }
        overlay->setZOrder(kPhysicsOverlayZOrder);
        manager->addChild(overlay);
    });
}

void syncHideModOverlayFromSettings() {
    queueInMainThread([] {
        if (state().overlay) {
            state().overlay->applyHideModOverlayFromTuning();
        }
    });
}

void registerPhysicsOverlay(PhysicsOverlay* overlay) {
    state().overlay = overlay;
}

void unregisterPhysicsOverlay(PhysicsOverlay* overlay) {
    if (state().overlay == overlay) {
        state().overlay = nullptr;
    }
}

void requestFullscreenSelfDestruct(char const* source) {
    state().restartRequired = true;
    if (state().teardownQueued.exchange(true)) {
        return;
    }
    performOverlayTeardown(source);
}

bool isRestartRequired() {
    return state().restartRequired.load();
}

} // namespace runtime_restart

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif

#ifdef GEODE_IS_WINDOWS
struct $modify(KickTheIconFullscreenHook, CCEGLView) {
    void toggleFullScreen(bool fullscreen, bool borderless, bool fix) {
        bool const wasFullscreen = this->getIsFullscreen();
        bool const wasBorderless = this->getIsBorderless();
        bool const wasFix = this->m_bIsFix;
        CCEGLView::toggleFullScreen(fullscreen, borderless, fix);
        if (
            this->getIsFullscreen() != wasFullscreen
            || this->getIsBorderless() != wasBorderless
            || this->m_bIsFix != wasFix
        ) {
            runtime_restart::requestFullscreenSelfDestruct("toggleFullScreen");
        }
    }
};
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif
