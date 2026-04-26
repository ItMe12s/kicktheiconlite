// @@GENERATED do not edit! Regenerate: python gen_mod_settings.py --bind-cpp src/ModSettings.cpp
// Source: src\ModTuning.h

#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <algorithm>
#include "ModSettings.h"
#include "ModTuning.h"

using namespace geode;

namespace mod_settings {

void bindAll() {
    auto* mod = Mod::get();

    // Player visual namespace
    player_visual::kMaxWorldBoundsTreeDepth = static_cast<int>(mod->getSettingValue<int64_t>("player-visual-max-world-bounds-tree-depth"));
    listenForSettingChanges<int64_t>("player-visual-max-world-bounds-tree-depth", [](int64_t v) { player_visual::kMaxWorldBoundsTreeDepth = static_cast<int>(v); });
    player_visual::kMinVisualWidthPx = static_cast<float>(mod->getSettingValue<double>("player-visual-min-visual-width-px"));
    listenForSettingChanges<double>("player-visual-min-visual-width-px", [](double v) { player_visual::kMinVisualWidthPx = static_cast<float>(v); });
    player_visual::kPlayerTargetSizeFraction = static_cast<float>(mod->getSettingValue<double>("player-visual-player-target-size-fraction"));
    listenForSettingChanges<double>("player-visual-player-target-size-fraction", [](double v) { player_visual::kPlayerTargetSizeFraction = static_cast<float>(v); });
    player_visual::kMinPlayerFrameId = static_cast<int>(mod->getSettingValue<int64_t>("player-visual-min-player-frame-id"));
    listenForSettingChanges<int64_t>("player-visual-min-player-frame-id", [](int64_t v) { player_visual::kMinPlayerFrameId = static_cast<int>(v); });
    player_visual::kPlayerRootAnchorXFrac = static_cast<float>(mod->getSettingValue<double>("player-visual-player-root-anchor-x-frac"));
    listenForSettingChanges<double>("player-visual-player-root-anchor-x-frac", [](double v) { player_visual::kPlayerRootAnchorXFrac = static_cast<float>(v); });
    player_visual::kPlayerRootAnchorYFrac = static_cast<float>(mod->getSettingValue<double>("player-visual-player-root-anchor-y-frac"));
    listenForSettingChanges<double>("player-visual-player-root-anchor-y-frac", [](double v) { player_visual::kPlayerRootAnchorYFrac = static_cast<float>(v); });
    player_visual::kPlayerVisualLocalZOrder = static_cast<int>(mod->getSettingValue<int64_t>("player-visual-player-visual-local-z-order"));
    listenForSettingChanges<int64_t>("player-visual-player-visual-local-z-order", [](int64_t v) { player_visual::kPlayerVisualLocalZOrder = static_cast<int>(v); });

    // Overlay and input ordering
    kPhysicsOverlayZOrder = static_cast<int>(mod->getSettingValue<int64_t>("physics-overlay-z-order"));
    listenForSettingChanges<int64_t>("physics-overlay-z-order", [](int64_t v) { kPhysicsOverlayZOrder = static_cast<int>(v); });
    kPhysicsOverlayTouchPriority = static_cast<int>(mod->getSettingValue<int64_t>("physics-overlay-touch-priority"));
    listenForSettingChanges<int64_t>("physics-overlay-touch-priority", [](int64_t v) { kPhysicsOverlayTouchPriority = static_cast<int>(v); });
    kPhysicsOverlaySchedulerPriority = static_cast<int>(mod->getSettingValue<int64_t>("physics-overlay-scheduler-priority"));
    listenForSettingChanges<int64_t>("physics-overlay-scheduler-priority", [](int64_t v) { kPhysicsOverlaySchedulerPriority = static_cast<int>(v); });
    kHitProxyLocalZOrder = static_cast<int>(mod->getSettingValue<int64_t>("hit-proxy-local-z-order"));
    listenForSettingChanges<int64_t>("hit-proxy-local-z-order", [](int64_t v) { kHitProxyLocalZOrder = static_cast<int>(v); });

    // Simulation and math
    kPlayerRootZOrder = static_cast<int>(mod->getSettingValue<int64_t>("player-root-z-order"));
    listenForSettingChanges<int64_t>("player-root-z-order", [](int64_t v) { kPlayerRootZOrder = static_cast<int>(v); });
    kMinSpeedForInverse = static_cast<float>(mod->getSettingValue<double>("min-speed-for-inverse"));
    listenForSettingChanges<double>("min-speed-for-inverse", [](double v) { kMinSpeedForInverse = static_cast<float>(v); });
    kGrabRadiusFraction = static_cast<float>(mod->getSettingValue<double>("grab-radius-fraction"));
    listenForSettingChanges<double>("grab-radius-fraction", [](double v) { kGrabRadiusFraction = static_cast<float>(v); });

    // Global render layering
    kImpactFlashBackdropZOrder = static_cast<int>(mod->getSettingValue<int64_t>("impact-flash-backdrop-z-order"));
    listenForSettingChanges<int64_t>("impact-flash-backdrop-z-order", [](int64_t v) { kImpactFlashBackdropZOrder = static_cast<int>(v); });
    kFireAuraZOrder = static_cast<int>(mod->getSettingValue<int64_t>("fire-aura-z-order"));
    listenForSettingChanges<int64_t>("fire-aura-z-order", [](int64_t v) { kFireAuraZOrder = static_cast<int>(v); });
    kStarBurstZOrder = static_cast<int>(mod->getSettingValue<int64_t>("star-burst-z-order"));
    listenForSettingChanges<int64_t>("star-burst-z-order", [](int64_t v) { kStarBurstZOrder = static_cast<int>(v); });
    kUnifiedWorldCaptureZOrder = static_cast<int>(mod->getSettingValue<int64_t>("unified-world-capture-z-order"));
    listenForSettingChanges<int64_t>("unified-world-capture-z-order", [](int64_t v) { kUnifiedWorldCaptureZOrder = static_cast<int>(v); });
    kUnifiedBlurCompositeZOrder = static_cast<int>(mod->getSettingValue<int64_t>("unified-blur-composite-z-order"));
    listenForSettingChanges<int64_t>("unified-blur-composite-z-order", [](int64_t v) { kUnifiedBlurCompositeZOrder = static_cast<int>(v); });
    kImpactNoiseZOrder = static_cast<int>(mod->getSettingValue<int64_t>("impact-noise-z-order"));
    listenForSettingChanges<int64_t>("impact-noise-z-order", [](int64_t v) { kImpactNoiseZOrder = static_cast<int>(v); });
    kGlobalStartBurstZOrder = static_cast<int>(mod->getSettingValue<int64_t>("global-start-burst-z-order"));
    listenForSettingChanges<int64_t>("global-start-burst-z-order", [](int64_t v) { kGlobalStartBurstZOrder = static_cast<int>(v); });
    kPhysicsMenuZOrder = static_cast<int>(mod->getSettingValue<int64_t>("physics-menu-z-order"));
    listenForSettingChanges<int64_t>("physics-menu-z-order", [](int64_t v) { kPhysicsMenuZOrder = static_cast<int>(v); });
    kLayerTrailZOrderOffset = static_cast<int>(mod->getSettingValue<int64_t>("layer-trail-z-order-offset"));
    listenForSettingChanges<int64_t>("layer-trail-z-order-offset", [](int64_t v) { kLayerTrailZOrderOffset = static_cast<int>(v); });
    kLayerWorldZOrderOffset = static_cast<int>(mod->getSettingValue<int64_t>("layer-world-z-order-offset"));
    listenForSettingChanges<int64_t>("layer-world-z-order-offset", [](int64_t v) { kLayerWorldZOrderOffset = static_cast<int>(v); });
    kLayerUiZOrderOffset = static_cast<int>(mod->getSettingValue<int64_t>("layer-ui-z-order-offset"));
    listenForSettingChanges<int64_t>("layer-ui-z-order-offset", [](int64_t v) { kLayerUiZOrderOffset = static_cast<int>(v); });

    // Impact response toggles
    kEnablePlayerImpactTrail = mod->getSettingValue<bool>("enable-player-impact-trail");
    listenForSettingChanges<bool>("enable-player-impact-trail", [](bool v) { kEnablePlayerImpactTrail = v; });
    kEnablePlayerImpactFlashStack = mod->getSettingValue<bool>("enable-player-impact-flash-stack");
    listenForSettingChanges<bool>("enable-player-impact-flash-stack", [](bool v) { kEnablePlayerImpactFlashStack = v; });
    kEnablePanelImpactShake = mod->getSettingValue<bool>("enable-panel-impact-shake");
    listenForSettingChanges<bool>("enable-panel-impact-shake", [](bool v) { kEnablePanelImpactShake = v; });

    // Impact timing and phase
    kImpactHitstopSeconds = static_cast<float>(mod->getSettingValue<double>("impact-hitstop-seconds"));
    listenForSettingChanges<double>("impact-hitstop-seconds", [](double v) { kImpactHitstopSeconds = static_cast<float>(v); });
    kImpactFlashCooldownSeconds = static_cast<float>(mod->getSettingValue<double>("impact-flash-cooldown-seconds"));
    listenForSettingChanges<double>("impact-flash-cooldown-seconds", [](double v) { kImpactFlashCooldownSeconds = static_cast<float>(v); });
    kImpactFlashInvertPhaseEndPhaseCount = static_cast<int>(mod->getSettingValue<int64_t>("impact-flash-invert-phase-end-phase-count"));
    listenForSettingChanges<int64_t>("impact-flash-invert-phase-end-phase-count", [](int64_t v) { kImpactFlashInvertPhaseEndPhaseCount = static_cast<int>(v); });

    // Screen shake
    kPlayerImpactMinFlashSpeed = static_cast<float>(mod->getSettingValue<double>("player-impact-min-flash-speed"));
    listenForSettingChanges<double>("player-impact-min-flash-speed", [](double v) { kPlayerImpactMinFlashSpeed = static_cast<float>(v); });
    kPlayerImpactMinShakeSpeed = static_cast<float>(mod->getSettingValue<double>("player-impact-min-shake-speed"));
    listenForSettingChanges<double>("player-impact-min-shake-speed", [](double v) { kPlayerImpactMinShakeSpeed = static_cast<float>(v); });
    kPlayerImpactShakeDuration = static_cast<float>(mod->getSettingValue<double>("player-impact-shake-duration"));
    listenForSettingChanges<double>("player-impact-shake-duration", [](double v) { kPlayerImpactShakeDuration = static_cast<float>(v); });
    kPlayerImpactShakeSpeedToStrength = static_cast<float>(mod->getSettingValue<double>("player-impact-shake-speed-to-strength"));
    listenForSettingChanges<double>("player-impact-shake-speed-to-strength", [](double v) { kPlayerImpactShakeSpeedToStrength = static_cast<float>(v); });
    kPlayerImpactMaxShakeStrength = static_cast<float>(mod->getSettingValue<double>("player-impact-max-shake-strength"));
    listenForSettingChanges<double>("player-impact-max-shake-strength", [](double v) { kPlayerImpactMaxShakeStrength = static_cast<float>(v); });
    kPanelImpactMinShakeSpeed = static_cast<float>(mod->getSettingValue<double>("panel-impact-min-shake-speed"));
    listenForSettingChanges<double>("panel-impact-min-shake-speed", [](double v) { kPanelImpactMinShakeSpeed = static_cast<float>(v); });
    kPanelImpactShakeDuration = static_cast<float>(mod->getSettingValue<double>("panel-impact-shake-duration"));
    listenForSettingChanges<double>("panel-impact-shake-duration", [](double v) { kPanelImpactShakeDuration = static_cast<float>(v); });
    kPanelImpactShakeSpeedToStrength = static_cast<float>(mod->getSettingValue<double>("panel-impact-shake-speed-to-strength"));
    listenForSettingChanges<double>("panel-impact-shake-speed-to-strength", [](double v) { kPanelImpactShakeSpeedToStrength = static_cast<float>(v); });
    kPanelImpactMaxShakeStrength = static_cast<float>(mod->getSettingValue<double>("panel-impact-max-shake-strength"));
    listenForSettingChanges<double>("panel-impact-max-shake-strength", [](double v) { kPanelImpactMaxShakeStrength = static_cast<float>(v); });
    kPanelShatterMinImpactSpeed = static_cast<float>(mod->getSettingValue<double>("panel-shatter-min-impact-speed"));
    listenForSettingChanges<double>("panel-shatter-min-impact-speed", [](double v) { kPanelShatterMinImpactSpeed = static_cast<float>(v); });
    kScreenShakeIntervals = std::max(static_cast<int>(static_cast<int>(mod->getSettingValue<int64_t>("screen-shake-intervals"))), 1);
    listenForSettingChanges<int64_t>("screen-shake-intervals", [](int64_t v) { kScreenShakeIntervals = std::max(static_cast<int>(v), 1); });
    kScreenShakeSampleMin = static_cast<float>(mod->getSettingValue<double>("screen-shake-sample-min"));
    listenForSettingChanges<double>("screen-shake-sample-min", [](double v) { kScreenShakeSampleMin = static_cast<float>(v); });
    kScreenShakeSampleMax = static_cast<float>(mod->getSettingValue<double>("screen-shake-sample-max"));
    listenForSettingChanges<double>("screen-shake-sample-max", [](double v) { kScreenShakeSampleMax = static_cast<float>(v); });
    kScreenShakeCooldownExtraSeconds = static_cast<float>(mod->getSettingValue<double>("screen-shake-cooldown-extra-seconds"));
    listenForSettingChanges<double>("screen-shake-cooldown-extra-seconds", [](double v) { kScreenShakeCooldownExtraSeconds = static_cast<float>(v); });

    // Impact noise pass
    kImpactNoiseFadeSeconds = std::max(static_cast<float>(static_cast<float>(mod->getSettingValue<double>("impact-noise-fade-seconds"))), 0.0001f);
    listenForSettingChanges<double>("impact-noise-fade-seconds", [](double v) { kImpactNoiseFadeSeconds = std::max(static_cast<float>(v), 0.0001f); });
    kImpactNoiseStackedImpactTimeSkip = static_cast<float>(mod->getSettingValue<double>("impact-noise-stacked-impact-time-skip"));
    listenForSettingChanges<double>("impact-noise-stacked-impact-time-skip", [](double v) { kImpactNoiseStackedImpactTimeSkip = static_cast<float>(v); });
    kImpactNoiseCompositeNearestFilter = mod->getSettingValue<bool>("impact-noise-composite-nearest-filter");
    listenForSettingChanges<bool>("impact-noise-composite-nearest-filter", [](bool v) { kImpactNoiseCompositeNearestFilter = v; });
    kImpactNoiseRenderScale = static_cast<float>(mod->getSettingValue<double>("impact-noise-render-scale"));
    listenForSettingChanges<double>("impact-noise-render-scale", [](double v) { kImpactNoiseRenderScale = static_cast<float>(v); });

    // Star burst composition
    kBigStarCount = static_cast<int>(mod->getSettingValue<int64_t>("big-star-count"));
    listenForSettingChanges<int64_t>("big-star-count", [](int64_t v) { kBigStarCount = static_cast<int>(v); });
    kSmallStarCount = static_cast<int>(mod->getSettingValue<int64_t>("small-star-count"));
    listenForSettingChanges<int64_t>("small-star-count", [](int64_t v) { kSmallStarCount = static_cast<int>(v); });
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

    // Sandevistan trail
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
    kSandevistanTrailLayerZOrder = static_cast<int>(mod->getSettingValue<int64_t>("sandevistan-trail-layer-z-order"));
    listenForSettingChanges<int64_t>("sandevistan-trail-layer-z-order", [](int64_t v) { kSandevistanTrailLayerZOrder = static_cast<int>(v); });
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

    // Fire aura intensity
    kMinFireAuraSpeedPx = static_cast<float>(mod->getSettingValue<double>("min-fire-aura-speed-px"));
    listenForSettingChanges<double>("min-fire-aura-speed-px", [](double v) { kMinFireAuraSpeedPx = static_cast<float>(v); });
    kMaxFireAuraSpeedPx = static_cast<float>(mod->getSettingValue<double>("max-fire-aura-speed-px"));
    listenForSettingChanges<double>("max-fire-aura-speed-px", [](double v) { kMaxFireAuraSpeedPx = static_cast<float>(v); });
    kFireAuraDiameterScale = static_cast<float>(mod->getSettingValue<double>("fire-aura-diameter-scale"));
    listenForSettingChanges<double>("fire-aura-diameter-scale", [](double v) { kFireAuraDiameterScale = static_cast<float>(v); });
    kFireAuraVelocityToShader = static_cast<float>(mod->getSettingValue<double>("fire-aura-velocity-to-shader"));
    listenForSettingChanges<double>("fire-aura-velocity-to-shader", [](double v) { kFireAuraVelocityToShader = static_cast<float>(v); });

    // FireAuraSprite default uniforms
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

    // Object motion blur
    kPlayerMinBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("player-min-blur-speed-px"));
    listenForSettingChanges<double>("player-min-blur-speed-px", [](double v) { kPlayerMinBlurSpeedPx = static_cast<float>(v); });
    kPlayerMaxBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("player-max-blur-speed-px"));
    listenForSettingChanges<double>("player-max-blur-speed-px", [](double v) { kPlayerMaxBlurSpeedPx = static_cast<float>(v); });
    kPlayerBlurUvSpread = static_cast<float>(mod->getSettingValue<double>("player-blur-uv-spread"));
    listenForSettingChanges<double>("player-blur-uv-spread", [](double v) { kPlayerBlurUvSpread = static_cast<float>(v); });
    kPlayerBlurStepDivisor = static_cast<int>(mod->getSettingValue<int64_t>("player-blur-step-divisor"));
    listenForSettingChanges<int64_t>("player-blur-step-divisor", [](int64_t v) { kPlayerBlurStepDivisor = static_cast<int>(v); });
    kPlayerKeepBaseVisible = mod->getSettingValue<bool>("player-keep-base-visible");
    listenForSettingChanges<bool>("player-keep-base-visible", [](bool v) { kPlayerKeepBaseVisible = v; });
    kMenuMinBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("menu-min-blur-speed-px"));
    listenForSettingChanges<double>("menu-min-blur-speed-px", [](double v) { kMenuMinBlurSpeedPx = static_cast<float>(v); });
    kMenuMaxBlurSpeedPx = static_cast<float>(mod->getSettingValue<double>("menu-max-blur-speed-px"));
    listenForSettingChanges<double>("menu-max-blur-speed-px", [](double v) { kMenuMaxBlurSpeedPx = static_cast<float>(v); });
    kMenuBlurUvSpread = static_cast<float>(mod->getSettingValue<double>("menu-blur-uv-spread"));
    listenForSettingChanges<double>("menu-blur-uv-spread", [](double v) { kMenuBlurUvSpread = static_cast<float>(v); });
    kMenuBlurStepDivisor = static_cast<int>(mod->getSettingValue<int64_t>("menu-blur-step-divisor"));
    listenForSettingChanges<int64_t>("menu-blur-step-divisor", [](int64_t v) { kMenuBlurStepDivisor = static_cast<int>(v); });
    kMenuKeepBaseVisible = mod->getSettingValue<bool>("menu-keep-base-visible");
    listenForSettingChanges<bool>("menu-keep-base-visible", [](bool v) { kMenuKeepBaseVisible = v; });

    // Debug overlay
    kDebugLabelMarginX = static_cast<float>(mod->getSettingValue<double>("debug-label-margin-x"));
    listenForSettingChanges<double>("debug-label-margin-x", [](double v) { kDebugLabelMarginX = static_cast<float>(v); });
    kDebugLabelMarginY = static_cast<float>(mod->getSettingValue<double>("debug-label-margin-y"));
    listenForSettingChanges<double>("debug-label-margin-y", [](double v) { kDebugLabelMarginY = static_cast<float>(v); });
    kDebugLabelFontScale = static_cast<float>(mod->getSettingValue<double>("debug-label-font-scale"));
    listenForSettingChanges<double>("debug-label-font-scale", [](double v) { kDebugLabelFontScale = static_cast<float>(v); });
    kDebugLabelBoxPadX = static_cast<float>(mod->getSettingValue<double>("debug-label-box-pad-x"));
    listenForSettingChanges<double>("debug-label-box-pad-x", [](double v) { kDebugLabelBoxPadX = static_cast<float>(v); });
    kDebugLabelBoxPadY = static_cast<float>(mod->getSettingValue<double>("debug-label-box-pad-y"));
    listenForSettingChanges<double>("debug-label-box-pad-y", [](double v) { kDebugLabelBoxPadY = static_cast<float>(v); });
    kDebugLabelBoxColorR = static_cast<float>(mod->getSettingValue<double>("debug-label-box-color-r"));
    listenForSettingChanges<double>("debug-label-box-color-r", [](double v) { kDebugLabelBoxColorR = static_cast<float>(v); });
    kDebugLabelBoxColorG = static_cast<float>(mod->getSettingValue<double>("debug-label-box-color-g"));
    listenForSettingChanges<double>("debug-label-box-color-g", [](double v) { kDebugLabelBoxColorG = static_cast<float>(v); });
    kDebugLabelBoxColorB = static_cast<float>(mod->getSettingValue<double>("debug-label-box-color-b"));
    listenForSettingChanges<double>("debug-label-box-color-b", [](double v) { kDebugLabelBoxColorB = static_cast<float>(v); });
    kDebugLabelBoxAlpha = static_cast<float>(mod->getSettingValue<double>("debug-label-box-alpha"));
    listenForSettingChanges<double>("debug-label-box-alpha", [](double v) { kDebugLabelBoxAlpha = static_cast<float>(v); });

    // Physics menu panel defaults
    kPanelDefaultWFrac = static_cast<float>(mod->getSettingValue<double>("panel-default-w-frac"));
    listenForSettingChanges<double>("panel-default-w-frac", [](double v) { kPanelDefaultWFrac = static_cast<float>(v); });
    kPanelDefaultHFrac = static_cast<float>(mod->getSettingValue<double>("panel-default-h-frac"));
    listenForSettingChanges<double>("panel-default-h-frac", [](double v) { kPanelDefaultHFrac = static_cast<float>(v); });
    kPanelDefaultXFrac = static_cast<float>(mod->getSettingValue<double>("panel-default-x-frac"));
    listenForSettingChanges<double>("panel-default-x-frac", [](double v) { kPanelDefaultXFrac = static_cast<float>(v); });
    kPanelDefaultYFrac = static_cast<float>(mod->getSettingValue<double>("panel-default-y-frac"));
    listenForSettingChanges<double>("panel-default-y-frac", [](double v) { kPanelDefaultYFrac = static_cast<float>(v); });

    // Fake menu shard simulation (two triangles per grid cell)
    kMenuShardLaunchSpeedMinPx = static_cast<float>(mod->getSettingValue<double>("menu-shard-launch-speed-min-px"));
    listenForSettingChanges<double>("menu-shard-launch-speed-min-px", [](double v) { kMenuShardLaunchSpeedMinPx = static_cast<float>(v); });
    kMenuShardLaunchSpeedMaxPx = static_cast<float>(mod->getSettingValue<double>("menu-shard-launch-speed-max-px"));
    listenForSettingChanges<double>("menu-shard-launch-speed-max-px", [](double v) { kMenuShardLaunchSpeedMaxPx = static_cast<float>(v); });
    kMenuShardExtraImpactVelocityScale = static_cast<float>(mod->getSettingValue<double>("menu-shard-extra-impact-velocity-scale"));
    listenForSettingChanges<double>("menu-shard-extra-impact-velocity-scale", [](double v) { kMenuShardExtraImpactVelocityScale = static_cast<float>(v); });
    kMenuShardAngularVelocityMin = static_cast<float>(mod->getSettingValue<double>("menu-shard-angular-velocity-min"));
    listenForSettingChanges<double>("menu-shard-angular-velocity-min", [](double v) { kMenuShardAngularVelocityMin = static_cast<float>(v); });
    kMenuShardAngularVelocityMax = static_cast<float>(mod->getSettingValue<double>("menu-shard-angular-velocity-max"));
    listenForSettingChanges<double>("menu-shard-angular-velocity-max", [](double v) { kMenuShardAngularVelocityMax = static_cast<float>(v); });
    kMenuShardLinearDampingPerSecond = static_cast<float>(mod->getSettingValue<double>("menu-shard-linear-damping-per-second"));
    listenForSettingChanges<double>("menu-shard-linear-damping-per-second", [](double v) { kMenuShardLinearDampingPerSecond = static_cast<float>(v); });
    kMenuShardAngularDampingPerSecond = static_cast<float>(mod->getSettingValue<double>("menu-shard-angular-damping-per-second"));
    listenForSettingChanges<double>("menu-shard-angular-damping-per-second", [](double v) { kMenuShardAngularDampingPerSecond = static_cast<float>(v); });
    kMenuShardHoldSeconds = static_cast<float>(mod->getSettingValue<double>("menu-shard-hold-seconds"));
    listenForSettingChanges<double>("menu-shard-hold-seconds", [](double v) { kMenuShardHoldSeconds = static_cast<float>(v); });
    kMenuShardFadeSeconds = static_cast<float>(mod->getSettingValue<double>("menu-shard-fade-seconds"));
    listenForSettingChanges<double>("menu-shard-fade-seconds", [](double v) { kMenuShardFadeSeconds = static_cast<float>(v); });

    // World physics
    kPixelsPerMeter = std::max(static_cast<float>(static_cast<float>(mod->getSettingValue<double>("pixels-per-meter"))), 1.0f);
    listenForSettingChanges<double>("pixels-per-meter", [](double v) { kPixelsPerMeter = std::max(static_cast<float>(v), 1.0f); });
    kEarthGravity = static_cast<float>(mod->getSettingValue<double>("earth-gravity"));
    listenForSettingChanges<double>("earth-gravity", [](double v) { kEarthGravity = static_cast<float>(v); });
    kGravityScale = static_cast<float>(mod->getSettingValue<double>("gravity-scale"));
    listenForSettingChanges<double>("gravity-scale", [](double v) { kGravityScale = static_cast<float>(v); });
    kWorldIterations = static_cast<int>(mod->getSettingValue<int64_t>("world-iterations"));
    listenForSettingChanges<int64_t>("world-iterations", [](int64_t v) { kWorldIterations = static_cast<int>(v); });
    kWallHalfThickness = static_cast<float>(mod->getSettingValue<double>("wall-half-thickness"));
    listenForSettingChanges<double>("wall-half-thickness", [](double v) { kWallHalfThickness = static_cast<float>(v); });
    kWallLengthPadding = static_cast<float>(mod->getSettingValue<double>("wall-length-padding"));
    listenForSettingChanges<double>("wall-length-padding", [](double v) { kWallLengthPadding = static_cast<float>(v); });
    kWallThickness = static_cast<float>(mod->getSettingValue<double>("wall-thickness"));
    listenForSettingChanges<double>("wall-thickness", [](double v) { kWallThickness = static_cast<float>(v); });
    kArenaCenterFrac = static_cast<float>(mod->getSettingValue<double>("arena-center-frac"));
    listenForSettingChanges<double>("arena-center-frac", [](double v) { kArenaCenterFrac = static_cast<float>(v); });
    kPlayerDensity = static_cast<float>(mod->getSettingValue<double>("player-density"));
    listenForSettingChanges<double>("player-density", [](double v) { kPlayerDensity = static_cast<float>(v); });
    kPlayerInitialXFrac = static_cast<float>(mod->getSettingValue<double>("player-initial-x-frac"));
    listenForSettingChanges<double>("player-initial-x-frac", [](double v) { kPlayerInitialXFrac = static_cast<float>(v); });
    kPlayerInitialYFrac = static_cast<float>(mod->getSettingValue<double>("player-initial-y-frac"));
    listenForSettingChanges<double>("player-initial-y-frac", [](double v) { kPlayerInitialYFrac = static_cast<float>(v); });
    kPlayerInitialVelX = static_cast<float>(mod->getSettingValue<double>("player-initial-vel-x"));
    listenForSettingChanges<double>("player-initial-vel-x", [](double v) { kPlayerInitialVelX = static_cast<float>(v); });
    kPlayerInitialVelY = static_cast<float>(mod->getSettingValue<double>("player-initial-vel-y"));
    listenForSettingChanges<double>("player-initial-vel-y", [](double v) { kPlayerInitialVelY = static_cast<float>(v); });
    kPlayerInitialAngularVel = static_cast<float>(mod->getSettingValue<double>("player-initial-angular-vel"));
    listenForSettingChanges<double>("player-initial-angular-vel", [](double v) { kPlayerInitialAngularVel = static_cast<float>(v); });
    kPlayerFriction = static_cast<float>(mod->getSettingValue<double>("player-friction"));
    listenForSettingChanges<double>("player-friction", [](double v) { kPlayerFriction = static_cast<float>(v); });
    kDragSpring = static_cast<float>(mod->getSettingValue<double>("drag-spring"));
    listenForSettingChanges<double>("drag-spring", [](double v) { kDragSpring = static_cast<float>(v); });
    kDragDamping = static_cast<float>(mod->getSettingValue<double>("drag-damping"));
    listenForSettingChanges<double>("drag-damping", [](double v) { kDragDamping = static_cast<float>(v); });
    kDragAngularDamping = static_cast<float>(mod->getSettingValue<double>("drag-angular-damping"));
    listenForSettingChanges<double>("drag-angular-damping", [](double v) { kDragAngularDamping = static_cast<float>(v); });
    kDefaultDragTargetXFrac = static_cast<float>(mod->getSettingValue<double>("default-drag-target-x-frac"));
    listenForSettingChanges<double>("default-drag-target-x-frac", [](double v) { kDefaultDragTargetXFrac = static_cast<float>(v); });
    kDefaultDragTargetYFrac = static_cast<float>(mod->getSettingValue<double>("default-drag-target-y-frac"));
    listenForSettingChanges<double>("default-drag-target-y-frac", [](double v) { kDefaultDragTargetYFrac = static_cast<float>(v); });
    kPanelDragSpring = static_cast<float>(mod->getSettingValue<double>("panel-drag-spring"));
    listenForSettingChanges<double>("panel-drag-spring", [](double v) { kPanelDragSpring = static_cast<float>(v); });
    kPanelDragDamping = static_cast<float>(mod->getSettingValue<double>("panel-drag-damping"));
    listenForSettingChanges<double>("panel-drag-damping", [](double v) { kPanelDragDamping = static_cast<float>(v); });
    kPanelDragAngularDamping = static_cast<float>(mod->getSettingValue<double>("panel-drag-angular-damping"));
    listenForSettingChanges<double>("panel-drag-angular-damping", [](double v) { kPanelDragAngularDamping = static_cast<float>(v); });
    kOutsideBarrierSlack = static_cast<float>(mod->getSettingValue<double>("outside-barrier-slack"));
    listenForSettingChanges<double>("outside-barrier-slack", [](double v) { kOutsideBarrierSlack = static_cast<float>(v); });
    kPanelDensity = static_cast<float>(mod->getSettingValue<double>("panel-density"));
    listenForSettingChanges<double>("panel-density", [](double v) { kPanelDensity = static_cast<float>(v); });
    kPanelFriction = static_cast<float>(mod->getSettingValue<double>("panel-friction"));
    listenForSettingChanges<double>("panel-friction", [](double v) { kPanelFriction = static_cast<float>(v); });

    // Soggy dynamic object
    kSoggyFrameCount = static_cast<int>(mod->getSettingValue<int64_t>("soggy-frame-count"));
    listenForSettingChanges<int64_t>("soggy-frame-count", [](int64_t v) { kSoggyFrameCount = static_cast<int>(v); });
    kSoggyHitboxSizePx = static_cast<float>(mod->getSettingValue<double>("soggy-hitbox-size-px"));
    listenForSettingChanges<double>("soggy-hitbox-size-px", [](double v) { kSoggyHitboxSizePx = static_cast<float>(v); });
    kSoggySpawnXFrac = static_cast<float>(mod->getSettingValue<double>("soggy-spawn-x-frac"));
    listenForSettingChanges<double>("soggy-spawn-x-frac", [](double v) { kSoggySpawnXFrac = static_cast<float>(v); });
    kSoggySpawnYFrac = static_cast<float>(mod->getSettingValue<double>("soggy-spawn-y-frac"));
    listenForSettingChanges<double>("soggy-spawn-y-frac", [](double v) { kSoggySpawnYFrac = static_cast<float>(v); });
    kSoggyDensity = static_cast<float>(mod->getSettingValue<double>("soggy-density"));
    listenForSettingChanges<double>("soggy-density", [](double v) { kSoggyDensity = static_cast<float>(v); });
    kSoggyFriction = static_cast<float>(mod->getSettingValue<double>("soggy-friction"));
    listenForSettingChanges<double>("soggy-friction", [](double v) { kSoggyFriction = static_cast<float>(v); });
    kSoggyAnimFps = static_cast<float>(mod->getSettingValue<double>("soggy-anim-fps"));
    listenForSettingChanges<double>("soggy-anim-fps", [](double v) { kSoggyAnimFps = static_cast<float>(v); });
    kSoggyGrabRadiusFraction = static_cast<float>(mod->getSettingValue<double>("soggy-grab-radius-fraction"));
    listenForSettingChanges<double>("soggy-grab-radius-fraction", [](double v) { kSoggyGrabRadiusFraction = static_cast<float>(v); });

    // Click and gestures
    kPendingDoubleActionTag = static_cast<int>(mod->getSettingValue<int64_t>("pending-double-action-tag"));
    listenForSettingChanges<int64_t>("pending-double-action-tag", [](int64_t v) { kPendingDoubleActionTag = static_cast<int>(v); });

    // Input began vs ended tracking slop
    kTapSlopPx = static_cast<float>(mod->getSettingValue<double>("tap-slop-px"));
    listenForSettingChanges<double>("tap-slop-px", [](double v) { kTapSlopPx = static_cast<float>(v); });
    kTrackResidualSlopPx = static_cast<float>(mod->getSettingValue<double>("track-residual-slop-px"));
    listenForSettingChanges<double>("track-residual-slop-px", [](double v) { kTrackResidualSlopPx = static_cast<float>(v); });
    kMaxFingerForTrackTapPx = static_cast<float>(mod->getSettingValue<double>("max-finger-for-track-tap-px"));
    listenForSettingChanges<double>("max-finger-for-track-tap-px", [](double v) { kMaxFingerForTrackTapPx = static_cast<float>(v); });
    kMaxTapGestureSec = static_cast<double>(mod->getSettingValue<double>("max-tap-gesture-sec"));
    listenForSettingChanges<double>("max-tap-gesture-sec", [](double v) { kMaxTapGestureSec = static_cast<double>(v); });

    // Physics menu UI chrome
    kPhysicsMenuButtonSpacingPx = static_cast<float>(mod->getSettingValue<double>("physics-menu-button-spacing-px"));
    listenForSettingChanges<double>("physics-menu-button-spacing-px", [](double v) { kPhysicsMenuButtonSpacingPx = static_cast<float>(v); });
    kPhysicsMenuButtonScale = static_cast<float>(mod->getSettingValue<double>("physics-menu-button-scale"));
    listenForSettingChanges<double>("physics-menu-button-scale", [](double v) { kPhysicsMenuButtonScale = static_cast<float>(v); });
    kPhysicsMenuTitleLabelScale = static_cast<float>(mod->getSettingValue<double>("physics-menu-title-label-scale"));
    listenForSettingChanges<double>("physics-menu-title-label-scale", [](double v) { kPhysicsMenuTitleLabelScale = static_cast<float>(v); });
    kPhysicsMenuTitleTopInset = static_cast<float>(mod->getSettingValue<double>("physics-menu-title-top-inset"));
    listenForSettingChanges<double>("physics-menu-title-top-inset", [](double v) { kPhysicsMenuTitleTopInset = static_cast<float>(v); });
    kPhysicsMenuMenuYFrac = static_cast<float>(mod->getSettingValue<double>("physics-menu-menu-y-frac"));
    listenForSettingChanges<double>("physics-menu-menu-y-frac", [](double v) { kPhysicsMenuMenuYFrac = static_cast<float>(v); });
    kPhysicsMenuPopupOpacity = static_cast<float>(mod->getSettingValue<double>("physics-menu-popup-opacity"));
    listenForSettingChanges<double>("physics-menu-popup-opacity", [](double v) { kPhysicsMenuPopupOpacity = static_cast<float>(v); });
    kScreenShakeActionTag = static_cast<int>(mod->getSettingValue<int64_t>("screen-shake-action-tag"));
    listenForSettingChanges<int64_t>("screen-shake-action-tag", [](int64_t v) { kScreenShakeActionTag = static_cast<int>(v); });

    // Box2D-Lite polygon vertices
    kB2RestitutionInSpeedThreshold = static_cast<float>(mod->getSettingValue<double>("b2-restitution-in-speed-threshold"));
    listenForSettingChanges<double>("b2-restitution-in-speed-threshold", [](double v) { kB2RestitutionInSpeedThreshold = static_cast<float>(v); });
    kB2RestitutionCoefficient = static_cast<float>(mod->getSettingValue<double>("b2-restitution-coefficient"));
    listenForSettingChanges<double>("b2-restitution-coefficient", [](double v) { kB2RestitutionCoefficient = static_cast<float>(v); });
    kB2CollideReferenceEdgeRelativeTol = static_cast<float>(mod->getSettingValue<double>("b2-collide-reference-edge-relative-tol"));
    listenForSettingChanges<double>("b2-collide-reference-edge-relative-tol", [](double v) { kB2CollideReferenceEdgeRelativeTol = static_cast<float>(v); });
    kB2CollideReferenceEdgeAbsoluteTol = static_cast<float>(mod->getSettingValue<double>("b2-collide-reference-edge-absolute-tol"));
    listenForSettingChanges<double>("b2-collide-reference-edge-absolute-tol", [](double v) { kB2CollideReferenceEdgeAbsoluteTol = static_cast<float>(v); });
}

} // namespace mod_settings
