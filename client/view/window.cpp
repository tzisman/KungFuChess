#include "view/window.hpp"

#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace kfc::view {
namespace {

// Matches the neutral canvas colour every renderer fills its background
// with, so the bars this pads with disappear into the surrounding frame
// instead of reading as a visibly different colour.
const cv::Scalar kLetterboxColour{240, 240, 240, 255};

// Scales frame up or down as one block — never stretching width and height by
// different factors, which would distort it — to the largest size that fits
// inside target, then centres it on a target-sized canvas. Whichever axis
// has slack is padded rather than cropped, so scaling never overflows target
// regardless of how the window's current aspect ratio compares to the
// frame's.
Img fitToSize(const Img& frame, WindowSize target) {
    const cv::Mat& pixels = frame.get_mat();
    double scale =
        std::min(static_cast<double>(target.width) / pixels.cols,
                 static_cast<double>(target.height) / pixels.rows);
    int scaledWidth = std::clamp(
        static_cast<int>(std::lround(pixels.cols * scale)), 1, target.width);
    int scaledHeight = std::clamp(
        static_cast<int>(std::lround(pixels.rows * scale)), 1, target.height);

    Img scaled = frame.clone();
    scaled.resizeTo(scaledWidth, scaledHeight);

    Img canvas;
    canvas.blank(target.width, target.height, kLetterboxColour);
    scaled.draw_on(canvas, (target.width - scaledWidth) / 2,
                   (target.height - scaledHeight) / 2);
    return canvas;
}

}  // namespace

Window::Window(std::string title) : title_(std::move(title)) {
    cv::namedWindow(title_, cv::WINDOW_NORMAL);
    cv::setMouseCallback(title_, &Window::onMouse, this);
}

Window::~Window() { cv::destroyWindow(title_); }

void Window::show(const Img& frame) {
    WindowSize live = contentSize();
    const cv::Mat& pixels = frame.get_mat();
    if (live.width <= 0 || live.height <= 0 ||
        (pixels.cols == live.width && pixels.rows == live.height)) {
        cv::imshow(title_, pixels);
        return;
    }

    cv::imshow(title_, fitToSize(frame, live).get_mat());
}

int Window::waitKey(int waitMs) const { return cv::waitKey(waitMs); }

WindowSize Window::contentSize() const {
    cv::Rect rect = cv::getWindowImageRect(title_);
    return {rect.width, rect.height};
}

void Window::resizeTo(WindowSize size) {
    cv::resizeWindow(title_, size.width, size.height);
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
