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
    //
    // interactive gates all input, independently of myColor: a spectator has
    // no colour either, but unlike offline hotseat play (also no colour) a
    // spectator must not be able to select or move anything at all. Set it to
    // false for a connection with no seat in the match.
    Controller(const model::Board& board, CommandSink& commands,
               BoardMapper mapper,
               std::optional<model::Color> myColor = std::nullopt,
               bool interactive = true);

    void handleClick(int x, int y);
    void handleJump(int x, int y);
    const std::optional<model::Position>& selection() const;

    // Re-points click mapping at the board's geometry when it is displayed at
    // a new size, without disturbing the current selection.
    void setGeometry(view::BoardGeometry geometry);

private:
    void handleFirstClick(std::optional<model::Position> cell);
    void handleSecondClick(std::optional<model::Position> cell);
    bool isOwnPiece(model::Position cell) const;

    const model::Board& board_;
    CommandSink& commands_;
    BoardMapper mapper_;
    std::optional<model::Color> myColor_;
    bool interactive_;
    std::optional<model::Position> selected_;
};

}
