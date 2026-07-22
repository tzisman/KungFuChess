#pragma once

#include <string>

#include "img.hpp"
#include "view/board_geometry.hpp"
#include "view/panel_layout.hpp"
#include "view/renderer.hpp"
#include "view/resize_watcher.hpp"
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

// The presentation drawn at the largest size whose whole canvas — board,
// coordinate gutter and both panels — still fits inside content. Building to
// fit is what lets the window be left exactly where the user dragged it: the
// frame never has to be rescaled again on its way to the screen, so it stays
// crisp without the window being snapped to some size of the view's choosing.
Presentation buildPresentationToFit(const std::string& boardImagePath,
                                    view::WindowSize content);

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

}
