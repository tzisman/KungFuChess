#include "view/image_view.hpp"

#include <utility>

#include "img.hpp"

namespace kfc::view {

ImageView::ImageView(std::string boardImagePath)
    : boardImagePath_(std::move(boardImagePath)) {}

void ImageView::show() {
    Img board;
    board.read(boardImagePath_);
    board.show();
}

}
