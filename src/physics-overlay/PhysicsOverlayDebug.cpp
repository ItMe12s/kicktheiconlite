#include "PhysicsOverlay.h"

#include <Geode/cocos/cocoa/CCArray.h>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "PhysicsWorld.h"

using namespace geode::prelude;

namespace {

inline ccColor4F debugLabelBoxFill() {
    return ccc4f(
        kDebugLabelBoxColorR,
        kDebugLabelBoxColorG,
        kDebugLabelBoxColorB,
        kDebugLabelBoxAlpha
    );
}

inline GLubyte colorToByte(float channel) {
    float const clamped = std::max(0.0f, std::min(channel, 1.0f));
    return static_cast<GLubyte>(clamped * 255.0f);
}

std::vector<std::string> splitDebugLines(std::string const& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.emplace_back(std::move(line));
    }
    if (lines.empty()) {
        lines.emplace_back("");
    }
    return lines;
}

} // namespace

void PhysicsOverlay::updateDebugOverlayText(float intervalSec) {
    if (!m_debugLabel || !m_physics) {
        return;
    }

    float const sampleMs = intervalSec * 1000.0f;
    float const sampleHz = intervalSec > 0.00001f ? (1.0f / intervalSec) : 0.0f;
    float const accumulatorRatio = std::clamp(m_physicsAccumulator / kFixedPhysicsDt, 0.0f, 1.0f);

    PhysicsState const playerState = m_physics->getPlayerState();
    PhysicsVelocity const playerVelocity = m_physics->getPlayerVelocityPixels();
    float const playerSpeed = m_physics->getPlayerSpeed();

    bool const panelAvailable = m_physics->hasPanel();
    PhysicsState panelState{};
    PhysicsVelocity panelVelocity{};
    if (panelAvailable) {
        panelState = m_physics->getPanelState();
        panelVelocity = m_physics->getPanelVelocityPixels();
    }

    int const trailGhosts = m_trail.layer ? m_trail.layer->getChildrenCount() : 0;

    std::ostringstream debug;
    debug << std::fixed << std::setprecision(1);
    debug << "sample ms:" << sampleMs
          << " hz:" << sampleHz
          << " substeps:" << m_lastPhysicsSubsteps
          << " acc:" << accumulatorRatio << "\n";
    debug << "player pos(" << playerState.x << "," << playerState.y
          << ") ang:" << playerState.angle
          << " vel(" << playerVelocity.vx << "," << playerVelocity.vy
          << ") speed:" << playerSpeed << "\n";
    debug << "panel on:" << (panelAvailable ? 1 : 0);
    if (panelAvailable) {
        debug << " pos(" << panelState.x << "," << panelState.y
              << ") ang:" << panelState.angle
              << " vel(" << panelVelocity.vx << "," << panelVelocity.vy << ")";
    }
    debug << "\n";
    debug << "impact player trig:" << (m_lastPlayerImpact.triggered ? 1 : 0)
          << " pre:" << m_lastPlayerImpact.preSpeedPx
          << " post:" << m_lastPlayerImpact.postSpeedPx
          << " hit:" << m_lastPlayerImpact.impactSpeedPx << "\n";
    debug << "impact panel trig:" << (m_lastPanelImpact.triggered ? 1 : 0)
          << " pre:" << m_lastPanelImpact.preSpeedPx
          << " post:" << m_lastPanelImpact.postSpeedPx
          << " hit:" << m_lastPanelImpact.impactSpeedPx << "\n";
    debug << "cooldowns hitstop:" << m_impactFlash.hitstopRemaining
          << " white:" << m_impactFlash.whiteFlashRemaining
          << " flashCd:" << m_impactFlash.impactFlashCooldownRemaining
          << " noise:" << m_impactNoise.remaining
          << " starPh:" << m_starBurst.phaseIndex
          << " fireT:" << m_fireAura.time << "\n";
    debug << "trail on:" << (m_trail.active ? 1 : 0)
          << " ghosts:" << trailGhosts
          << " spawnAcc:" << m_trail.spawnAccumulator << "\n";
    debug << "world bodies:" << m_physics->getBodyCount()
          << " joints:" << m_physics->getJointCount()
          << " arbiters:" << m_physics->getArbiterCount()
          << " shards:" << m_physics->getShatterBodyCount()
          << " grab:" << (m_grabActive ? 1 : 0)
          << " panelDrag:" << (m_panelDragActive ? 1 : 0)
          << " vis:" << (m_visualBuilt ? 1 : 0);

    std::string const text = debug.str();
    m_debugLabel->setString(text.c_str());
    if (m_debugLabelBackground && m_debugLabelMeasure) {
        auto const lines = splitDebugLines(text);

        float const scaledTotalHeight = m_debugLabel->getContentSize().height * kDebugLabelFontScale;
        float const lineAdvance =
            lines.empty() ? 0.0f : (scaledTotalHeight / static_cast<float>(lines.size()));
        float const baseX = kDebugLabelMarginX;
        float const topY = m_winSize.height - kDebugLabelMarginY;
        ccColor4F const boxFill = debugLabelBoxFill();
        ccColor3B const boxColor = ccc3(
            colorToByte(boxFill.r),
            colorToByte(boxFill.g),
            colorToByte(boxFill.b)
        );
        GLubyte const boxOpacity = colorToByte(boxFill.a);
        auto* textureSprite = m_debugLabelBackgroundTexture ? m_debugLabelBackgroundTexture->getSprite() : nullptr;
        auto* boxTexture = textureSprite ? textureSprite->getTexture() : nullptr;

        if (m_debugLabelBackgroundSprites.size() < lines.size() && boxTexture) {
            for (size_t i = m_debugLabelBackgroundSprites.size(); i < lines.size(); ++i) {
                auto* sprite = CCSprite::createWithTexture(boxTexture);
                if (!sprite) {
                    continue;
                }
                sprite->setAnchorPoint({0.0f, 0.0f});
                sprite->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
                sprite->setVisible(false);
                m_debugLabelBackground->addChild(sprite);
                m_debugLabelBackgroundSprites.push_back(sprite);
            }
        }

        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
            m_debugLabelMeasure->setString(lines[lineIndex].c_str());
            float const lineWidth =
                m_debugLabelMeasure->getContentSize().width * kDebugLabelFontScale + (kDebugLabelBoxPadX * 2.0f);
            float const minX = baseX - kDebugLabelBoxPadX;
            float const maxX = minX + lineWidth;
            float const lineTop = topY - (static_cast<float>(lineIndex) * lineAdvance);
            float const lineBottom = lineTop - lineAdvance;
            float const maxY = lineTop + kDebugLabelBoxPadY;
            float const minY = lineBottom - kDebugLabelBoxPadY;
            if (lineIndex >= m_debugLabelBackgroundSprites.size()) {
                continue;
            }

            auto* sprite = m_debugLabelBackgroundSprites[lineIndex];
            sprite->setColor(boxColor);
            sprite->setOpacity(boxOpacity);
            sprite->setPosition({minX, minY});
            sprite->setScaleX(maxX - minX);
            sprite->setScaleY(maxY - minY);
            sprite->setVisible(true);
        }

        for (size_t lineIndex = lines.size(); lineIndex < m_debugLabelBackgroundSprites.size(); ++lineIndex) {
            m_debugLabelBackgroundSprites[lineIndex]->setVisible(false);
        }
    }
}
