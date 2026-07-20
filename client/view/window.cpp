#include "view/window.hpp"

#include <opencv2/highgui.hpp>

#include <utility>

namespace kfc::view {

Window::Window(std::string title) : title_(std::move(title)) {
    cv::namedWindow(title_, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(title_, &Window::onMouse, this);
}

Window::~Window() { cv::destroyWindow(title_); }

void Window::show(const Img& frame) { cv::imshow(title_, frame.get_mat()); }

int Window::waitKey(int waitMs) const { return cv::waitKey(waitMs); }

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
