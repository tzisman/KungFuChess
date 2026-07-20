#pragma once

#include "view/board_geometry.hpp"

namespace kfc::view {

// Where the board and the two side panels sit on the window, and where the
// lines and columns of a panel fall within it. Every measurement is derived
// from the size the board is drawn at, so the panels follow the board to any
// size and the drawing code holds no pixel counts of its own.
//
// Positions within a panel are relative to that panel's top-left corner; the
// panel's own place on the canvas is given by the origin accessors.
class PanelLayout {
public:
    PanelLayout(int boardWidth, int boardHeight);

    int panelWidth() const { return panelWidth_; }
    int canvasWidth() const;
    int canvasHeight() const;

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
    int boardWidth_;
    int boardHeight_;
    int panelWidth_;
};

}
