#pragma once

#include <optional>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "model/position.hpp"

namespace kfc::input {

class Controller {
public:
    Controller(engine::GameEngine& engine, BoardMapper mapper);

    void handleClick(int x, int y);
    void handleJump(int x, int y);
    const std::optional<model::Position>& selection() const;

private:
    void handleFirstClick(std::optional<model::Position> cell);
    void handleSecondClick(std::optional<model::Position> cell);

    engine::GameEngine& engine_;
    BoardMapper mapper_;
    std::optional<model::Position> selected_;
};

}
