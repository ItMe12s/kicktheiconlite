#pragma once

#include <numbers>

// Overlay/input ordering
constexpr int kPhysicsOverlayZOrder = 1000;
constexpr int kPhysicsOverlayTouchPriority = -6767;
constexpr int kPhysicsOverlaySchedulerPriority = 0;
constexpr int kHitProxyLocalZOrder = 6767; // Invisible touch target for ClickTracker

// Simulation and math
constexpr int kPlayerRootZOrder = 1;
constexpr float kFixedPhysicsDt = 1.0f / 120.0f;
constexpr int kMaxPhysicsSubsteps = 16;
constexpr float kPhysicsAccumulatorCap = kFixedPhysicsDt * static_cast<float>(kMaxPhysicsSubsteps);
constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
constexpr float kMinSpeedForInverse = 1e-6f;
constexpr float kGrabRadiusFraction = 2.0f / 3.0f;

// Global render layering
constexpr int kImpactFlashBackdropZOrder = -3;
constexpr int kFireAuraZOrder = -1;
constexpr int kStarBurstZOrder = 0;
constexpr int kUnifiedWorldCaptureZOrder = 1;
constexpr int kUnifiedBlurCompositeZOrder = 2;
constexpr int kImpactNoiseZOrder = 3;
constexpr int kGlobalStartBurstZOrder = 4;
constexpr int kPhysicsMenuZOrder = 5;
constexpr int kLayerTrailZOrderOffset = -1;
constexpr int kLayerWorldZOrderOffset = 0;
constexpr int kLayerUiZOrderOffset = 1;

// Impact response toggles
constexpr bool kEnablePlayerImpactTrail = true;
constexpr bool kEnablePlayerImpactFlashStack = true;
constexpr bool kEnablePanelImpactShake = true;

// Impact timing and phase
constexpr float kImpactHitstopSeconds = 0.15f;
constexpr float kImpactFlashTotalSeconds = 0.15f;
constexpr float kImpactFlashPhaseSeconds = 0.05f;
constexpr float kImpactFlashCooldownSeconds = 0.4f;
constexpr int kImpactFlashInvertPhaseEndPhaseCount = 2;
constexpr int kStarBurstMaxPhaseIndex =
    static_cast<int>(kImpactFlashTotalSeconds / kImpactFlashPhaseSeconds) - 1;

// Screen shake
constexpr float kPlayerImpactMinFlashSpeed = 1600.0f;
constexpr float kPlayerImpactMinShakeSpeed = 300.0f;
constexpr float kPlayerImpactShakeDuration = 0.25f;
constexpr float kPlayerImpactShakeSpeedToStrength = 0.005f;
constexpr float kPlayerImpactMaxShakeStrength = 6.7f;
constexpr float kPanelImpactMinShakeSpeed = 220.0f;
constexpr float kPanelImpactShakeDuration = 0.18f;
constexpr float kPanelImpactShakeSpeedToStrength = 0.0035f;
constexpr float kPanelImpactMaxShakeStrength = 3.0f;
constexpr int kScreenShakeIntervals = 10;
constexpr float kScreenShakeSampleMin = -1.0f;
constexpr float kScreenShakeSampleMax = 1.0f;
constexpr float kScreenShakeCooldownExtraSeconds = 0.1f;

// Impact noise pass
constexpr float kImpactNoiseFadeSeconds = 1.75f;
constexpr float kImpactNoiseStackedImpactTimeSkip = 73.0f;
// Shader runs at (width * scale) x (height * scale) composite sprite samples that rt
// true = GL_NEAREST, false = GL_LINEAR, Call it pixel art and not shit resolution ofc ;)
constexpr bool kImpactNoiseCompositeNearestFilter = true;
constexpr float kImpactNoiseRenderScale = 0.1f;

// Star burst composition
constexpr int kStarBurstCount = 5;
constexpr int kBigStarCount = 2;
constexpr int kSmallStarCount = 3;
constexpr float kBigStarRadiusMin = 0.05f;
constexpr float kBigStarRadiusMax = 0.3f;
constexpr float kSmallStarRadiusMin = 0.2f;
constexpr float kSmallStarRadiusMax = 0.6f;
constexpr float kBigStarScreenFrac = 0.9f;
constexpr float kSmallStarScreenFrac = 0.2f;
constexpr float kStarScaleVariance = 0.15f;

// Sandevistan trail
constexpr float kSandevistanEndSpeedPx = 200.0f;
constexpr float kSandevistanSpawnIntervalSec = 0.04f;
constexpr float kSandevistanGhostFadeSec = 0.4f;
constexpr int kSandevistanGhostStartOpacity = 128;
constexpr int kSandevistanMaxConcurrentGhosts = 24;
constexpr int kSandevistanTrailLayerZOrder = 0;
constexpr int kSandevistanTrailHueOrangeR = 255;
constexpr int kSandevistanTrailHueOrangeG = 175;
constexpr int kSandevistanTrailHueOrangeB = 55;
constexpr int kSandevistanTrailHuePurpleR = 185;
constexpr int kSandevistanTrailHuePurpleG = 95;
constexpr int kSandevistanTrailHuePurpleB = 255;
constexpr int kSandevistanTrailHueCyanR = 80;
constexpr int kSandevistanTrailHueCyanG = 230;
constexpr int kSandevistanTrailHueCyanB = 255;

// Fire aura intensity
constexpr float kMinFireAuraSpeedPx = 600.0f;
constexpr float kMaxFireAuraSpeedPx = 2800.0f;
constexpr float kFireAuraDiameterScale = 2.25f;
constexpr float kFireAuraVelocityToShader = 0.002f;

// Object motion blur tuning
constexpr float kPlayerMinBlurSpeedPx = 120.0f;
constexpr float kPlayerMaxBlurSpeedPx = 3200.0f;
constexpr float kPlayerBlurUvSpread = 0.03f;
constexpr int kPlayerBlurStepDivisor = 6;
constexpr bool kPlayerKeepBaseVisible = true;
constexpr float kMenuMinBlurSpeedPx = 180.0f;
constexpr float kMenuMaxBlurSpeedPx = 1800.0f;
constexpr float kMenuBlurUvSpread = 0.01f;
constexpr int kMenuBlurStepDivisor = 4;
constexpr bool kMenuKeepBaseVisible = false;

// Physics menu panel defaults
constexpr float kPanelDefaultWFrac = 0.35f;
constexpr float kPanelDefaultHFrac = 0.25f;
constexpr float kPanelDefaultXFrac = 0.5f;
constexpr float kPanelDefaultYFrac = 0.7f;
