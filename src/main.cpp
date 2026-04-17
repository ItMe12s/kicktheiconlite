#include <Geode/Geode.hpp>

#include "ModSettings.h"
#include "RuntimeRestart.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif

$on_mod(Loaded) {
    mod_settings::bindAll();
    runtime_restart::installPhysicsOverlay();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
