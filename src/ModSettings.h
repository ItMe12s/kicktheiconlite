#pragma once

namespace mod_settings {
    // Seed all inline globals from Geode settings and register change listeners.
    // Call once from $on_mod(Loaded) before any subsystem init.
    void bindAll();
} // namespace mod_settings
