#include "view/renderer.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace kfc::view {
namespace {

const cv::Scalar kSelectionColour{0, 215, 255, 255};
constexpr int kSelectionThickness = 3;

}  // namespace

Renderer::Renderer(const std::string& boardImagePath,
                   const SpriteLibrary& sprites, BoardGeometry geometry)
    : sprites_(sprites), geometry_(std::move(geometry)) {
    background_.read(boardImagePath,
                     {geometry_.imageWidth(), geometry_.imageHeight()});
}

Img Renderer::render(const GameSnapshot& snapshot, int nowMs) const {
    Img canvas = background_.clone();
    for (const PieceView& piece : snapshot.pieces) {
        drawPiece(canvas, piece, nowMs);
    }
    if (snapshot.selectedCell) drawSelection(canvas, *snapshot.selectedCell);
    return canvas;
}

Animation Renderer::animationFor(model::PieceState state) {
    switch (state) {
        case model::PieceState::kMoving:
            return Animation::kMove;
        case model::PieceState::kAirborne:
            return Animation::kJump;
        case model::PieceState::kShortResting:
            return Animation::kShortRest;
        case model::PieceState::kLongResting:
            return Animation::kLongRest;
        case model::PieceState::kIdle:
        case model::PieceState::kCaptured:
            break;
    }
    return Animation::kIdle;
}

void Renderer::drawPiece(Img& canvas, const PieceView& piece,
                         int nowMs) const {
    if (piece.state == model::PieceState::kCaptured) return;

    Animation animation = animationFor(piece.state);
    const std::vector<Img>& frames =
        sprites_.frames(piece.kind, piece.color, animation);
    if (frames.empty()) return;

    Pixel at = pixelOf(piece);
    Img frame =
        frames[frameIndex(piece, animation, static_cast<int>(frames.size()),
                          nowMs)];
    frame.draw_on(canvas, at.x, at.y);
}

Pixel Renderer::pixelOf(const PieceView& piece) const {
    Pixel at = geometry_.topLeftOf(piece.cell);
    if (piece.movingTo) {
        Pixel to = geometry_.topLeftOf(*piece.movingTo);
        at.x += static_cast<int>((to.x - at.x) * piece.progress);
        at.y += static_cast<int>((to.y - at.y) * piece.progress);
    }
    return at;
}

// Looping animations page on the shared display clock; one-shot animations
// page on how long the piece has been in its state, holding the last frame.
int Renderer::frameIndex(const PieceView& piece, Animation animation,
                         int frameCount, int nowMs) const {
    const AnimationSpec& spec =
        sprites_.spec(piece.kind, piece.color, animation);
    if (spec.isLoop) {
        return static_cast<int>(nowMs * spec.framesPerSec / 1000.0) %
               frameCount;
    }
    int frame =
        static_cast<int>(piece.stateElapsedMs * spec.framesPerSec / 1000.0);
    return std::min(frame, frameCount - 1);
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

}
