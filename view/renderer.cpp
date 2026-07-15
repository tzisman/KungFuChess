#include "view/renderer.hpp"

#include <opencv2/imgproc.hpp>

#include <utility>
#include <vector>

namespace kfc::view {
namespace {

const cv::Scalar kSelectionColour{0, 215, 255, 255};
constexpr int kSelectionThickness = 3;

}  // namespace

Renderer::Renderer(std::string boardImagePath, const SpriteLibrary& sprites,
                   BoardGeometry geometry)
    : boardImagePath_(std::move(boardImagePath)),
      sprites_(sprites),
      geometry_(std::move(geometry)) {}

// The board is drawn at exactly the size the geometry describes, so the
// picture and the cell coordinates can never disagree about how big a cell is.
Img Renderer::render(const GameSnapshot& snapshot) const {
    Img canvas;
    canvas.read(boardImagePath_,
                {geometry_.imageWidth(), geometry_.imageHeight()});
    for (const PieceView& piece : snapshot.pieces) drawPiece(canvas, piece);
    if (snapshot.selectedCell) drawSelection(canvas, *snapshot.selectedCell);
    return canvas;
}

// Img can compose images but not draw shapes, so the outline is drawn straight
// onto the pixels the canvas already owns. Copying the Mat header shares those
// pixels rather than duplicating them.
void Renderer::drawSelection(Img& canvas, model::Position cell) const {
    cv::Mat pixels = canvas.get_mat();
    Pixel at = geometry_.topLeftOf(cell);
    cv::rectangle(pixels,
                  cv::Rect(at.x, at.y, geometry_.cellWidth(),
                           geometry_.cellHeight()),
                  kSelectionColour, kSelectionThickness);
}

Animation Renderer::animationFor(model::PieceState state) {
    switch (state) {
        case model::PieceState::kMoving:
            return Animation::kMove;
        case model::PieceState::kAirborne:
            return Animation::kJump;
        case model::PieceState::kResting:
            return Animation::kLongRest;
        case model::PieceState::kIdle:
        case model::PieceState::kCaptured:
            break;
    }
    return Animation::kIdle;
}

void Renderer::drawPiece(Img& canvas, const PieceView& piece) const {
    if (piece.state == model::PieceState::kCaptured) return;

    const std::vector<Img>& frames =
        sprites_.frames(piece.kind, piece.color, animationFor(piece.state));
    if (frames.empty()) return;

    Pixel at = geometry_.topLeftOf(piece.cell);
    Img frame = frames.front();
    frame.draw_on(canvas, at.x, at.y);
}

}
