#include "Board.hpp"

#include "Piece.hpp"
#include "PieceConstants.hpp"
#include "PieceRegistry.hpp"

namespace kfc::logic {

bool operator==(Position a, Position b) { return a.row == b.row && a.col == b.col; }

Board::Board(std::vector<Row> rows) : rows_(std::move(rows)) {}

int Board::height() const { return static_cast<int>(rows_.size()); }

int Board::width() const { return rows_.empty() ? 0 : static_cast<int>(rows_.front().size()); }

bool Board::inBounds(Position p) const {
    return p.row >= 0 && p.row < height() && p.col >= 0 && p.col < width();
}

bool Board::isEmpty(Position p) const { return at(p) == kEmptyCellToken; }

bool Board::sameColor(Position a, Position b) const { return colorAt(a) == colorAt(b); }

const Piece& Board::pieceAt(Position p) const { return *pieceRegistry().pieceFor(at(p)[1]); }

char Board::colorAt(Position p) const { return at(p)[0]; }

const std::string& Board::tokenAt(Position p) const { return at(p); }

bool Board::isRoyalAt(Position p) const { return !isEmpty(p) && pieceAt(p).isRoyal(); }

void Board::movePiece(Position from, Position to) {
    rows_[to.row][to.col] = rows_[from.row][from.col];
    removePiece(from);
}

void Board::removePiece(Position p) { rows_[p.row][p.col] = kEmptyCellToken; }

void Board::transformPiece(Position p, char newSymbol) {
    rows_[p.row][p.col] = std::string(1, colorAt(p)) + newSymbol;
}

const std::string& Board::at(Position p) const { return rows_[p.row][p.col]; }

}
