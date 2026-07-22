#pragma once

#include "view/board_geometry.hpp"

namespace kfc::view {

// Where the board and the two side panels sit on the window, and where the
// lines and columns of a panel fall within it. Every measurement is derived
// from the size the board is drawn at, so the panels follow the board to any
// size and the drawing code holds no pixel counts of its own.
//
// The board is inset from everything around it by a coordinate gutter — the
// strip its file letters and rank numbers are written in. Because the inset is
// carried by boardOrigin, and both drawing and clicking read the board's place
// from that same origin, widening the gutter cannot make the two disagree
// about where a square is.
//
// Positions within a panel are relative to that panel's top-left corner; the
// panel's own place on the canvas is given by the origin accessors.
class PanelLayout {
public:
    // Laid out on the smallest canvas that holds it exactly.
    PanelLayout(int boardWidth, int boardHeight);

    // Laid out on a canvas of a given size — the window's own, so that what is
    // rendered needs no rescaling on its way to the screen and a click lands on
    // the pixel it was aimed at. Any space the board and panels do not fill is
    // split evenly around them, and a canvas smaller than they need is widened
    // to fit rather than cropping them.
    PanelLayout(int boardWidth, int boardHeight, int canvasWidth,
                int canvasHeight);

    int panelWidth() const { return panelWidth_; }
    int coordGutter() const { return coordGutter_; }
    int canvasWidth() const { return canvasWidth_; }
    int canvasHeight() const { return canvasHeight_; }

    Pixel boardOrigin() const;
    Pixel leftPanelOrigin() const;
    Pixel rightPanelOrigin() const;

    int nameLine() const;
    int scoreLine() const;
    int headerLine() const;
    int firstMoveLine() const;
    int maxVisibleMoves() const;

    int lineHeight() const;
    int textHeight() const;
    int lineY(int line) const;
    int timeColumnX() const;
    int moveColumnX() const;

private:
    int naturalWidth() const;
    int naturalHeight() const;
    Pixel margin() const;

    int boardWidth_;
    int boardHeight_;
    int panelWidth_;
    int coordGutter_;
    int canvasWidth_;
    int canvasHeight_;
};

}
