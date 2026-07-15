#include "view/board_geometry.hpp"

namespace kfc::view {

BoardGeometry::BoardGeometry(int imageWidth, int imageHeight, int cols,
                             int rows)
    : imageWidth_(imageWidth),
      imageHeight_(imageHeight),
      cols_(cols),
      rows_(rows) {}

int BoardGeometry::cellWidth() const { return imageWidth_ / cols_; }

int BoardGeometry::cellHeight() const { return imageHeight_ / rows_; }

// Multiplying before dividing keeps the cells evenly spread across the whole
// image even when the image does not divide exactly by the cell count, so the
// rounding never accumulates across the board. The division rounds up because
// a cell starts at the first pixel that belongs to it: rounding down would
// name the last pixel of the previous cell, which cellAt would then read back
// as that previous cell.
Pixel BoardGeometry::topLeftOf(model::Position cell) const {
    return {(cell.col * imageWidth_ + cols_ - 1) / cols_,
            (cell.row * imageHeight_ + rows_ - 1) / rows_};
}

std::optional<model::Position> BoardGeometry::cellAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= imageWidth_ || y >= imageHeight_) {
        return std::nullopt;
    }
    return model::Position{y * rows_ / imageHeight_, x * cols_ / imageWidth_};
}

}
