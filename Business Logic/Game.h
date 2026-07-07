#pragma once

#include <cstdlib>
#include <optional>
#include <vector>

#include "BoardParser.h"

namespace kfc::logic {

struct Position {
    int row;
    int col;
};

inline bool isShapeLegal(char piece, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    if (dr == 0 && dc == 0) return false;

    int adr = std::abs(dr);
    int adc = std::abs(dc);

    switch (piece) {
        case 'K': return adr <= 1 && adc <= 1;
        case 'R': return dr == 0 || dc == 0;
        case 'B': return adr == adc;
        case 'Q': return dr == 0 || dc == 0 || adr == adc;
        case 'N': return (adr == 1 && adc == 2) || (adr == 2 && adc == 1);
        default: return false;
    }
}

class Board {
public:
    explicit Board(std::vector<Row> rows) : rows_(std::move(rows)) {}

    int height() const { return static_cast<int>(rows_.size()); }

    int width() const { return rows_.empty() ? 0 : static_cast<int>(rows_.front().size()); }

    bool inBounds(Position p) const {
        return p.row >= 0 && p.row < height() && p.col >= 0 && p.col < width();
    }

    bool isEmpty(Position p) const { return at(p) == "."; }

    bool sameColor(Position a, Position b) const { return at(a)[0] == at(b)[0]; }

    char pieceTypeAt(Position p) const { return at(p)[1]; }

    void movePiece(Position from, Position to) {
        rows_[to.row][to.col] = rows_[from.row][from.col];
        rows_[from.row][from.col] = ".";
    }

    const std::vector<Row>& rows() const { return rows_; }

private:
    const std::string& at(Position p) const { return rows_[p.row][p.col]; }

    std::vector<Row> rows_;
};

class Game {
public:
    explicit Game(std::vector<Row> initialRows) : board_(std::move(initialRows)) {}

    void handleClickCell(Position p) {
        if (!board_.inBounds(p)) return;

        if (!selected_.has_value()) {
            if (!board_.isEmpty(p)) selected_ = p;
            return;
        }

        if (!board_.isEmpty(p) && board_.sameColor(*selected_, p)) {
            selected_ = p;
            return;
        }

        if (!isShapeLegal(board_.pieceTypeAt(*selected_), *selected_, p)) return;

        board_.movePiece(*selected_, p);
        selected_.reset();
    }

    void advanceClock(long long ms) { clockMs_ += ms; }

    const Board& board() const { return board_; }

private:
    Board board_;
    std::optional<Position> selected_;
    long long clockMs_ = 0;
};

}
