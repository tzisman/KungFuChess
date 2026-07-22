#pragma once

#include <optional>
#include <string>
#include <vector>

#include "img.hpp"
#include "view/board_geometry.hpp"
#include "view/game_snapshot.hpp"
#include "view/panel_layout.hpp"
#include "view/sprite_library.hpp"

namespace kfc::view {

// Draws a snapshot onto a fresh canvas: the board where the geometry puts it,
// and a player's panel to either side of it. It decides nothing about the
// game: it reads the state it is handed and paints it, down to the wording of
// the log. Every call produces a new image, so a rendered frame is never
// mutated after the fact. nowMs is the display clock used to page looping
// animations; per-piece timing comes from the snapshot itself. overlayText,
// when present, is drawn over the board — the caller (not the renderer)
// decides what it says and when it applies, e.g. a disconnect countdown.
class Renderer {
public:
    Renderer(const std::string& boardImagePath, const SpriteLibrary& sprites,
             BoardGeometry geometry, PanelLayout layout);

    Img render(const GameSnapshot& snapshot, int nowMs,
              const std::optional<std::string>& overlayText = std::nullopt) const;

private:
    static Animation animationFor(model::PieceState state);
    void drawBoardFrame(Img& canvas) const;
    void drawCoordinates(Img& canvas) const;
    void drawCoordLabel(Img& canvas, const std::string& label, Pixel centre,
                        double scale) const;
    double coordFontScale() const;
    void drawPiece(Img& canvas, const PieceView& piece, int nowMs) const;
    Pixel centeredIn(Pixel cellTopLeft, const Img& sprite) const;
    void drawRestBar(Img& canvas, const PieceView& piece) const;
    void drawMoveTargets(Img& canvas,
                         const std::vector<model::Position>& cells) const;
    void drawSelection(Img& canvas, model::Position cell) const;
    void drawGameOver(Img& canvas) const;
    void drawOverlayText(Img& canvas, const std::string& text) const;
    void drawPanels(Img& canvas, const std::vector<PlayerView>& players) const;
    void drawPanel(Img& canvas, const PlayerView& player, Pixel at) const;
    void drawPanelRule(Img& canvas, Pixel at) const;
    void drawMoveTable(Img& canvas, const std::vector<MoveLine>& moves,
                       Pixel at) const;
    void drawLine(Img& canvas, const std::string& text, Pixel at) const;
    double fontScale() const;
    Pixel pixelOf(const PieceView& piece) const;
    int frameIndex(const PieceView& piece, Animation animation,
                   int frameCount, int nowMs) const;

    Img background_;
    const SpriteLibrary& sprites_;
    BoardGeometry geometry_;
    PanelLayout layout_;
};

}
