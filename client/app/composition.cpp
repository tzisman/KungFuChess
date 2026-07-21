#include "app/composition.hpp"

#include <utility>

namespace kfc::app {

Presentation buildPresentation(const std::string& boardImagePath,
                               int targetSize) {
    Img boardImage;
    boardImage.read(boardImagePath, {targetSize, targetSize},
                    /*keep_aspect=*/true);
    view::PanelLayout layout{boardImage.get_mat().cols,
                             boardImage.get_mat().rows};
    return {std::move(boardImage), std::move(layout)};
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
