#pragma once

class PhysicsOverlay;

namespace runtime_restart {

void installPhysicsOverlay();
void bindHideModOverlaySync();
void registerPhysicsOverlay(PhysicsOverlay* overlay);
void unregisterPhysicsOverlay(PhysicsOverlay* overlay);
void requestFullscreenSelfDestruct(char const* source);
bool isRestartRequired();

}
