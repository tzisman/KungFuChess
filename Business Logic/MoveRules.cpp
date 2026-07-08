#include "MoveRules.hpp"

#include <algorithm>
#include <cstdlib>

#include "Piece.hpp"

namespace kfc::logic {

bool isOrthogonalMove(Position from, Position to) {
    return from.row == to.row || from.col == to.col;
}

bool isDiagonalMove(Position from, Position to) {
    return std::abs(to.row - from.row) == std::abs(to.col - from.col);
}

bool isPathClear(const Board& board, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    int stepR = (dr > 0) - (dr < 0);
    int stepC = (dc > 0) - (dc < 0);

    Position cur{from.row + stepR, from.col + stepC};
    while (cur.row != to.row || cur.col != to.col) {
        if (!board.isEmpty(cur)) return false;
        cur.row += stepR;
        cur.col += stepC;
    }
    return true;
}

int chebyshevDistance(Position from, Position to) {
    return std::max(std::abs(to.row - from.row), std::abs(to.col - from.col));
}

long long travelDurationMs(Position from, Position to) {
    return chebyshevDistance(from, to) * kSquareTravelMs;
}

bool isMoveLegal(const Board& board, const Piece& piece, char color, Position from, Position to) {
    if (from == to) return false;
    if (!piece.isShapeLegal(color, from, to, board.height())) return false;
    if (piece.requiresClearPath() && !isPathClear(board, from, to)) return false;
    if (!piece.isDestinationLegal(board, color, from, to)) return false;
    return true;
}

}
