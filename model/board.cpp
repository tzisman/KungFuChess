#include "model/board.hpp"

#include "model/errors.hpp"

namespace kfc::model {

Board::Board(int width, int height) : width_(width), height_(height) {}

bool Board::inBounds(Position cell) const {
    return cell.row >= 0 && cell.row < height_ &&
           cell.col >= 0 && cell.col < width_;
}

std::optional<Piece> Board::pieceAt(Position cell) const {
    auto it = cells_.find(cell);
    if (it == cells_.end()) return std::nullopt;
    return it->second;
}

void Board::addPiece(const Piece& piece) {
    requireInBounds(piece.cell());
    auto [it, inserted] = cells_.emplace(piece.cell(), piece);
    (void)it;
    if (!inserted) {
        throw CellOccupiedError("addPiece: cell is already occupied");
    }
}

void Board::removePiece(Position cell) {
    requireInBounds(cell);
    if (cells_.erase(cell) == 0) {
        throw CellEmptyError("removePiece: cell is empty");
    }
}

void Board::movePiece(Position from, Position to) {
    requireInBounds(from);
    requireInBounds(to);

    auto source = cells_.find(from);
    if (source == cells_.end()) {
        throw CellEmptyError("movePiece: source cell is empty");
    }
    if (cells_.count(to) != 0) {
        throw CellOccupiedError("movePiece: destination cell is occupied");
    }

    Piece moved = source->second;
    moved.setCell(to);
    cells_.erase(source);
    cells_.emplace(to, moved);
}

void Board::setPieceKind(Position cell, PieceKind kind) {
    requireInBounds(cell);
    auto it = cells_.find(cell);
    if (it == cells_.end()) {
        throw CellEmptyError("setPieceKind: cell is empty");
    }
    it->second.setKind(kind);
}

void Board::requireInBounds(Position cell) const {
    if (!inBounds(cell)) {
        throw OutOfBoundsError("cell is out of bounds");
    }
}

}
