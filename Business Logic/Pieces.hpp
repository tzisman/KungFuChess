#pragma once

#include "Piece.hpp"

namespace kfc::logic {

class King : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
    bool isRoyal() const override;
};

class Rook : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
};

class Bishop : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
};

class Queen : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
};

class Knight : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
};

class Pawn : public Piece {
public:
    char symbol() const override;
    bool isShapeLegal(char color, Position from, Position to, int boardHeight) const override;
    bool requiresClearPath() const override;
    bool isDestinationLegal(const Board& board, char color, Position from, Position to) const override;
    std::optional<char> promotionSymbol(char color, Position at, int boardHeight) const override;

private:
    int direction(char color) const;
    int startRow(char color, int boardHeight) const;
    int promotionRow(char color, int boardHeight) const;
};

}
