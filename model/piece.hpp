#pragma once

#include <array>
#include <iosfwd>

#include "model/position.hpp"

namespace kfc::model {

using PieceId = int;

enum class Color { kWhite, kBlack };
enum class PieceKind { kKing, kQueen, kRook, kBishop, kKnight, kPawn };

inline constexpr std::array<PieceKind, 6> kAllPieceKinds{
    PieceKind::kKing,   PieceKind::kQueen,  PieceKind::kRook,
    PieceKind::kBishop, PieceKind::kKnight, PieceKind::kPawn,
};

inline constexpr std::array<Color, 2> kAllColors{Color::kWhite, Color::kBlack};


enum class PieceState {
    kIdle,
    kMoving,
    kCaptured,
    kAirborne,
    kShortResting,
    kLongResting,
};

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
    void setKind(PieceKind kind) { kind_ = kind; }

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
