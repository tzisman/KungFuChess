#pragma once

#include <optional>

#include "input/board_mapper.hpp"
#include "input/command_sink.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::input {

class Controller {
public:
    // myColor restricts what a first click may select, for online play where a
    // player should only ever pick up their own pieces; offline leaves it
    // unset, so either colour can be selected exactly as before.
    Controller(const model::Board& board, CommandSink& commands,
               BoardMapper mapper,
               std::optional<model::Color> myColor = std::nullopt);

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
    std::optional<model::Color> myColor_;
    std::optional<model::Position> selected_;
};

}
