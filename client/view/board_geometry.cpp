#include "view/board_geometry.hpp"

namespace kfc::view {

BoardGeometry::BoardGeometry(int imageWidth, int imageHeight, int cols,
                             int rows, Pixel origin)
    : imageWidth_(imageWidth),
      imageHeight_(imageHeight),
      cols_(cols),
      rows_(rows),
      origin_(origin) {}

int BoardGeometry::cellWidth() const { return imageWidth_ / cols_; }

int BoardGeometry::cellHeight() const { return imageHeight_ / rows_; }


Pixel BoardGeometry::topLeftOf(model::Position cell) const {
    return {origin_.x + (cell.col * imageWidth_ + cols_ - 1) / cols_,
            origin_.y + (cell.row * imageHeight_ + rows_ - 1) / rows_};
}

std::optional<model::Position> BoardGeometry::cellAt(int x, int y) const {
    int boardX = x - origin_.x;
    int boardY = y - origin_.y;
    if (boardX < 0 || boardY < 0 || boardX >= imageWidth_ ||
        boardY >= imageHeight_) {
        return std::nullopt;
    }
    return model::Position{boardY * rows_ / imageHeight_,
                           boardX * cols_ / imageWidth_};
}

}
