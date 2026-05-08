#pragma once

#include <Geode/utils/cocos.hpp>

#include <algorithm>

inline GLubyte colorChannelFToByte(float channel) {
    float const c = std::clamp(channel, 0.0f, 1.0f);
    return static_cast<GLubyte>(c * 255.0f);
}

inline cocos2d::ccColor3B rgbFToColor3B(float r, float g, float b) {
    return cocos2d::ccc3(
        colorChannelFToByte(r),
        colorChannelFToByte(g),
        colorChannelFToByte(b)
    );
}
