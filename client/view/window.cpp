#include "view/window.hpp"

#include <opencv2/highgui.hpp>

#include <utility>

namespace kfc::view {

Window::Window(std::string title, WindowSize initial)
    : title_(std::move(title)) {
    cv::namedWindow(title_, cv::WINDOW_NORMAL);
    cv::resizeWindow(title_, initial.width, initial.height);
    cv::setMouseCallback(title_, &Window::onMouse, this);
}

Window::~Window() { cv::destroyWindow(title_); }

void Window::show(const Img& frame) {
    cv::imshow(title_, frame.get_mat());
    shown_ = true;
}

int Window::waitKey(int waitMs) {
    int key = cv::waitKey(waitMs);
    measurable_ = shown_;
    return key;
}

std::optional<WindowSize> Window::contentSize() const {
    if (!measurable_) return std::nullopt;

    cv::Rect rect = cv::getWindowImageRect(title_);
    if (rect.width <= 0 || rect.height <= 0) return std::nullopt;
    return WindowSize{rect.width, rect.height};
}

std::vector<MouseEvent> Window::takeMouseEvents() {
    std::vector<MouseEvent> taken;
    taken.swap(events_);
    return taken;
}

// Called by the toolkit while the window pumps its events, which happens
// inside waitKey on this same thread, so the queue needs no locking.
void Window::onMouse(int event, int x, int y, int, void* userdata) {
    Window& window = *static_cast<Window*>(userdata);

    if (event == cv::EVENT_LBUTTONDOWN) {
        window.events_.push_back({MouseEvent::Type::kClick, x, y});
    } else if (event == cv::EVENT_LBUTTONDBLCLK) {
        window.events_.push_back({MouseEvent::Type::kDoubleClick, x, y});
    }
}

}
