#pragma once

#include <utility>

#include "input/board_mapper.hpp"
#include "input/command_sink.hpp"
#include "input/controller.hpp"
#include "model/board.hpp"
#include "view/board_geometry.hpp"

namespace kfc::app {

// The one place the click-to-command path is wired, shared by both composition
// roots so they cannot drift in how a pixel reaches the game. The command sink
// is supplied by the caller and must outlive the returned controller.
inline input::Controller makeController(const model::Board& board,
                                        input::CommandSink& commands,
                                        view::BoardGeometry geometry) {
    return input::Controller{board, commands,
                             input::BoardMapper{std::move(geometry)}};
}

}
