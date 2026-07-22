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
class ResizeWatcher {
public:
    explicit ResizeWatcher(WindowSize initial);

    // elapsedMs is the time since the previous call. Returns the settled size
    // exactly once, the moment it stabilizes; nullopt otherwise.
    std::optional<WindowSize> poll(WindowSize current, int elapsedMs);

    // Adopts size as already-settled, without reporting it. Used after the
    // caller has itself resized the window to match a rebuilt view: pass the
    // size actually read back from the window afterward, not the size that
    // was requested — the two can differ (DPI scaling, border/title-bar
    // insets), and seeding this from the requested size instead of the real
    // one leaves a permanent gap that poll() reads as a fresh user resize
    // every cycle, drifting the window further each time.
    void reset(WindowSize size);

private:
    WindowSize pending_;
    WindowSize settled_;
    int msPendingHeld_;
};

}
