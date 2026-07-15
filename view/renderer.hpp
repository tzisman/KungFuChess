#pragma once

#include <string>

#include "img.hpp"
#include "view/board_geometry.hpp"
#include "view/game_snapshot.hpp"
#include "view/sprite_library.hpp"

namespace kfc::view {

// Draws a snapshot onto a fresh board image. It decides nothing about the
// game: it reads the state it is handed and paints it. Every call produces a
// new image, so a rendered frame is never mutated after the fact. nowMs is the
// display clock used to page looping animations; per-piece timing comes from
// the snapshot itself.
class Renderer {
public:
    Renderer(const std::string& boardImagePath, const SpriteLibrary& sprites,
             BoardGeometry geometry);

    Img render(const GameSnapshot& snapshot, int nowMs) const;

private:
    static Animation animationFor(model::PieceState state);
    void drawPiece(Img& canvas, const PieceView& piece, int nowMs) const;
    void drawSelection(Img& canvas, model::Position cell) const;
    Pixel pixelOf(const PieceView& piece) const;
    int frameIndex(const PieceView& piece, Animation animation,
                   int frameCount, int nowMs) const;

    Img background_;
    const SpriteLibrary& sprites_;
    BoardGeometry geometry_;
};

}
