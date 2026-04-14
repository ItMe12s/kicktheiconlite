#pragma once

#include <numbers>

constexpr int kPhysicsOverlayZOrder = 1000;
constexpr int kPhysicsOverlayTouchPriority = -6767; // CCMenu uses -128.
constexpr int kPhysicsOverlaySchedulerPriority = 0;

constexpr float kGrabRadiusFraction = 2.0f / 3.0f;

constexpr float kImpactHitstopSeconds = 0.15f;
constexpr float kImpactFlashTotalSeconds = 0.15f;
constexpr float kImpactFlashPhaseSeconds = 0.05f;
constexpr float kImpactFlashCooldownSeconds = 0.7f;
constexpr float kWallShakeDuration = 0.25f;

constexpr int kStarBurstMaxPhaseIndex =
    static_cast<int>(kImpactFlashTotalSeconds / kImpactFlashPhaseSeconds) - 1;

constexpr float kImpactMinSpeed = 1400.0f;
constexpr float kMinWallShakeSpeed = 150.0f;
constexpr float kWallShakeSpeedToStrength = 0.0025f;
constexpr float kMaxWallShakeStrength = 5.0f;

constexpr float kFixedPhysicsDt = 1.0f / 120.0f;
constexpr int kMaxPhysicsSubsteps = 16;
constexpr float kPhysicsAccumulatorCap = kFixedPhysicsDt * static_cast<float>(kMaxPhysicsSubsteps);

constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;

constexpr int kStarBurstCount = 5;

constexpr int kImpactFlashBackdropZOrder = -1;
constexpr int kImpactFlashInvertPhaseEndPhaseCount = 2;

constexpr int kStarBurstZOrder = 4;
constexpr int kBigStarCount = 2;
constexpr int kSmallStarCount = 3;

constexpr float kBigStarRadiusMin = 0.05f;
constexpr float kBigStarRadiusMax = 0.3f;

constexpr float kSmallStarRadiusMin = 0.2f;
constexpr float kSmallStarRadiusMax = 0.6f;

constexpr float kBigStarScreenFrac = 0.9f;
constexpr float kSmallStarScreenFrac = 0.2f;

constexpr float kStarScaleVariance = 0.15f;

constexpr float kMinFireAuraSpeedPx = 600.0f;
constexpr float kMaxFireAuraSpeedPx = 1200.0f;
constexpr float kFireAuraDiameterScale = 4.0f;
constexpr float kFireAuraVelocityToShader = 0.002f;

constexpr float kMinBlurSpeedPx = 8.0f;
constexpr float kMaxBlurSpeedPx = 1200.0f;
constexpr float kBlurUvSpread = 0.038f;
constexpr float kBlurCaptureScale = 2.6f;
constexpr int kBlurStepDivisor = 4;
constexpr float kMinSpeedForInverse = 1e-6f;

constexpr int kMotionBlurOverlayLocalZ = 2;
constexpr int kWhiteFlashOverlayLocalZ = 3;
constexpr int kFireAuraZOrder = 1;

constexpr int kMinCaptureTextureSize = 32;
constexpr float kBoundsCenterFrac = 0.5f;

constexpr int kScreenShakeIntervals = 10;
constexpr float kScreenShakeSampleMin = -1.0f;
constexpr float kScreenShakeSampleMax = 1.0f;
constexpr float kScreenShakeCooldownExtraSeconds = 0.1f;
