#pragma once

#include <optional>

#include "model/position.hpp"

namespace kfc::view {

struct Pixel {
    int x;
    int y;
};

// The pixel dimensions of a displayed board, and the conversion between a cell
// and its pixels. An immutable value: when the board is displayed at a new
// size, build a new BoardGeometry rather than mutating anything. It derives
// every measurement from the actual image size, so no cell size is ever fixed
// in code.
class BoardGeometry {
public:
    BoardGeometry(int imageWidth, int imageHeight, int cols, int rows);

    int imageWidth() const { return imageWidth_; }
    int imageHeight() const { return imageHeight_; }
    int cols() const { return cols_; }
    int rows() const { return rows_; }

    int cellWidth() const;
    int cellHeight() const;

    Pixel topLeftOf(model::Position cell) const;
    std::optional<model::Position> cellAt(int x, int y) const;

private:
    int imageWidth_;
    int imageHeight_;
    int cols_;
    int rows_;
};

}
