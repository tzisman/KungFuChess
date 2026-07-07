#pragma once

#include <optional>
#include <vector>

#include "BoardParser.h"

namespace kfc::logic {

struct Position {
    int row;
    int col;
};

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
