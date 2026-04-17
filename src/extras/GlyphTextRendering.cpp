#include "GlyphTextRendering.h"

#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/cocos/misc_nodes/CCRenderTexture.h>
#include <Geode/cocos/platform/CCFileUtils.h>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <Geode/cocos/sprite_nodes/CCSpriteFrameCache.h>
#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <regex>
#include <sstream>
#include <unordered_map>

using namespace geode::prelude;

namespace extras::glyph_text {
namespace {

struct GlyphAtlasCache {
    bool attemptedLoad = false;
    bool loaded = false;
    std::unordered_map<char, std::string> symbolFrameNames;
};

GlyphAtlasCache& glyphAtlasCache() {
    static GlyphAtlasCache cache;
    return cache;
}

std::string qualifyFrameName(std::string const& rawFrameName) {
    return Mod::get()->getID() + "/" + rawFrameName;
}

CCSpriteFrame* findGlyphFrame(std::string const& rawFrameName, bool allowNamespacedFallback = true) {
    auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    // Raw frame keys are what glyph_atlas.plist stores
    // Probe them first to avoid Geode fallback-texture warnings for missing namespaced keys
    if (auto* raw = cache->spriteFrameByName(rawFrameName.c_str())) {
        return raw;
    }
    if (!allowNamespacedFallback) {
        return nullptr;
    }
    std::string const qualified = qualifyFrameName(rawFrameName);
    return cache->spriteFrameByName(qualified.c_str());
}

std::string decodeXmlEntityString(std::string text) {
    size_t pos = 0;
    while ((pos = text.find("&amp;", pos)) != std::string::npos) {
        text.replace(pos, 5, "&");
        pos += 1;
    }
    pos = 0;
    while ((pos = text.find("&lt;", pos)) != std::string::npos) {
        text.replace(pos, 4, "<");
        pos += 1;
    }
    pos = 0;
    while ((pos = text.find("&gt;", pos)) != std::string::npos) {
        text.replace(pos, 4, ">");
        pos += 1;
    }
    pos = 0;
    while ((pos = text.find("&quot;", pos)) != std::string::npos) {
        text.replace(pos, 6, "\"");
        pos += 1;
    }
    pos = 0;
    while ((pos = text.find("&apos;", pos)) != std::string::npos) {
        text.replace(pos, 6, "'");
        pos += 1;
    }
    return text;
}

bool parseSymbolFrameKeysFromPlist(std::string const& plistXml, std::unordered_map<char, std::string>& outMap) {
    outMap.clear();
    size_t const tagPos = plistXml.find("<key>symbolFrameKeys</key>");
    if (tagPos == std::string::npos) {
        return false;
    }
    size_t const dictOpenPos = plistXml.find("<dict>", tagPos);
    if (dictOpenPos == std::string::npos) {
        return false;
    }
    size_t const dictClosePos = plistXml.find("</dict>", dictOpenPos);
    if (dictClosePos == std::string::npos || dictClosePos <= dictOpenPos) {
        return false;
    }

    std::string const dictBlock = plistXml.substr(dictOpenPos, dictClosePos - dictOpenPos);
    std::regex const pairRegex(R"(<key>(.*?)</key>\s*<string>(.*?)</string>)");
    auto begin = std::sregex_iterator(dictBlock.begin(), dictBlock.end(), pairRegex);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::string symbolText = decodeXmlEntityString((*it)[1].str());
        std::string frameName = decodeXmlEntityString((*it)[2].str());
        if (symbolText.size() != 1 || frameName.empty()) {
            continue;
        }
        outMap[symbolText.front()] = frameName;
    }
    return !outMap.empty();
}

bool tryLoadGlyphAtlasMapping(std::string* outError = nullptr) {
    auto& cache = glyphAtlasCache();
    if (cache.loaded) {
        return true;
    }

    std::string const atlasPlist = "glyph_atlas.plist"_spr;
    std::string const fullPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(atlasPlist.c_str(), false);
    if (fullPath.empty()) {
        if (outError) {
            *outError = "missing glyph_atlas.plist asset";
        }
        return false;
    }

    std::string const atlasTexture = "glyph_atlas.png"_spr;
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(
        atlasPlist.c_str(),
        atlasTexture.c_str()
    );

    unsigned long dataSize = 0;
    auto* rawData = CCFileUtils::sharedFileUtils()->getFileData(atlasPlist.c_str(), "rb", &dataSize);
    if (!rawData || dataSize == 0) {
        if (outError) {
            *outError = "failed to open glyph_atlas.plist";
        }
        delete[] rawData;
        return false;
    }

    std::string plistXml(reinterpret_cast<char const*>(rawData), static_cast<size_t>(dataSize));
    delete[] rawData;
    std::unordered_map<char, std::string> mapping;
    if (!parseSymbolFrameKeysFromPlist(plistXml, mapping)) {
        if (outError) {
            *outError = "failed to parse metadata.symbolFrameKeys";
        }
        return false;
    }

    for (auto const& [symbol, frameName] : mapping) {
        if (!findGlyphFrame(frameName, false)) {
            if (outError) {
                *outError = fmt::format(
                    "missing sprite frame in atlas cache (raw='{}', namespaced='{}')",
                    frameName,
                    qualifyFrameName(frameName)
                );
            }
            return false;
        }
    }

    if (!findGlyphFrame("glyph_001_A.png", false)) {
        if (outError) {
            *outError = "atlas verification failed: glyph_001_A.png unresolved";
        }
        return false;
    }

    cache.symbolFrameNames = std::move(mapping);
    cache.attemptedLoad = true;
    cache.loaded = true;
    return true;
}

CCSpriteFrame* resolveGlyphFrame(char symbol, std::string& outFrameName) {
    auto& cache = glyphAtlasCache();
    auto it = cache.symbolFrameNames.find(symbol);
    if (it == cache.symbolFrameNames.end() && std::isalpha(static_cast<unsigned char>(symbol))) {
        char const upperSymbol = static_cast<char>(std::toupper(static_cast<unsigned char>(symbol)));
        it = cache.symbolFrameNames.find(upperSymbol);
    }
    if (it == cache.symbolFrameNames.end()) {
        return nullptr;
    }
    outFrameName = it->second;
    return findGlyphFrame(outFrameName);
}

float effectiveLineHeight(float maxGlyphHeight, TextOptions const& options) {
    float const scaled = maxGlyphHeight * std::max(options.scale, 0.0f);
    if (options.lineHeight > 0.0f) {
        return options.lineHeight;
    }
    return scaled > 0.0f ? scaled : 1.0f;
}

void applyAlignment(LayoutResult& out, LayoutLine const& line, float totalWidth, TextAlign align) {
    if (line.glyphCount == 0) {
        return;
    }
    float offsetX = 0.0f;
    if (align == TextAlign::Center) {
        offsetX = (totalWidth - line.width) * 0.5f;
    } else if (align == TextAlign::Right) {
        offsetX = totalWidth - line.width;
    }
    size_t const end = line.beginGlyphIndex + line.glyphCount;
    for (size_t i = line.beginGlyphIndex; i < end; ++i) {
        out.glyphs[i].position.x += offsetX;
    }
}

float measureWordWidth(std::string const& text, size_t start, TextOptions const& options) {
    float const safeScale = std::max(options.scale, 0.0f);
    float width = 0.0f;
    bool hasGlyph = false;
    for (size_t i = start; i < text.size(); ++i) {
        char const c = text[i];
        if (c == '\n' || c == ' ') {
            break;
        }
        std::string frameName;
        auto* frame = resolveGlyphFrame(c, frameName);
        if (!frame) {
            if (options.unsupportedPolicy == UnsupportedPolicy::Skip) {
                continue;
            }
            frame = resolveGlyphFrame(options.fallbackSymbol, frameName);
            if (!frame) {
                continue;
            }
        }
        float const w = frame->getOriginalSize().width * safeScale;
        if (hasGlyph) {
            width += options.letterSpacing;
        }
        width += w;
        hasGlyph = true;
    }
    return width;
}

float spaceAdvance(TextOptions const& options) {
    float const safeScale = std::max(options.scale, 0.0f);
    // Pick a simple margin for spaces so they don't render fallback glyphs
    return std::max(8.0f, 56.0f * safeScale);
}

std::vector<CCSprite*> buildSpritesFromLayout(LayoutResult const& layout) {
    std::vector<CCSprite*> out;
    out.reserve(layout.glyphs.size());
    for (auto const& glyph : layout.glyphs) {
        if (!glyph.frame) {
            continue;
        }
        auto* sprite = CCSprite::createWithSpriteFrame(glyph.frame);
        if (!sprite) {
            continue;
        }
        sprite->setAnchorPoint({0.0f, 0.0f});
        sprite->setPosition(glyph.position);
        float const fw = sprite->getContentSize().width;
        float const fh = sprite->getContentSize().height;
        sprite->setScaleX(fw > 0.0f ? glyph.size.width / fw : 1.0f);
        sprite->setScaleY(fh > 0.0f ? glyph.size.height / fh : 1.0f);
        out.push_back(sprite);
    }
    return out;
}

} // namespace

