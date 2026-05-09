#include "ModTuning.h"

bool kHideModOverlay = false;

namespace player_visual {

int kMaxWorldBoundsTreeDepth = 64;
float kMinVisualWidthPx = 1.0f;
float kPlayerTargetSizeFraction = 0.125f;
int kMinPlayerFrameId = 1;

float kPlayerRootAnchorXFrac = 0.5f;
float kPlayerRootAnchorYFrac = 0.5f;
int kPlayerVisualLocalZOrder = 0;

} // namespace player_visual

int kPhysicsOverlayZOrder = 1000;
int kPhysicsOverlayTouchPriority = -6767;
int kPhysicsOverlaySchedulerPriority = 0;

int kPlayerRootZOrder = 1;
float kMinSpeedForInverse = 1e-6f;
float kGrabRadiusFraction = 2.0f / 3.0f;

int kImpactFlashBackdropZOrder = -3;
int kFireAuraZOrder = -1;
int kStarBurstZOrder = 0;
int kUnifiedWorldCaptureZOrder = 1;
int kUnifiedBlurCompositeZOrder = 2;
int kImpactNoiseZOrder = 3;
int kGlobalStartBurstZOrder = 4;
int kLayerTrailZOrderOffset = -1;
int kLayerWorldZOrderOffset = 0;
int kLayerUiZOrderOffset = 2;

bool kEnablePlayerImpactTrail = true;
bool kEnablePlayerImpactFlashStack = true;

float kImpactHitstopSeconds = 0.15f;
float kImpactFlashCooldownSeconds = 0.4f;
int kImpactFlashInvertPhaseEndPhaseCount = 2;

float kPlayerImpactMinFlashSpeed = 1600.0f;
float kPlayerImpactMinTrailSpeed = 300.0f;

float kImpactNoiseFadeSeconds = 1.75f;
float kImpactNoiseStackedImpactTimeSkip = 73.0f;
bool kImpactNoiseCompositeNearestFilter = true;
float kImpactNoiseRenderScale = 0.1f;

int kBigStarCount = 2;
int kSmallStarCount = 3;
float kBigStarRadiusMin = 0.05f;
float kBigStarRadiusMax = 0.3f;
float kSmallStarRadiusMin = 0.2f;
float kSmallStarRadiusMax = 0.6f;
float kBigStarScreenFrac = 0.9f;
float kSmallStarScreenFrac = 0.2f;
float kStarScaleVariance = 0.15f;

float kSandevistanEndSpeedPx = 200.0f;
float kSandevistanSpawnIntervalSec = 0.04f;
float kSandevistanGhostFadeSec = 0.4f;
int kSandevistanGhostStartOpacity = 128;
int kSandevistanMaxConcurrentGhosts = 24;
int kSandevistanTrailLayerZOrder = 0;
int kSandevistanTrailHueOrangeR = 255;
int kSandevistanTrailHueOrangeG = 175;
int kSandevistanTrailHueOrangeB = 55;
int kSandevistanTrailHuePurpleR = 185;
int kSandevistanTrailHuePurpleG = 95;
int kSandevistanTrailHuePurpleB = 255;
int kSandevistanTrailHueCyanR = 80;
int kSandevistanTrailHueCyanG = 230;
int kSandevistanTrailHueCyanB = 255;

float kMinFireAuraSpeedPx = 600.0f;
float kMaxFireAuraSpeedPx = 2800.0f;
float kFireAuraDiameterScale = 2.25f;
float kFireAuraVelocityToShader = 0.002f;

float kFireAuraDefaultPrimaryR = 1.0f;
float kFireAuraDefaultPrimaryG = 0.9f;
float kFireAuraDefaultPrimaryB = 0.5f;
float kFireAuraDefaultSecondaryR = 0.32f;
float kFireAuraDefaultSecondaryG = 0.02f;
float kFireAuraDefaultSecondaryB = 0.0f;

float kPlayerMinBlurSpeedPx = 120.0f;
float kPlayerMaxBlurSpeedPx = 3200.0f;
float kPlayerBlurUvSpread = 0.035f;
int kPlayerBlurStepDivisor = 6;
bool kPlayerKeepBaseVisible = true;

bool kDebugLabelEnabled = false;
float kDebugLabelMarginX = 4.0f;
float kDebugLabelMarginY = 4.0f;
float kDebugLabelFontScale = 0.5f;
float kDebugLabelBoxPadX = 1.0f;
float kDebugLabelBoxPadY = 0.0f;
float kDebugLabelBoxColorR = 0.0f;
float kDebugLabelBoxColorG = 0.0f;
float kDebugLabelBoxColorB = 0.0f;
float kDebugLabelBoxAlpha = 0.35f;

float kPixelsPerMeter = 50.0f;

float kEarthGravity = 9.8f;
float kGravityScale = 1.75f;
int kWorldIterations = 10;

float kWallHalfThickness = 0.5f;
float kWallLengthPadding = 4.0f;
float kWallThickness = 1.0f;
float kArenaCenterFrac = 0.5f;

float kPlayerDensity = 1.0f;
float kPlayerInitialXFrac = 0.25f;
float kPlayerInitialYFrac = 0.25f;

float kPlayerInitialVelX = 5.0f;
float kPlayerInitialVelY = 10.0f;
float kPlayerInitialAngularVel = 30.0f;

float kPlayerFriction = 0.4f;
float kDragSpring = 220.0f;
float kDragDamping = 10.0f;
float kDragAngularDamping = 0.2f;
float kDefaultDragTargetXFrac = 0.5f;
float kDefaultDragTargetYFrac = 0.5f;

float kOutsideBarrierSlack = 1.2f;

float kB2RestitutionInSpeedThreshold = 3.0f;

float kB2RestitutionCoefficient = 0.67f;

float kB2CollideReferenceEdgeRelativeTol = 0.98f;
float kB2CollideReferenceEdgeAbsoluteTol = 0.001f;
