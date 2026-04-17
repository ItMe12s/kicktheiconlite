#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

namespace cocos2d {
class CCSprite;
class CCRenderTexture;
class CCSpriteFrame;
}

namespace extras::glyph_text {

enum class TextAlign : int {
    Left = 0,
    Center = 1,
    Right = 2,
};

enum class UnsupportedPolicy : int {
    Skip = 0,
    ReplaceWithFallback = 1,
    Error = 2,
};

enum class WrapMode : int {
    None = 0,
    Character = 1,
    Word = 2,
};

struct TextOptions {
    float scale = 1.0f;
    float letterSpacing = 0.0f;
    float lineHeight = 0.0f;
    TextAlign align = TextAlign::Left;
    UnsupportedPolicy unsupportedPolicy = UnsupportedPolicy::ReplaceWithFallback;
    char fallbackSymbol = '?';
    float maxWidth = 0.0f;
    WrapMode wrapMode = WrapMode::None;
};

struct LayoutGlyph {
    char symbol = '\0';
    std::string frameName;
    // Non-owning pointer resolved from sprite frame cache; treat as short-lived.
    cocos2d::CCSpriteFrame* frame = nullptr;
    cocos2d::CCPoint position = {0.0f, 0.0f};
    cocos2d::CCSize size = {0.0f, 0.0f};
    float advance = 0.0f;
};

struct LayoutLine {
    size_t beginGlyphIndex = 0;
    size_t glyphCount = 0;
    float width = 0.0f;
};

struct LayoutResult {
    bool ok = false;
    std::vector<LayoutGlyph> glyphs;
    std::vector<LayoutLine> lines;
    cocos2d::CCSize bounds = {0.0f, 0.0f};
    std::string error;
};

LayoutResult layoutText(std::string const& text, TextOptions const& options = {});
std::vector<cocos2d::CCSprite*> buildTextSprites(std::string const& text, TextOptions const& options = {});
cocos2d::CCRenderTexture* buildTextTexture(std::string const& text, TextOptions const& options = {});
cocos2d::CCSprite* buildTextSprite(std::string const& text, TextOptions const& options = {});

} // namespace extras::glyph_text
