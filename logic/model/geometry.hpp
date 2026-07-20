#pragma once

#include <vector>

#include "model/position.hpp"

namespace kfc::model {

// Pure coordinate geometry over Positions: the single source of the straight-line
// stepping shared by the movement rules and the real-time motion. It knows nothing
// of the board, the game rules, or piece kinds.
struct Offset {
    int dRow;
    int dCol;
};

Position shifted(Position cell, Offset offset);

// The unit step (sign per axis) from `from` toward `to`. For an aligned line this
// is one cell along it; for `from == to` it is {0, 0}.
Offset stepDirection(Position from, Position to);

// King-steps (Chebyshev distance) between two cells.
int cellDistance(Position from, Position to);

// Whether the two cells lie on a single rank, file, or diagonal.
bool isStraightLine(Position from, Position to);

// The cells a piece passes through going from `from` to `to`, excluding `from`
// and including `to`. Along a straight line this is one cell per step; otherwise
// (a leap, e.g. a knight) it is the single cell `to`.
std::vector<Position> pathCells(Position from, Position to);

}
