#include "view/panel_layout.hpp"

namespace kfc::view {
namespace {

constexpr double kPanelWidthFraction = 0.42;
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
    : boardWidth_(boardWidth),
      boardHeight_(boardHeight),
      panelWidth_(static_cast<int>(boardWidth * kPanelWidthFraction)) {}

int PanelLayout::canvasWidth() const { return boardWidth_ + 2 * panelWidth_; }

int PanelLayout::canvasHeight() const { return boardHeight_; }

Pixel PanelLayout::boardOrigin() const { return {panelWidth_, 0}; }

Pixel PanelLayout::leftPanelOrigin() const { return {0, 0}; }

Pixel PanelLayout::rightPanelOrigin() const {
    return {panelWidth_ + boardWidth_, 0};
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
    return static_cast<int>(lineHeight() * kTextHeightFraction);
}

// The baseline a line of text sits on, so line zero clears the panel's top edge
// rather than being clipped by it.
int PanelLayout::lineY(int line) const { return (line + 1) * lineHeight(); }

int PanelLayout::timeColumnX() const {
    return static_cast<int>(panelWidth_ * kTimeColumnFraction);
}

int PanelLayout::moveColumnX() const {
    return static_cast<int>(panelWidth_ * kMoveColumnFraction);
}

}
