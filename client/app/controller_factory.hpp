#pragma once

#include <optional>
#include <utility>

#include "input/board_mapper.hpp"
#include "input/command_sink.hpp"
#include "input/controller.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "view/board_geometry.hpp"

namespace kfc::app {

// The one place the click-to-command path is wired, shared by every
// composition root so they cannot drift in how a pixel reaches the game. The
// command sink is supplied by the caller and must outlive the returned
// controller. myColor is left unset offline; the networked client passes its
// assigned colour, or leaves it unset with interactive=false for a spectator
// (who has no colour but, unlike offline hotseat play, must not be able to
// move anything).
//
// It sits apart from composition.hpp because wiring a controller needs only a
// board geometry, never a drawn pixel: keeping it here lets the text pipeline
// — which links no image backend at all — share this wiring without being
// dragged through the display headers that building a Presentation requires.
inline input::Controller makeController(
    const model::Board& board, input::CommandSink& commands,
    view::BoardGeometry geometry,
    std::optional<model::Color> myColor = std::nullopt,
    bool interactive = true) {
    return input::Controller{board, commands,
                             input::BoardMapper{std::move(geometry)}, myColor,
                             interactive};
}

}
