#pragma once

#include <algorithm>
#include <numbers>

// App-wide tuning (single header), box2d-lite collision/restitution/vertex cap
// included here for one place to edit, see kB2* symbols

namespace player_visual {

constexpr int kMaxWorldBoundsTreeDepth = 64;
constexpr float kMinVisualWidthPx = 1.0f;
constexpr float kPlayerTargetSizeFraction = 0.125f;
constexpr int kMinPlayerFrameId = 1;

constexpr float kPlayerRootAnchorXFrac = 0.5f;
constexpr float kPlayerRootAnchorYFrac = 0.5f;
constexpr int kPlayerVisualLocalZOrder = 0;

} // namespace player_visual

// Overlay/input ordering
constexpr int kPhysicsOverlayZOrder = 1000;
constexpr int kPhysicsOverlayTouchPriority = -6767;
constexpr int kPhysicsOverlaySchedulerPriority = 0;
constexpr int kHitProxyLocalZOrder = 6767; // Invisible touch target for ClickTracker

// Simulation and math
constexpr int kPlayerRootZOrder = 1;
constexpr float kFixedPhysicsDt = 1.0f / 120.0f;
static_assert(kFixedPhysicsDt > 0.0f);
constexpr int kMaxPhysicsSubsteps = 16;
// Clamp cocos scheduler dt so hitches do not explode physics accumulator before kPhysicsAccumulatorCap
constexpr float kMaxSimulationFrameDt = 1.0f / 30.0f;
static_assert(kMaxSimulationFrameDt > 0.0f);
constexpr float kPhysicsAccumulatorCap = kFixedPhysicsDt * static_cast<float>(kMaxPhysicsSubsteps);
static_assert(kPhysicsAccumulatorCap > 0.0f);
// One scheduler frame may add at most one capped step worth into the bucket (worst hitch still bounded)
static_assert(kMaxSimulationFrameDt <= kPhysicsAccumulatorCap);
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
constexpr int kLayerUiZOrderOffset = 2;

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
constexpr float kPanelShatterMinImpactSpeed = 2200.0f;
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

// Fire aura intensity (shader input scaling)
constexpr float kMinFireAuraSpeedPx = 600.0f;
constexpr float kMaxFireAuraSpeedPx = 2800.0f;
constexpr float kFireAuraDiameterScale = 2.25f;
constexpr float kFireAuraVelocityToShader = 0.002f;
// FireAuraSprite default uniforms (RGB 0..1 before setFireColors)
constexpr float kFireAuraDefaultPrimaryR = 1.0f;
constexpr float kFireAuraDefaultPrimaryG = 0.9f;
constexpr float kFireAuraDefaultPrimaryB = 0.5f;
constexpr float kFireAuraDefaultSecondaryR = 0.32f;
constexpr float kFireAuraDefaultSecondaryG = 0.02f;
constexpr float kFireAuraDefaultSecondaryB = 0.0f;

// Object motion blur tuning
constexpr float kPlayerMinBlurSpeedPx = 120.0f;
constexpr float kPlayerMaxBlurSpeedPx = 3200.0f;
constexpr float kPlayerBlurUvSpread = 0.035f;
constexpr int kPlayerBlurStepDivisor = 6;
constexpr bool kPlayerKeepBaseVisible = true;
constexpr float kMenuMinBlurSpeedPx = 180.0f;
constexpr float kMenuMaxBlurSpeedPx = 1800.0f;
constexpr float kMenuBlurUvSpread = 0.01f;
constexpr int kMenuBlurStepDivisor = 4;
constexpr bool kMenuKeepBaseVisible = false;

// Debug overlay
constexpr float kDebugLabelMarginX = 4.0f;
constexpr float kDebugLabelMarginY = 4.0f;
constexpr int kDebugLabelZOrder = 6767;
constexpr int kDebugLabelBackgroundZOrder = kDebugLabelZOrder - 1;
constexpr float kDebugLabelUpdateHz = 20.0f;
constexpr float kDebugLabelUpdateInterval = 1.0f / kDebugLabelUpdateHz;
constexpr float kDebugLabelFontScale = 0.5f;
constexpr float kDebugLabelBoxPadX = 1.0f;
constexpr float kDebugLabelBoxPadY = 0.0f;
constexpr float kDebugLabelBoxColorR = 0.0f;
constexpr float kDebugLabelBoxColorG = 0.0f;
constexpr float kDebugLabelBoxColorB = 0.0f;
constexpr float kDebugLabelBoxAlpha = 0.35f;

