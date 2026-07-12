#include "model/piece.hpp"

#include <ostream>

namespace kfc::model {

Piece::Piece(PieceId id, Color color, PieceKind kind, Position cell,
             PieceState state)
    : id_(id), color_(color), kind_(kind), cell_(cell), state_(state) {}

std::ostream& operator<<(std::ostream& os, Color color) {
    switch (color) {
        case Color::kWhite: return os << "White";
        case Color::kBlack: return os << "Black";
    }
    return os << "?";
}

std::ostream& operator<<(std::ostream& os, PieceKind kind) {
    switch (kind) {
        case PieceKind::kKing:   return os << "King";
        case PieceKind::kQueen:  return os << "Queen";
        case PieceKind::kRook:   return os << "Rook";
        case PieceKind::kBishop: return os << "Bishop";
        case PieceKind::kKnight: return os << "Knight";
        case PieceKind::kPawn:   return os << "Pawn";
    }
    return os << "?";
}

std::ostream& operator<<(std::ostream& os, PieceState state) {
    switch (state) {
        case PieceState::kIdle:     return os << "Idle";
        case PieceState::kMoving:   return os << "Moving";
        case PieceState::kCaptured: return os << "Captured";
    }
    return os << "?";
}

std::ostream& operator<<(std::ostream& os, const Piece& piece) {
    return os << "Piece(id=" << piece.id()
              << ", color=" << piece.color()
              << ", kind=" << piece.kind()
              << ", cell=" << piece.cell()
              << ", state=" << piece.state() << ")";
}

}
