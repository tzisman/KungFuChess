#pragma once

#include <utility>

#include "model/board.hpp"

namespace kfc::model {


class GameState {
public:
    explicit GameState(Board board) : board_(std::move(board)) {}

    const Board& board() const { return board_; }
    Board& board() { return board_; }

    bool isOver() const { return over_; }
    void markOver() { over_ = true; }

private:
    Board board_;
    bool over_ = false;
};

}
