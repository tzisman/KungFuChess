#pragma once

#include <optional>
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
//
// The frame it is given is stretched by the toolkit to fill the window, and
// mouse positions come back measured in that frame's own pixels rather than
// the screen's. So a frame built to contentSize() is shown pixel-for-pixel and
// clicked exactly where it was aimed, while a frame of any other size still
// stays fully visible and still maps clicks correctly — it is only rescaled,
// never cropped or shifted. That is what lets an expensive rebuild of the view
// be deferred until a resize drag settles.
class Window {
public:
    // The only moment the window is given a size. From here on it is the
    // user's to drag: nothing writes a size back to it, so what is on screen
    // never moves unless the user moves it.
    Window(std::string title, WindowSize initial);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void show(const Img& frame);

    // Pumps the window's events and waits up to waitMs for a key. Pass 0 to
    // wait indefinitely. Returns the key code, or -1 if none was pressed.
    int waitKey(int waitMs);

    // The area the window draws its frame into, in pixels — the size a frame
    // must be built to in order to be shown unscaled.
    //
    // Empty until a frame has been shown and its events pumped: until then the
    // toolkit has not laid the window out and answers with a size that is not
    // merely stale but wrong, and a view built to that answer would be locked
    // to a fraction of the window it is supposed to fill.
    std::optional<WindowSize> contentSize() const;

    // Hands over the mouse events collected since the last call, leaving none
    // behind. Events are queued rather than dispatched so that the window need
    // know nothing about what reacts to them.
    std::vector<MouseEvent> takeMouseEvents();

private:
    static void onMouse(int event, int x, int y, int flags, void* userdata);

    std::string title_;
    std::vector<MouseEvent> events_;
    bool shown_ = false;
    bool measurable_ = false;
};

}
