#pragma once

#include "Board.hpp"

namespace kfc::logic {

class Piece;

inline constexpr long long kSquareTravelMs = 1000;
inline constexpr long long kJumpDurationMs = 1000;

// Shared geometry predicates, used by several pieces to describe their shape.
bool isOrthogonalMove(Position from, Position to);
bool isDiagonalMove(Position from, Position to);

// Shared, type-agnostic helpers.
bool isPathClear(const Board& board, Position from, Position to);
int chebyshevDistance(Position from, Position to);
long long travelDurationMs(Position from, Position to);

// Orchestrates the per-piece rules (shape, path, destination) into a single
// legality check. The piece supplies the type-specific answers; the sequencing
// and the shared path scan live here.
bool isMoveLegal(const Board& board, const Piece& piece, char color, Position from, Position to);

}
