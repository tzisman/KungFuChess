#pragma once

#include <string>
#include <vector>

namespace kfc::logic {

class Piece;

struct Position {
    int row;
    int col;
};

bool operator==(Position a, Position b);

using Row = std::vector<std::string>;

class Board {
public:
    explicit Board(std::vector<Row> rows);

    int height() const;
    int width() const;

    bool inBounds(Position p) const;
    bool isEmpty(Position p) const;
    bool sameColor(Position a, Position b) const;

    const Piece& pieceAt(Position p) const;
    char colorAt(Position p) const;
    const std::string& tokenAt(Position p) const;
    bool isRoyalAt(Position p) const;

    void movePiece(Position from, Position to);
    void removePiece(Position p);
    void transformPiece(Position p, char newSymbol);

private:
    const std::string& at(Position p) const;

    std::vector<Row> rows_;
};

}
