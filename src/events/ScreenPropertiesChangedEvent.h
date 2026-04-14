#pragma once

#include <Geode/loader/Event.hpp>

namespace events {

enum class ScreenPropertiesChangeSource {
    FullscreenToggle,
    FrameSizeChanged,
};

struct ScreenProperties {
    float width = 0.f;
    float height = 0.f;
    bool fullscreen = false;
    bool borderless = false;
    bool fix = false;
    ScreenPropertiesChangeSource source = ScreenPropertiesChangeSource::FullscreenToggle;
};

inline bool operator==(ScreenProperties const& a, ScreenProperties const& b) {
    return a.width == b.width && a.height == b.height && a.fullscreen == b.fullscreen
        && a.borderless == b.borderless && a.fix == b.fix;
}

struct ScreenPropertiesChangedMarker {};

using ScreenPropertiesChangedEvent =
    geode::Event<ScreenPropertiesChangedMarker, void(ScreenProperties const&)>;

// Snapshot of the current GL view frame size and fullscreen flags (for listeners or polling)
ScreenProperties captureScreenProperties(ScreenPropertiesChangeSource source);

} // namespace events
