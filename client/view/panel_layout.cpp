#include "view/panel_layout.hpp"

#include <algorithm>
#include <cmath>

namespace kfc::view {
namespace {

int scale(int base, double fraction) {
    return static_cast<int>(std::lround(base * fraction));
}


constexpr double kPanelWidthFraction = 0.34;
constexpr double kCoordGutterFraction = 0.055;
constexpr int kLinesPerPanel = 24;
constexpr double kTextHeightFraction = 0.6;
constexpr double kTimeColumnFraction = 0.07;
constexpr double kMoveColumnFraction = 0.55;

constexpr int kNameLine = 0;
constexpr int kScoreLine = 1;
constexpr int kHeaderLine = 3;
constexpr int kFirstMoveLine = 4;

}  // namespace

PanelLayout::PanelLayout(int boardWidth, int boardHeight)
    : PanelLayout(boardWidth, boardHeight, 0, 0) {}

PanelLayout::PanelLayout(int boardWidth, int boardHeight, int canvasWidth,
                         int canvasHeight)
    : boardWidth_(boardWidth),
      boardHeight_(boardHeight),
      panelWidth_(scale(boardWidth, kPanelWidthFraction)),
      coordGutter_(scale(boardHeight, kCoordGutterFraction)) {
    canvasWidth_ = std::max(canvasWidth, naturalWidth());
    canvasHeight_ = std::max(canvasHeight, naturalHeight());
}

int PanelLayout::naturalWidth() const {
    return boardWidth_ + 2 * panelWidth_ + 2 * coordGutter_;
}

int PanelLayout::naturalHeight() const {
    return boardHeight_ + 2 * coordGutter_;
}

Pixel PanelLayout::margin() const {
    return {(canvasWidth_ - naturalWidth()) / 2,
            (canvasHeight_ - naturalHeight()) / 2};
}

Pixel PanelLayout::boardOrigin() const {
    Pixel at = margin();
    return {at.x + panelWidth_ + coordGutter_, at.y + coordGutter_};
}

// The panels start level with the board's top edge rather than the canvas's,
// so a panel's first line and the board's first rank read as one row.
Pixel PanelLayout::leftPanelOrigin() const {
    Pixel at = margin();
    return {at.x, at.y + coordGutter_};
}

Pixel PanelLayout::rightPanelOrigin() const {
    Pixel at = margin();
    return {at.x + panelWidth_ + boardWidth_ + 2 * coordGutter_,
            at.y + coordGutter_};
}

int PanelLayout::nameLine() const { return kNameLine; }

int PanelLayout::scoreLine() const { return kScoreLine; }

int PanelLayout::headerLine() const { return kHeaderLine; }

int PanelLayout::firstMoveLine() const { return kFirstMoveLine; }

int PanelLayout::maxVisibleMoves() const {
    return kLinesPerPanel - kFirstMoveLine;
}

int PanelLayout::lineHeight() const { return boardHeight_ / kLinesPerPanel; }

int PanelLayout::textHeight() const {
    return scale(lineHeight(), kTextHeightFraction);
}

// The baseline a line of text sits on, so line zero clears the panel's top edge
// rather than being clipped by it.
int PanelLayout::lineY(int line) const { return (line + 1) * lineHeight(); }

int PanelLayout::timeColumnX() const {
    return scale(panelWidth_, kTimeColumnFraction);
}

int PanelLayout::moveColumnX() const {
    return scale(panelWidth_, kMoveColumnFraction);
}

}
