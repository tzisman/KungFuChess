#include "view/lobby_layout.hpp"

namespace kfc::view {
namespace {
constexpr int kButtonWidth = 220;
constexpr int kButtonHeight = 60;
constexpr int kButtonGap = 30;
}  // namespace

LobbyLayout::LobbyLayout(int canvasWidth, int canvasHeight)
    : canvasWidth_(canvasWidth), canvasHeight_(canvasHeight) {
    int x = (canvasWidth - kButtonWidth) / 2;
    int totalHeight = 2 * kButtonHeight + kButtonGap;
    int firstY = (canvasHeight - totalHeight) / 2;
    play_ = LobbyButtonRect{x, firstY, kButtonWidth, kButtonHeight};
    enterRoom_ =
        LobbyButtonRect{x, firstY + kButtonHeight + kButtonGap, kButtonWidth, kButtonHeight};
}

}
