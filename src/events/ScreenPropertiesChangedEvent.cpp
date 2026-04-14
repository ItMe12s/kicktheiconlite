#include "ScreenPropertiesChangedEvent.h"

#include <Geode/modify/CCEGLView.hpp>

using namespace geode::prelude;

namespace events {

ScreenProperties captureScreenProperties(ScreenPropertiesChangeSource source) {
    ScreenProperties p{};
    p.source = source;
    auto* view = cocos2d::CCEGLView::sharedOpenGLView();
    if (!view) {
        return p;
    }
    auto const fs = view->getFrameSize();
    p.width = fs.width;
    p.height = fs.height;
    p.fullscreen = view->getIsFullscreen();
    p.borderless = view->getIsBorderless();
    p.fix = view->m_bIsFix;
    return p;
}

} // namespace events

class $modify(ScreenPropertiesCCEGLViewHook, cocos2d::CCEGLView) {
    void toggleFullScreen(bool fullscreen, bool borderless, bool fix) {
        cocos2d::CCEGLView::toggleFullScreen(fullscreen, borderless, fix);
        events::ScreenPropertiesChangedEvent().send(
            events::captureScreenProperties(events::ScreenPropertiesChangeSource::FullscreenToggle));
    }

    void setFrameSize(float width, float height) {
        cocos2d::CCEGLView::setFrameSize(width, height);
        events::ScreenPropertiesChangedEvent().send(
            events::captureScreenProperties(events::ScreenPropertiesChangeSource::FrameSizeChanged));
    }
};
