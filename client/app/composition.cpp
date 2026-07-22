#include "app/composition.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace kfc::app {
namespace {

// Below this the board's cells round down to nothing and the sprite library
// has no pixels left to scale into, so a window dragged smaller than this
// stops shrinking the view and the toolkit scales the frame down instead.
constexpr int kMinBoardSize = 240;

Presentation buildPresentationOn(const std::string& boardImagePath,
                                 int boardSize, view::WindowSize canvas) {
    Img boardImage;
    boardImage.read(boardImagePath, {boardSize, boardSize},
                    /*keep_aspect=*/true);
    view::PanelLayout layout{boardImage.get_mat().cols,
                             boardImage.get_mat().rows, canvas.width,
                             canvas.height};
    return {std::move(boardImage), std::move(layout)};
}

}  // namespace

Presentation buildPresentation(const std::string& boardImagePath,
                               int targetSize) {
    return buildPresentationOn(boardImagePath, targetSize, {0, 0});
}

// Every canvas dimension is a fixed multiple of the board's, so the ratio is
// measured once from a reference layout and the board scaled by whichever axis
// runs out of room first. The canvas itself is then laid out at the window's
// full size rather than the board's, so what is drawn covers the window
// exactly: the toolkit has nothing left to rescale, and a click therefore
// arrives at the pixel it was aimed at.
Presentation buildPresentationToFit(const std::string& boardImagePath,
                                    view::WindowSize content) {
    view::PanelLayout reference{content.height, content.height};
    double scale =
        std::min(static_cast<double>(content.width) / reference.canvasWidth(),
                 static_cast<double>(content.height) / reference.canvasHeight());
    int boardSize = std::max(
        kMinBoardSize, static_cast<int>(std::lround(content.height * scale)));

    return buildPresentationOn(boardImagePath, boardSize, content);
}

GameView::GameView(const std::string& boardImagePath,
                   const std::string& piecesRoot,
                   const Presentation& presentation, int boardCols,
                   int boardRows)
    : geometry(presentation.boardImage.get_mat().cols,
              presentation.boardImage.get_mat().rows, boardCols, boardRows,
              presentation.layout.boardOrigin()),
      sprites(piecesRoot, geometry.cellWidth(), geometry.cellHeight()),
      renderer(boardImagePath, sprites, geometry, presentation.layout) {}

}
