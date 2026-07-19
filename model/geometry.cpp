#include "model/geometry.hpp"

#include <cstdlib>

namespace kfc::model {

namespace {

int sign(int value) {
    return (value > 0) - (value < 0);
}

}

Position shifted(Position cell, Offset offset) {
    return Position{cell.row + offset.dRow, cell.col + offset.dCol};
}

Offset stepDirection(Position from, Position to) {
    return Offset{sign(to.row - from.row), sign(to.col - from.col)};
}

int cellDistance(Position from, Position to) {
    int dRow = std::abs(to.row - from.row);
    int dCol = std::abs(to.col - from.col);
    return dRow > dCol ? dRow : dCol;
}

bool isStraightLine(Position from, Position to) {
    int dRow = std::abs(to.row - from.row);
    int dCol = std::abs(to.col - from.col);
    return dRow == 0 || dCol == 0 || dRow == dCol;
}

std::vector<Position> pathCells(Position from, Position to) {
    std::vector<Position> cells;
    if (from == to) {
        return cells;
    }
    if (!isStraightLine(from, to)) {
        cells.push_back(to);
        return cells;
    }
    Offset step = stepDirection(from, to);
    for (Position cell = shifted(from, step); cell != to; cell = shifted(cell, step)) {
        cells.push_back(cell);
    }
    cells.push_back(to);
    return cells;
}

}
