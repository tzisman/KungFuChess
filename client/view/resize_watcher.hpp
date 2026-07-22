#pragma once

#include <optional>

namespace kfc::view {

// A window's content area in pixels. Toolkit-neutral: nothing here names
// OpenCV, so this can be reasoned about (and tested) without a display.
struct WindowSize {
    int width;
    int height;

    bool operator==(const WindowSize& other) const {
        return width == other.width && height == other.height;
    }
};

// Turns a window's raw, continuously-changing content size into a single
// settled size once the user stops dragging its edge. Feed it the window's
// current size every frame; it reports a new size only once that size has
// held steady for kSettleMs, so an expensive rebuild of the view (which
// reloads every piece sprite from disk) never fires on the intermediate sizes
// of an in-progress drag.
//
// A reported size is adopted as the new baseline, so each size the user comes
// to rest at is reported exactly once. Nothing may push a size back onto the
// window in response: the window's size is the user's alone, and a size
// written back would return through poll() as a fresh resize, rebuilding the
// view again with nobody touching anything.
class ResizeWatcher {
public:
    explicit ResizeWatcher(WindowSize initial);

    // elapsedMs is the time since the previous call. Returns the settled size
    // exactly once, the moment it stabilizes; nullopt otherwise.
    std::optional<WindowSize> poll(WindowSize current, int elapsedMs);

private:
    WindowSize pending_;
    WindowSize settled_;
    int msPendingHeld_;
};

}
