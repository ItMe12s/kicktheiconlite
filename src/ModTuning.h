#pragma once

#include <numbers>

// App-wide tuning (single header), box2d-lite collision/restitution/vertex cap
// included here for one place to edit, see kB2* symbols
//
// SPLIT: constexpr = compile-time-required (array sizes, static_asserts, derived)
//        extern    = runtime-tunable definitions in ModTuning.cpp (Geode settings seed via ModSettings.cpp)

// Important (hide-mod-overlay bound in ModSettings.cpp, overlay apply in RuntimeRestart)
constexpr float kHideModOverlayOffsetX = 676767.0f;
constexpr float kHideModOverlayOffsetY = 676767.0f;
extern bool kHideModOverlay;

namespace player_visual {

// Player Visual
extern int kMaxWorldBoundsTreeDepth;
extern float kMinVisualWidthPx;
extern float kPlayerTargetSizeFraction;
extern int kMinPlayerFrameId;

extern float kPlayerRootAnchorXFrac;
extern float kPlayerRootAnchorYFrac;
extern int kPlayerVisualLocalZOrder;

} // namespace player_visual

// Physics Overlay
extern int kPhysicsOverlayZOrder;
extern int kPhysicsOverlayTouchPriority;
extern int kPhysicsOverlaySchedulerPriority;

// Player Root
extern int kPlayerRootZOrder;
constexpr float kFixedPhysicsDt = 1.0f / 120.0f;
static_assert(kFixedPhysicsDt > 0.0f);
constexpr int kMaxPhysicsSubsteps = 16;
constexpr float kMaxSimulationFrameDt = 1.0f / 30.0f;
static_assert(kMaxSimulationFrameDt > 0.0f);
constexpr float kPhysicsAccumulatorCap = kFixedPhysicsDt * static_cast<float>(kMaxPhysicsSubsteps);
static_assert(kPhysicsAccumulatorCap > 0.0f);
static_assert(kMaxSimulationFrameDt <= kPhysicsAccumulatorCap);
constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
extern float kMinSpeedForInverse;
extern float kGrabRadiusFraction;

// VFX Z Orders
extern int kImpactFlashBackdropZOrder;
extern int kFireAuraZOrder;
extern int kStarBurstZOrder;
extern int kUnifiedWorldCaptureZOrder;
extern int kUnifiedBlurCompositeZOrder;
extern int kImpactNoiseZOrder;
extern int kGlobalStartBurstZOrder;
extern int kLayerTrailZOrderOffset;
extern int kLayerWorldZOrderOffset;
extern int kLayerUiZOrderOffset;

// VFX Toggles
extern bool kEnablePlayerImpactTrail;
extern bool kEnablePlayerImpactFlashStack;

// Impact Flash
extern float kImpactHitstopSeconds;
constexpr float kImpactFlashTotalSeconds = 0.15f;
constexpr float kImpactFlashPhaseSeconds = 0.05f;
extern float kImpactFlashCooldownSeconds;
extern int kImpactFlashInvertPhaseEndPhaseCount;
constexpr int kStarBurstMaxPhaseIndex =
    static_cast<int>(kImpactFlashTotalSeconds / kImpactFlashPhaseSeconds) - 1;

// Player Impact
extern float kPlayerImpactMinFlashSpeed;
extern float kPlayerImpactMinTrailSpeed;

// Impact Noise
extern float kImpactNoiseFadeSeconds;
extern float kImpactNoiseStackedImpactTimeSkip;
extern bool kImpactNoiseCompositeNearestFilter;
extern float kImpactNoiseRenderScale;

// Star Burst
constexpr int kStarBurstSpriteSlots = 5;
extern int kBigStarCount;
extern int kSmallStarCount;
extern float kBigStarRadiusMin;
extern float kBigStarRadiusMax;
extern float kSmallStarRadiusMin;
extern float kSmallStarRadiusMax;
extern float kBigStarScreenFrac;
extern float kSmallStarScreenFrac;
extern float kStarScaleVariance;

// Sandevistan
extern float kSandevistanEndSpeedPx;
extern float kSandevistanSpawnIntervalSec;
extern float kSandevistanGhostFadeSec;
extern int kSandevistanGhostStartOpacity;
extern int kSandevistanMaxConcurrentGhosts;
extern int kSandevistanTrailLayerZOrder;
extern int kSandevistanTrailHueOrangeR;
extern int kSandevistanTrailHueOrangeG;
extern int kSandevistanTrailHueOrangeB;
extern int kSandevistanTrailHuePurpleR;
extern int kSandevistanTrailHuePurpleG;
extern int kSandevistanTrailHuePurpleB;
extern int kSandevistanTrailHueCyanR;
extern int kSandevistanTrailHueCyanG;
extern int kSandevistanTrailHueCyanB;

// Fire Aura
extern float kMinFireAuraSpeedPx;
extern float kMaxFireAuraSpeedPx;
extern float kFireAuraDiameterScale;
extern float kFireAuraVelocityToShader;

extern float kFireAuraDefaultPrimaryR;
extern float kFireAuraDefaultPrimaryG;
extern float kFireAuraDefaultPrimaryB;
extern float kFireAuraDefaultSecondaryR;
extern float kFireAuraDefaultSecondaryG;
extern float kFireAuraDefaultSecondaryB;

// Player Motion Blur
extern float kPlayerMinBlurSpeedPx;
extern float kPlayerMaxBlurSpeedPx;
extern float kPlayerBlurUvSpread;
extern int kPlayerBlurStepDivisor;
extern bool kPlayerKeepBaseVisible;

// Debug Label
extern bool kDebugLabelEnabled;
extern float kDebugLabelMarginX;
extern float kDebugLabelMarginY;
constexpr int kDebugLabelZOrder = 6767;
constexpr int kDebugLabelBackgroundZOrder = kDebugLabelZOrder - 1;
constexpr float kDebugLabelUpdateHz = 20.0f;
constexpr float kDebugLabelUpdateInterval = 1.0f / kDebugLabelUpdateHz;
extern float kDebugLabelFontScale;
extern float kDebugLabelBoxPadX;
extern float kDebugLabelBoxPadY;
extern float kDebugLabelBoxColorR;
extern float kDebugLabelBoxColorG;
extern float kDebugLabelBoxColorB;
extern float kDebugLabelBoxAlpha;

// Physics World
extern float kPixelsPerMeter;

extern float kEarthGravity;
extern float kGravityScale;
extern int kWorldIterations;

extern float kWallHalfThickness;
extern float kWallLengthPadding;
extern float kWallThickness;
extern float kArenaCenterFrac;

// Player
extern float kPlayerDensity;
extern float kPlayerInitialXFrac;
extern float kPlayerInitialYFrac;

// Player Initial
extern float kPlayerInitialVelX;
extern float kPlayerInitialVelY;
extern float kPlayerInitialAngularVel;

// Player Physics
extern float kPlayerFriction;
extern float kDragSpring;
extern float kDragDamping;
extern float kDragAngularDamping;
extern float kDefaultDragTargetXFrac;
extern float kDefaultDragTargetYFrac;

// Outside Barrier Slack
extern float kOutsideBarrierSlack;

// B2 Max Polygon Vertices
constexpr int kB2MaxPolygonVertices = 16;

// B2 Restitution In Speed Threshold
extern float kB2RestitutionInSpeedThreshold;

// B2 Restitution Coefficient
extern float kB2RestitutionCoefficient;

// B2 Collide Reference Edge Relative Tol
extern float kB2CollideReferenceEdgeRelativeTol;
extern float kB2CollideReferenceEdgeAbsoluteTol;
