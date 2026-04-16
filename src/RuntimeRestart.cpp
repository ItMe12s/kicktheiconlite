#include "RuntimeRestart.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
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
std::atomic_bool g_restartRequired = false;
std::atomic_bool g_teardownQueued = false;
// Pointer only read/written from main thread (register/unregister, queueInMainThread callbacks)
std::atomic<PhysicsOverlay*> g_overlay{nullptr};

void performOverlayTeardown(char const* source) {
    std::string const reason = source ? source : "fullscreen toggle";
    log::warn("requesting Kick the Icon self-destruct after {}", reason);

    queueInMainThread([] {
        if (auto* overlay = g_overlay.load(std::memory_order_relaxed)) {
            overlay->beginFullscreenSelfDestruct();
        }
    });
}
} // namespace

void installPhysicsOverlay() {
    if (g_restartRequired.load()) {
        return;
    }

    queueInMainThread([] {
        if (g_restartRequired.load() || g_overlay.load(std::memory_order_relaxed)) {
            return;
        }

        auto* overlay = PhysicsOverlay::create();
        if (!overlay) {
            return;
        }
        if (g_restartRequired.load()) {
            unregisterPhysicsOverlay(overlay);
            return;
        }
        overlay->setZOrder(kPhysicsOverlayZOrder);
        OverlayManager::get()->addChild(overlay);
    });
}

void registerPhysicsOverlay(PhysicsOverlay* overlay) {
    g_overlay.store(overlay, std::memory_order_relaxed);
}

void unregisterPhysicsOverlay(PhysicsOverlay* overlay) {
    auto* cur = g_overlay.load(std::memory_order_relaxed);
    if (cur == overlay) {
        g_overlay.store(nullptr, std::memory_order_relaxed);
    }
}

void requestFullscreenSelfDestruct(char const* source) {
    g_restartRequired = true;
    if (g_teardownQueued.exchange(true)) {
        return;
    }
    performOverlayTeardown(source);
}

bool isRestartRequired() {
    return g_restartRequired.load();
}

} // namespace runtime_restart

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif

struct $modify(KickTheIconMenuLayerHook, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }
        runtime_restart::installPhysicsOverlay();
        return true;
    }
};

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
