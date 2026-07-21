#pragma once

#include <optional>
#include <string>
#include <utility>

#include "img.hpp"
#include "input/board_mapper.hpp"
#include "input/command_sink.hpp"
#include "input/controller.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "view/board_geometry.hpp"
#include "view/panel_layout.hpp"
#include "view/renderer.hpp"
#include "view/sprite_library.hpp"

namespace kfc::app {

// The board's pixel presentation: the picture it is drawn from, and the
// panel layout measured from that picture's size. Rebuilt as one unit
// whenever the display size changes — every other on-screen element (lobby,
// room prompt, and, once a match's grid is known, the game view) is sized
// from it, so nothing downstream keeps a pixel count of its own.
struct Presentation {
    Img boardImage;
    view::PanelLayout layout;
};

Presentation buildPresentation(const std::string& boardImagePath,
                               int targetSize);

// The board-and-sprite half of the presentation. It additionally needs the
// match's grid dimensions, so it can only be built once those are known
// (after the first snapshot), and is rebuilt alongside Presentation whenever
// the display is resized thereafter.
//
// Deliberately immovable: Renderer holds a reference to this object's own
// SpriteLibrary, so relocating a GameView (a copy or a move) would leave that
// reference dangling. Build a new one in place — e.g. via
// std::optional<GameView>::emplace — rather than ever assigning one.
struct GameView {
    GameView(const std::string& boardImagePath, const std::string& piecesRoot,
             const Presentation& presentation, int boardCols, int boardRows);

    GameView(const GameView&) = delete;
    GameView& operator=(const GameView&) = delete;
    GameView(GameView&&) = delete;
    GameView& operator=(GameView&&) = delete;

    view::BoardGeometry geometry;
    view::SpriteLibrary sprites;
    view::Renderer renderer;
};

// The one place the click-to-command path is wired, shared by both composition
// roots so they cannot drift in how a pixel reaches the game. The command sink
// is supplied by the caller and must outlive the returned controller. myColor
// is left unset offline; the networked client passes its assigned colour, or
// leaves it unset with interactive=false for a spectator (who has no colour
// but, unlike offline hotseat play, must not be able to move anything).
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
