#pragma once

#include <string>
#include <vector>

#include "img.hpp"
#include "view/resize_watcher.hpp"

namespace kfc::view {

// Something the user did with the mouse, in pixels. The window reports what
// happened and where; what it means for the game is decided elsewhere.
struct MouseEvent {
    enum class Type { kClick, kDoubleClick };

    Type type;
    int x;
    int y;
};

// The on-screen window. This is the only place that drives the GUI toolkit
// directly; everything else in the view works with images alone.
class Window {
public:
    explicit Window(std::string title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Shows frame scaled as one block (never stretched unevenly) to the
    // largest size that fits the window's current content area, letterboxed
    // to fill any leftover space on whichever axis has slack. This makes a
    // resize drag read as the picture scaling continuously with the cursor,
    // rather than snapping only once the view is next rebuilt at the new size
    // (rebuilding reloads every sprite from disk, so it is throttled to fire
    // only once dragging settles — see ResizeWatcher). Always showing a frame
    // whose pixel size matches the content area exactly also keeps a
    // WINDOW_NORMAL window from resizing itself to match the frame, which
    // imshow does whenever the two sizes differ.
    void show(const Img& frame);

    // Pumps the window's events and waits up to waitMs for a key. Pass 0 to
    // wait indefinitely. Returns the key code, or -1 if none was pressed.
    int waitKey(int waitMs) const;

    // The window's current content area, in pixels — what the user has
    // dragged it to, independent of whatever size was last shown into it.
    WindowSize contentSize() const;

    // Snaps the window to an exact content size, e.g. after a resize has
    // settled and the view has been rebuilt to fit it precisely. Read
    // contentSize() again afterward rather than assuming it lands exactly on
    // size — the toolkit doesn't always echo back exactly what was asked for.
    void resizeTo(WindowSize size);

    // Hands over the mouse events collected since the last call, leaving none
    // behind. Events are queued rather than dispatched so that the window need
    // know nothing about what reacts to them.
    std::vector<MouseEvent> takeMouseEvents();

private:
    static void onMouse(int event, int x, int y, int flags, void* userdata);

    std::string title_;
    std::vector<MouseEvent> events_;
};

}