LayoutResult layoutText(std::string const& text, TextOptions const& options) {
    LayoutResult out{};
    std::string loadError;
    if (!tryLoadGlyphAtlasMapping(&loadError)) {
        out.error = loadError;
        return out;
    }

    float const safeScale = std::max(options.scale, 0.0f);
    float x = 0.0f;
    float y = 0.0f;
    float maxLineWidth = 0.0f;
    float maxGlyphHeight = 0.0f;
    size_t lineCount = 0;

    LayoutLine currentLine{};
    currentLine.beginGlyphIndex = 0;
    bool hasGlyphInLine = false;
    auto finalizeLine = [&]() {
        currentLine.glyphCount = out.glyphs.size() - currentLine.beginGlyphIndex;
        currentLine.width = (currentLine.glyphCount > 0 && hasGlyphInLine) ? std::max(0.0f, x - options.letterSpacing) : 0.0f;
        maxLineWidth = std::max(maxLineWidth, currentLine.width);
        out.lines.push_back(currentLine);
        currentLine.beginGlyphIndex = out.glyphs.size();
        x = 0.0f;
        hasGlyphInLine = false;
        ++lineCount;
    };

    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == '\n') {
            finalizeLine();
            y -= effectiveLineHeight(maxGlyphHeight, options);
            continue;
        }
        if (symbol == ' ') {
            if (hasGlyphInLine) {
                x += options.letterSpacing;
            }
            x += spaceAdvance(options);
            continue;
        }

        std::string frameName;
        auto* frame = resolveGlyphFrame(symbol, frameName);
        if (!frame) {
            if (options.unsupportedPolicy == UnsupportedPolicy::Skip) {
                continue;
            }
            if (options.unsupportedPolicy == UnsupportedPolicy::Error) {
                out.error = fmt::format("unsupported glyph '{}'", symbol);
                return out;
            }
            frame = resolveGlyphFrame(options.fallbackSymbol, frameName);
            if (!frame) {
                out.error = fmt::format("missing fallback glyph '{}'", options.fallbackSymbol);
                return out;
            }
        }

        CCSize const frameSize = frame->getOriginalSize();
        float const glyphW = frameSize.width * safeScale;
        float const glyphH = frameSize.height * safeScale;
        maxGlyphHeight = std::max(maxGlyphHeight, frameSize.height);

        bool needsWrap = false;
        if (options.maxWidth > 0.0f && options.wrapMode != WrapMode::None && x > 0.0f) {
            if (options.wrapMode == WrapMode::Character) {
                needsWrap = (x + glyphW) > options.maxWidth;
            } else {
                float const wordWidth = measureWordWidth(text, i, options);
                needsWrap = (x + (wordWidth > 0.0f ? wordWidth : glyphW)) > options.maxWidth;
            }
        }
        if (needsWrap) {
            finalizeLine();
            y -= effectiveLineHeight(maxGlyphHeight, options);
        }

        LayoutGlyph glyph{};
        glyph.symbol = symbol;
        glyph.frameName = frameName;
        glyph.frame = frame;
        glyph.position = ccp(x, y);
        glyph.size = CCSizeMake(glyphW, glyphH);
        glyph.advance = glyphW + options.letterSpacing;
        out.glyphs.push_back(std::move(glyph));
        hasGlyphInLine = true;
        x += glyphW + options.letterSpacing;
    }
    finalizeLine();

    for (auto const& line : out.lines) {
        applyAlignment(out, line, maxLineWidth, options.align);
    }

    float minY = 0.0f;
    float maxY = 0.0f;
    for (auto const& glyph : out.glyphs) {
        minY = std::min(minY, glyph.position.y);
        maxY = std::max(maxY, glyph.position.y + glyph.size.height);
    }
    float const lineHeight = effectiveLineHeight(maxGlyphHeight, options);
    float const lineHeightBounds = static_cast<float>(lineCount) * lineHeight;
    out.bounds = CCSizeMake(maxLineWidth, std::max(lineHeightBounds, std::max(0.0f, maxY - minY)));
    out.ok = true;
    return out;
}

