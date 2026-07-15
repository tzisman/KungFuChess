#pragma once

#include <string>
#include <vector>

#include "img.hpp"

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

    void show(const Img& frame);

    // Pumps the window's events and waits up to waitMs for a key. Pass 0 to
    // wait indefinitely. Returns the key code, or -1 if none was pressed.
    int waitKey(int waitMs) const;

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
