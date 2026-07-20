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
    // Empties every cell. Lets a board be repopulated in place from a fresh
    // read-model instead of rebuilt, so anything holding a reference to it
    // (e.g. a Controller) keeps seeing the same object as its contents change.
    void clear();
    void movePiece(Position from, Position to);
    void setPieceKind(Position cell, PieceKind kind);
    void setPieceState(Position cell, PieceState state);

private:
    void requireInBounds(Position cell) const;

    int width_;
    int height_;
    std::map<Position, Piece> cells_;
};

}
