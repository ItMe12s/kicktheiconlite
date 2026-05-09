#pragma once

// Seed all tuning globals from Geode settings and register change listeners.
// Call once from $on_mod(Loaded) before any subsystem init.
void bindModSettings();