// Physics menu panel defaults
constexpr float kPanelDefaultWFrac = 0.35f;
constexpr float kPanelDefaultHFrac = 0.25f;
constexpr float kPanelDefaultXFrac = 0.5f;
constexpr float kPanelDefaultYFrac = 0.7f;

// Fake menu shard simulation (two triangles per grid cell)
constexpr int kMenuShardRows = 4;
constexpr int kMenuShardCols = 6;
constexpr int kMenuShardTrianglesTotal = kMenuShardRows * kMenuShardCols * 2;
constexpr float kMenuShardLaunchSpeedMinPx = 343.0f;
constexpr float kMenuShardLaunchSpeedMaxPx = 767.0f;
constexpr float kMenuShardExtraImpactVelocityScale = 0.35f;
constexpr float kMenuShardAngularVelocityMin = 15.0f;
constexpr float kMenuShardAngularVelocityMax = 67.0f;
constexpr float kMenuShardHoldSeconds = 12.0f;
constexpr float kMenuShardFadeSeconds = 3.0f;

// Physics world (box2d-lite integration)
constexpr float kPixelsPerMeter = 50.0f;

constexpr float kEarthGravity = 9.8f;
constexpr float kGravityScale = 1.75f;
constexpr int kWorldIterations = 10;

constexpr float kWallHalfThickness = 0.5f;
constexpr float kWallLengthPadding = 4.0f;
constexpr float kWallThickness = 1.0f;
constexpr float kArenaCenterFrac = 0.5f;

constexpr float kPlayerDensity = 1.0f;
constexpr float kPlayerInitialXFrac = 0.25f;
constexpr float kPlayerInitialYFrac = 0.25f;

constexpr float kPlayerInitialVelX = 5.0f;
constexpr float kPlayerInitialVelY = 10.0f;
constexpr float kPlayerInitialAngularVel = 30.0f;
constexpr float kPlayerFriction = 0.4f;

constexpr float kDragSpring = 200.0f;
constexpr float kDragDamping = 10.0f;
constexpr float kDragAngularDamping = 0.2f;
constexpr float kDefaultDragTargetXFrac = 0.5f;
constexpr float kDefaultDragTargetYFrac = 0.5f;

constexpr float kPanelDragSpring = 170.0f;
constexpr float kPanelDragDamping = 15.0f;
constexpr float kPanelDragAngularDamping = 0.3f;

constexpr float kOutsideBarrierSlack = 1.2f;

constexpr float kPanelDensity = 1.25f;
constexpr float kPanelFriction = 0.6f;

// Click / gesture (ClickTracker)
constexpr double kClickWindowSec = 0.5;
// Minimum delay before scheduling a double-commit, actual schedule uses kDoubleClickScheduledDelaySec
constexpr double kDoubleTapMinCommitDelaySec = 0.15;
// Scheduled delay before DoubleClickEvent fires, must be >= kClickWindowSec so a third tap can still land in the chain window
constexpr double kDoubleClickScheduledDelaySec =
    std::max(kDoubleTapMinCommitDelaySec, kClickWindowSec);

constexpr int kPendingDoubleActionTag = 0x434C4B44; // Means CLKD, deferred double click

// Began vs Ended
constexpr float kTapSlopPx = 12.0f;
// When the icon moves, the finger often moves with it
constexpr float kTrackResidualSlopPx = 20.0f;
// Residual path is only for short gestures, long drags exceed this even if residual is small
constexpr float kMaxFingerForTrackTapPx = 48.0f;
constexpr double kMaxTapGestureSec = 0.24;

// Physics menu UI chrome (PhysicsMenu::build)
constexpr float kPhysicsMenuButtonSpacingPx = 8.0f;
constexpr float kPhysicsMenuButtonScale = 0.55f;
constexpr float kPhysicsMenuTitleLabelScale = 0.5f;
constexpr float kPhysicsMenuTitleTopInset = 14.0f;
constexpr float kPhysicsMenuMenuYFrac = 0.45f;
constexpr float kPhysicsMenuPopupOpacity = 192.0f;

// OverlayRendering: CCAction tag for screen shake sequences
constexpr int kScreenShakeActionTag = 0x6B53484B; // "kSHK"

// box2d-lite
constexpr int kB2MaxPolygonVertices = 16;
constexpr float kB2RestitutionInSpeedThreshold = 3.0f;
constexpr float kB2RestitutionCoefficient = 0.67f;
constexpr float kB2CollideReferenceEdgeRelativeTol = 0.98f;
constexpr float kB2CollideReferenceEdgeAbsoluteTol = 0.001f;
