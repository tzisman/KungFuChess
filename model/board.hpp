#pragma once

#include <map>
#include <optional>

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::model {

class Board {
public:
    Board(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    bool inBounds(Position cell) const;
    std::optional<Piece> pieceAt(Position cell) const;

    void addPiece(const Piece& piece);
    void removePiece(Position cell);
    void movePiece(Position from, Position to);

private:
    void requireInBounds(Position cell) const;

    int width_;
    int height_;
    std::map<Position, Piece> cells_;
};

}
