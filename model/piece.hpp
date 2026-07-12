#pragma once

#include <iosfwd>

#include "model/position.hpp"

namespace kfc::model {

using PieceId = int;

enum class Color { kWhite, kBlack };
enum class PieceKind { kKing, kQueen, kRook, kBishop, kKnight, kPawn };

// Lifecycle flag only: no path, destination, timing, or arrival. Those belong
// to Motion and RealTimeArbiter.
enum class PieceState { kIdle, kMoving, kCaptured };

// A piece's identity (id, color, kind) is fixed at construction; only its cell
// and lifecycle state change, and only from the outside.
class Piece {
public:
    Piece(PieceId id, Color color, PieceKind kind, Position cell,
          PieceState state = PieceState::kIdle);

    PieceId id() const { return id_; }
    Color color() const { return color_; }
    PieceKind kind() const { return kind_; }
    Position cell() const { return cell_; }
    PieceState state() const { return state_; }

    void setCell(Position cell) { cell_ = cell; }
    void setState(PieceState state) { state_ = state; }

private:
    PieceId id_;
    Color color_;
    PieceKind kind_;
    Position cell_;
    PieceState state_;
};

std::ostream& operator<<(std::ostream& os, Color color);
std::ostream& operator<<(std::ostream& os, PieceKind kind);
std::ostream& operator<<(std::ostream& os, PieceState state);
std::ostream& operator<<(std::ostream& os, const Piece& piece);

}
