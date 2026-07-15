#pragma once

#include <string>

#include "img.hpp"
#include "view/board_geometry.hpp"
#include "view/game_snapshot.hpp"
#include "view/sprite_library.hpp"

namespace kfc::view {

// Draws a snapshot onto a fresh board image. It decides nothing about the
// game: it reads the state it is handed and paints it. Every call produces a
// new image, so a rendered frame is never mutated after the fact.
class Renderer {
public:
    Renderer(std::string boardImagePath, const SpriteLibrary& sprites,
             BoardGeometry geometry);

    Img render(const GameSnapshot& snapshot) const;

private:
    static Animation animationFor(model::PieceState state);
    void drawPiece(Img& canvas, const PieceView& piece) const;
    void drawSelection(Img& canvas, model::Position cell) const;

    std::string boardImagePath_;
    const SpriteLibrary& sprites_;
    BoardGeometry geometry_;
};

}
