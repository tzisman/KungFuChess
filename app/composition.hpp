#pragma once

#include <utility>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "view/board_geometry.hpp"

namespace kfc::app {

// The one place the click-to-command path is wired, shared by both composition
// roots so they cannot drift in how a pixel reaches the engine.
inline input::Controller makeController(engine::GameEngine& engine,
                                        view::BoardGeometry geometry) {
    return input::Controller{engine, input::BoardMapper{std::move(geometry)}};
}

}
