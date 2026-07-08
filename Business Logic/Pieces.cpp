#include "Pieces.hpp"

#include <cstdlib>

#include "MoveRules.hpp"
#include "PieceConstants.hpp"

namespace kfc::logic {

namespace {
// How far a pawn's start row is inset from its own back edge of the board.
// 0 = pawns start on the edge row; bump to 1 for a standard-chess back rank
// in front of them. Pawn-specific configuration, so it lives with the pawn.
constexpr int kPawnStartRowInsetFromEdge = 0;
}

// --- King -----------------------------------------------------------------
char King::symbol() const { return symbols::King; }

bool King::isShapeLegal(char, Position from, Position to, int) const {
    return std::abs(to.row - from.row) <= 1 && std::abs(to.col - from.col) <= 1;
}

bool King::requiresClearPath() const { return false; }

bool King::isRoyal() const { return true; }

// --- Rook -----------------------------------------------------------------
char Rook::symbol() const { return symbols::Rook; }

bool Rook::isShapeLegal(char, Position from, Position to, int) const {
    return isOrthogonalMove(from, to);
}

bool Rook::requiresClearPath() const { return true; }

// --- Bishop ---------------------------------------------------------------
char Bishop::symbol() const { return symbols::Bishop; }

bool Bishop::isShapeLegal(char, Position from, Position to, int) const {
    return isDiagonalMove(from, to);
}

bool Bishop::requiresClearPath() const { return true; }

// --- Queen ----------------------------------------------------------------
char Queen::symbol() const { return symbols::Queen; }

bool Queen::isShapeLegal(char, Position from, Position to, int) const {
    return isOrthogonalMove(from, to) || isDiagonalMove(from, to);
}

bool Queen::requiresClearPath() const { return true; }

// --- Knight ---------------------------------------------------------------
char Knight::symbol() const { return symbols::Knight; }

bool Knight::isShapeLegal(char, Position from, Position to, int) const {
    int adr = std::abs(to.row - from.row);
    int adc = std::abs(to.col - from.col);
    return (adr == 1 && adc == 2) || (adr == 2 && adc == 1);
}

bool Knight::requiresClearPath() const { return false; }

// --- Pawn -----------------------------------------------------------------
char Pawn::symbol() const { return symbols::Pawn; }

int Pawn::direction(char color) const { return (color == kWhiteColor) ? -1 : 1; }

int Pawn::startRow(char color, int boardHeight) const {
    return (color == kWhiteColor) ? (boardHeight - 1 - kPawnStartRowInsetFromEdge)
                                  : kPawnStartRowInsetFromEdge;
}

int Pawn::promotionRow(char color, int boardHeight) const {
    return (color == kWhiteColor) ? 0 : boardHeight - 1;
}

bool Pawn::isShapeLegal(char color, Position from, Position to, int boardHeight) const {
    int dir = direction(color);
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    if (std::abs(dc) == 1) return dr == dir;
    if (dc != 0) return false;
    if (dr == dir) return true;
    if (dr == 2 * dir) return from.row == startRow(color, boardHeight);
    return false;
}

bool Pawn::requiresClearPath() const { return true; }

bool Pawn::isDestinationLegal(const Board& board, char, Position from, Position to) const {
    bool isDiagonal = (to.col != from.col);
    return isDiagonal ? !board.isEmpty(to) : board.isEmpty(to);
}

std::optional<char> Pawn::promotionSymbol(char color, Position at, int boardHeight) const {
    if (at.row == promotionRow(color, boardHeight)) return symbols::Queen;
    return std::nullopt;
}

}
