# Important

Please do not fork this, it's incomplete in terms of feature and the codebase is super outdated from the full version.

## Resources

Assets stay in `src/assets/`, only files listed in `mod.json` "resources" are packed.

## Tunables

Runtime defaults and externs live in [`src/ModTuning.h`](src/ModTuning.h) with values seeded from Geode settings in [`src/ModSettings.cpp`](src/ModSettings.cpp) (keys mirror `mod.json`).
