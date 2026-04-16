#include <Geode/Geode.hpp>

#include "RuntimeRestart.h"

$on_mod(Loaded) {
    runtime_restart::installPhysicsOverlay();
}
