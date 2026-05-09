#pragma once

class PhysicsOverlay;

namespace runtime_restart {

// Hooks scene entry to add PhysicsOverlay, call once from $on_mod(Loaded).
void installPhysicsOverlay();
// Re-read hide-overlay setting and move or freeze overlay (live toggle).
void syncHideModOverlayFromSettings();
void registerPhysicsOverlay(PhysicsOverlay* overlay);
// Clear registration when overlay is destroyed (e.g. scene teardown).
void unregisterPhysicsOverlay(PhysicsOverlay* overlay);
// Before fullscreen/windowed GL teardown: tear down overlay GPU resources safely.
void requestFullscreenSelfDestruct(char const* source);
// After requestFullscreenSelfDestruct the user should restart the game (see about.md).
bool isRestartRequired();

}
