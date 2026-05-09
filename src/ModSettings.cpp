// Now manually maintained :D

#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <algorithm>

#include "ModSettings.h"
#include "ModTuning.h"
#include "RuntimeRestart.h"
#include "StarBurst.h"

using namespace geode;

void bindModSettings() {
    auto* mod = Mod::get();

    // Important (mod.json)
    kHideModOverlay = mod->getSettingValue<bool>("hide-mod-overlay");
    listenForSettingChanges<bool>("hide-mod-overlay", [](bool v) {
        kHideModOverlay = v;
        runtime_restart::syncHideModOverlayFromSettings();
    });

    // Visuals
    player_visual::kPlayerTargetSizeFraction = static_cast<float>(mod->getSettingValue<double>("player-visual-player-target-size-fraction"));
    listenForSettingChanges<double>("player-visual-player-target-size-fraction", [](double v) { player_visual::kPlayerTargetSizeFraction = static_cast<float>(v); });

    // Impact toggles
    kEnablePlayerImpactTrail = mod->getSettingValue<bool>("enable-player-impact-trail");
    listenForSettingChanges<bool>("enable-player-impact-trail", [](bool v) { kEnablePlayerImpactTrail = v; });
    kEnablePlayerImpactFlashStack = mod->getSettingValue<bool>("enable-player-impact-flash-stack");
    listenForSettingChanges<bool>("enable-player-impact-flash-stack", [](bool v) { kEnablePlayerImpactFlashStack = v; });

    // Impact flash
    kImpactHitstopSeconds = static_cast<float>(mod->getSettingValue<double>("impact-hitstop-seconds"));
    listenForSettingChanges<double>("impact-hitstop-seconds", [](double v) { kImpactHitstopSeconds = static_cast<float>(v); });
    kImpactFlashCooldownSeconds = static_cast<float>(mod->getSettingValue<double>("impact-flash-cooldown-seconds"));
    listenForSettingChanges<double>("impact-flash-cooldown-seconds", [](double v) { kImpactFlashCooldownSeconds = static_cast<float>(v); });
    kImpactFlashInvertPhaseEndPhaseCount = static_cast<int>(mod->getSettingValue<int64_t>("impact-flash-invert-phase-end-phase-count"));
    listenForSettingChanges<int64_t>("impact-flash-invert-phase-end-phase-count", [](int64_t v) { kImpactFlashInvertPhaseEndPhaseCount = static_cast<int>(v); });

    // Impact thresholds
    kPlayerImpactMinFlashSpeed = static_cast<float>(mod->getSettingValue<double>("player-impact-min-flash-speed"));
    listenForSettingChanges<double>("player-impact-min-flash-speed", [](double v) { kPlayerImpactMinFlashSpeed = static_cast<float>(v); });
    kPlayerImpactMinTrailSpeed = static_cast<float>(mod->getSettingValue<double>("player-impact-min-trail-speed"));
    listenForSettingChanges<double>("player-impact-min-trail-speed", [](double v) { kPlayerImpactMinTrailSpeed = static_cast<float>(v); });

    // Impact noise
    kImpactNoiseFadeSeconds = std::max(static_cast<float>(mod->getSettingValue<double>("impact-noise-fade-seconds")), 0.0001f);
    listenForSettingChanges<double>("impact-noise-fade-seconds", [](double v) { kImpactNoiseFadeSeconds = std::max(static_cast<float>(v), 0.0001f); });
    kImpactNoiseCompositeNearestFilter = mod->getSettingValue<bool>("impact-noise-composite-nearest-filter");
    listenForSettingChanges<bool>("impact-noise-composite-nearest-filter", [](bool v) { kImpactNoiseCompositeNearestFilter = v; });
    kImpactNoiseRenderScale = static_cast<float>(mod->getSettingValue<double>("impact-noise-render-scale"));
    listenForSettingChanges<double>("impact-noise-render-scale", [](double v) { kImpactNoiseRenderScale = static_cast<float>(v); });

    // Star burst
    kBigStarCount = static_cast<int>(mod->getSettingValue<int64_t>("big-star-count"));
    kSmallStarCount = static_cast<int>(mod->getSettingValue<int64_t>("small-star-count"));
    star_burst::clampStarBurstCountsInPlace();
    listenForSettingChanges<int64_t>("big-star-count", [](int64_t v) {
        kBigStarCount = static_cast<int>(v);
        star_burst::clampStarBurstCountsInPlace();
    });
    listenForSettingChanges<int64_t>("small-star-count", [](int64_t v) {
        kSmallStarCount = static_cast<int>(v);
        star_burst::clampStarBurstCountsInPlace();
    });
    kBigStarRadiusMin = static_cast<float>(mod->getSettingValue<double>("big-star-radius-min"));
    listenForSettingChanges<double>("big-star-radius-min", [](double v) { kBigStarRadiusMin = static_cast<float>(v); });
    kBigStarRadiusMax = static_cast<float>(mod->getSettingValue<double>("big-star-radius-max"));
    listenForSettingChanges<double>("big-star-radius-max", [](double v) { kBigStarRadiusMax = static_cast<float>(v); });
    kSmallStarRadiusMin = static_cast<float>(mod->getSettingValue<double>("small-star-radius-min"));
    listenForSettingChanges<double>("small-star-radius-min", [](double v) { kSmallStarRadiusMin = static_cast<float>(v); });
    kSmallStarRadiusMax = static_cast<float>(mod->getSettingValue<double>("small-star-radius-max"));
    listenForSettingChanges<double>("small-star-radius-max", [](double v) { kSmallStarRadiusMax = static_cast<float>(v); });
    kBigStarScreenFrac = static_cast<float>(mod->getSettingValue<double>("big-star-screen-frac"));
    listenForSettingChanges<double>("big-star-screen-frac", [](double v) { kBigStarScreenFrac = static_cast<float>(v); });
    kSmallStarScreenFrac = static_cast<float>(mod->getSettingValue<double>("small-star-screen-frac"));
    listenForSettingChanges<double>("small-star-screen-frac", [](double v) { kSmallStarScreenFrac = static_cast<float>(v); });
    kStarScaleVariance = static_cast<float>(mod->getSettingValue<double>("star-scale-variance"));
    listenForSettingChanges<double>("star-scale-variance", [](double v) { kStarScaleVariance = static_cast<float>(v); });

    // Sandevistan
    kSandevistanEndSpeedPx = static_cast<float>(mod->getSettingValue<double>("sandevistan-end-speed-px"));
    listenForSettingChanges<double>("sandevistan-end-speed-px", [](double v) { kSandevistanEndSpeedPx = static_cast<float>(v); });
    kSandevistanSpawnIntervalSec = static_cast<float>(mod->getSettingValue<double>("sandevistan-spawn-interval-sec"));
    listenForSettingChanges<double>("sandevistan-spawn-interval-sec", [](double v) { kSandevistanSpawnIntervalSec = static_cast<float>(v); });
    kSandevistanGhostFadeSec = static_cast<float>(mod->getSettingValue<double>("sandevistan-ghost-fade-sec"));
    listenForSettingChanges<double>("sandevistan-ghost-fade-sec", [](double v) { kSandevistanGhostFadeSec = static_cast<float>(v); });
    kSandevistanGhostStartOpacity = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-ghost-start-opacity"));
    listenForSettingChanges<int64_t>("sandevistan-ghost-start-opacity", [](int64_t v) { kSandevistanGhostStartOpacity = static_cast<int>(v); });
    kSandevistanMaxConcurrentGhosts = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-max-concurrent-ghosts"));
    listenForSettingChanges<int64_t>("sandevistan-max-concurrent-ghosts", [](int64_t v) { kSandevistanMaxConcurrentGhosts = static_cast<int>(v); });
    kSandevistanTrailHueOrangeR = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-orange-r"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-orange-r", [](int64_t v) { kSandevistanTrailHueOrangeR = static_cast<int>(v); });
    kSandevistanTrailHueOrangeG = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-orange-g"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-orange-g", [](int64_t v) { kSandevistanTrailHueOrangeG = static_cast<int>(v); });
    kSandevistanTrailHueOrangeB = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-orange-b"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-orange-b", [](int64_t v) { kSandevistanTrailHueOrangeB = static_cast<int>(v); });
    kSandevistanTrailHuePurpleR = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-purple-r"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-purple-r", [](int64_t v) { kSandevistanTrailHuePurpleR = static_cast<int>(v); });
    kSandevistanTrailHuePurpleG = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-purple-g"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-purple-g", [](int64_t v) { kSandevistanTrailHuePurpleG = static_cast<int>(v); });
    kSandevistanTrailHuePurpleB = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-purple-b"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-purple-b", [](int64_t v) { kSandevistanTrailHuePurpleB = static_cast<int>(v); });
    kSandevistanTrailHueCyanR = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-cyan-r"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-cyan-r", [](int64_t v) { kSandevistanTrailHueCyanR = static_cast<int>(v); });
    kSandevistanTrailHueCyanG = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-cyan-g"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-cyan-g", [](int64_t v) { kSandevistanTrailHueCyanG = static_cast<int>(v); });
    kSandevistanTrailHueCyanB = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-hue-cyan-b"));
    listenForSettingChanges<int64_t>("sandevistan-trail-hue-cyan-b", [](int64_t v) { kSandevistanTrailHueCyanB = static_cast<int>(v); });

    // Fire aura
    kMinFireAuraSpeedPx = static_cast<float>(mod->getSettingValue<double>("min-fire-aura-speed-px"));
    listenForSettingChanges<double>("min-fire-aura-speed-px", [](double v) { kMinFireAuraSpeedPx = static_cast<float>(v); });
    kMaxFireAuraSpeedPx = static_cast<float>(mod->getSettingValue<double>("max-fire-aura-speed-px"));
    listenForSettingChanges<double>("max-fire-aura-speed-px", [](double v) { kMaxFireAuraSpeedPx = static_cast<float>(v); });
    kFireAuraDiameterScale = static_cast<float>(mod->getSettingValue<double>("fire-aura-diameter-scale"));
    listenForSettingChanges<double>("fire-aura-diameter-scale", [](double v) { kFireAuraDiameterScale = static_cast<float>(v); });
    kFireAuraVelocityToShader = static_cast<float>(mod->getSettingValue<double>("fire-aura-velocity-to-shader"));
    listenForSettingChanges<double>("fire-aura-velocity-to-shader", [](double v) { kFireAuraVelocityToShader = static_cast<float>(v); });
    kFireAuraDefaultPrimaryR = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-primary-r"));
    listenForSettingChanges<double>("fire-aura-default-primary-r", [](double v) { kFireAuraDefaultPrimaryR = static_cast<float>(v); });
    kFireAuraDefaultPrimaryG = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-primary-g"));
    listenForSettingChanges<double>("fire-aura-default-primary-g", [](double v) { kFireAuraDefaultPrimaryG = static_cast<float>(v); });
    kFireAuraDefaultPrimaryB = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-primary-b"));
    listenForSettingChanges<double>("fire-aura-default-primary-b", [](double v) { kFireAuraDefaultPrimaryB = static_cast<float>(v); });
    kFireAuraDefaultSecondaryR = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-secondary-r"));
    listenForSettingChanges<double>("fire-aura-default-secondary-r", [](double v) { kFireAuraDefaultSecondaryR = static_cast<float>(v); });
    kFireAuraDefaultSecondaryG = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-secondary-g"));
    listenForSettingChanges<double>("fire-aura-default-secondary-g", [](double v) { kFireAuraDefaultSecondaryG = static_cast<float>(v); });
    kFireAuraDefaultSecondaryB = static_cast<float>(mod->getSettingValue<double>("fire-aura-default-secondary-b"));
    listenForSettingChanges<double>("fire-aura-default-secondary-b", [](double v) { kFireAuraDefaultSecondaryB = static_cast<float>(v); });

    // Motion blur
    kPlayerMinBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("player-min-blur-speed-px"));
    listenForSettingChanges<double>("player-min-blur-speed-px", [](double v) { kPlayerMinBlurSpeedPx = static_cast<float>(v); });
    kPlayerMaxBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("player-max-blur-speed-px"));
    listenForSettingChanges<double>("player-max-blur-speed-px", [](double v) { kPlayerMaxBlurSpeedPx = static_cast<float>(v); });
    kPlayerBlurUvSpread = static_cast<float>(mod->getSettingValue<double>("player-blur-uv-spread"));
    listenForSettingChanges<double>("player-blur-uv-spread", [](double v) { kPlayerBlurUvSpread = static_cast<float>(v); });

    // Debug
    kDebugLabelEnabled = mod->getSettingValue<bool>("debug-label-enabled");
    listenForSettingChanges<bool>("debug-label-enabled", [](bool v) { kDebugLabelEnabled = v; });

    runtime_restart::syncHideModOverlayFromSettings();
}
