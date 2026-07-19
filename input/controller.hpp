#pragma once

#include <optional>

#include "input/board_mapper.hpp"
#include "input/command_sink.hpp"
#include "model/board.hpp"
#include "model/position.hpp"

namespace kfc::input {

class Controller {
public:
    Controller(const model::Board& board, CommandSink& commands,
               BoardMapper mapper);

    void handleClick(int x, int y);
    void handleJump(int x, int y);
    const std::optional<model::Position>& selection() const;

private:
    void handleFirstClick(std::optional<model::Position> cell);
    void handleSecondClick(std::optional<model::Position> cell);
    bool isOwnPiece(model::Position cell) const;

    const model::Board& board_;
    CommandSink& commands_;
    BoardMapper mapper_;
    std::optional<model::Position> selected_;
};

}
