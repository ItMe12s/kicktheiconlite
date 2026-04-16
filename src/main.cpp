#include <Geode/Geode.hpp>

#include "RuntimeRestart.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif

$on_mod(Loaded) {
    runtime_restart::installPhysicsOverlay();
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