std::vector<CCSprite*> buildTextSprites(std::string const& text, TextOptions const& options) {
    auto layout = layoutText(text, options);
    if (!layout.ok) {
        return {};
    }
    return buildSpritesFromLayout(layout);
}

CCRenderTexture* buildTextTexture(std::string const& text, TextOptions const& options) {
    auto layout = layoutText(text, options);
    if (!layout.ok) {
        return nullptr;
    }
    int const rtWidth = std::max(1, static_cast<int>(std::ceil(layout.bounds.width)));
    int const rtHeight = std::max(1, static_cast<int>(std::ceil(layout.bounds.height)));
    auto* rt = CCRenderTexture::create(rtWidth, rtHeight, kCCTexture2DPixelFormat_RGBA8888);
    if (!rt) {
        return nullptr;
    }
    auto sprites = buildSpritesFromLayout(layout);
    float minY = 0.0f;
    for (auto const& sprite : sprites) {
        minY = std::min(minY, sprite->getPositionY());
    }
    for (auto const& sprite : sprites) {
        sprite->setPositionY(sprite->getPositionY() - minY);
    }
    rt->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    for (auto* sprite : sprites) {
        sprite->visit();
    }
    rt->end();
    return rt;
}

CCSprite* buildTextSprite(std::string const& text, TextOptions const& options) {
    auto* texture = buildTextTexture(text, options);
    if (!texture || !texture->getSprite()) {
        return nullptr;
    }
    auto* sprite = CCSprite::createWithTexture(texture->getSprite()->getTexture());
    if (!sprite) {
        return nullptr;
    }
    sprite->setAnchorPoint({0.0f, 0.0f});
    sprite->setFlipY(true);
    return sprite;
}

} // namespace extras::glyph_text
