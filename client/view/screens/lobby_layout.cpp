#include "view/lobby_layout.hpp"

#include <cmath>

namespace kfc::view {
namespace {

int scale(int base, double fraction) {
    return static_cast<int>(std::lround(base * fraction));
}

// Fractions of the canvas rather than fixed pixel sizes, so the buttons grow
// and shrink along with everything else when the window is resized instead of
// staying a constant size on a canvas that changes size around them.
constexpr double kButtonWidthFraction = 0.28;
constexpr double kButtonHeightFraction = 0.12;
constexpr double kButtonGapFraction = 0.06;

}  // namespace

LobbyLayout::LobbyLayout(int canvasWidth, int canvasHeight)
    : canvasWidth_(canvasWidth), canvasHeight_(canvasHeight) {
    int buttonWidth = scale(canvasWidth, kButtonWidthFraction);
    int buttonHeight = scale(canvasHeight, kButtonHeightFraction);
    int buttonGap = scale(canvasHeight, kButtonGapFraction);

    int x = (canvasWidth - buttonWidth) / 2;
    int totalHeight = 2 * buttonHeight + buttonGap;
    int firstY = (canvasHeight - totalHeight) / 2;
    play_ = LobbyButtonRect{x, firstY, buttonWidth, buttonHeight};
    enterRoom_ =
        LobbyButtonRect{x, firstY + buttonHeight + buttonGap, buttonWidth, buttonHeight};
}

}
