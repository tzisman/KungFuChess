#include "view/renderer.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace kfc::view {
namespace {

const cv::Scalar kSelectionColour{0, 215, 255, 255};
constexpr int kSelectionThickness = 3;

const cv::Scalar kMoveTargetColour{0, 215, 255, 255};
constexpr double kMoveTargetRadiusFraction = 0.16;
constexpr double kMoveTargetAlpha = 0.55;

const cv::Scalar kRestBarColour{90, 200, 90, 255};
const cv::Scalar kRestBarTrackColour{40, 40, 40, 255};
constexpr int kRestBarHeight = 5;
constexpr int kRestBarInset = 4;

constexpr char kGameOverText[] = "GAME OVER";
constexpr double kGameOverWidthFraction = 0.8;
const cv::Scalar kGameOverColour{255, 255, 255, 255};
const cv::Scalar kGameOverOutlineColour{0, 0, 0, 255};
constexpr int kGameOverThickness = 4;
constexpr int kGameOverOutlineThickness = 12;
constexpr int kTextFont = cv::FONT_HERSHEY_SIMPLEX;

bool isResting(model::PieceState state) {
    return state == model::PieceState::kShortResting ||
           state == model::PieceState::kLongResting;
}

}  // namespace

Renderer::Renderer(const std::string& boardImagePath,
                   const SpriteLibrary& sprites, BoardGeometry geometry)
    : sprites_(sprites), geometry_(std::move(geometry)) {
    background_.read(boardImagePath,
                     {geometry_.imageWidth(), geometry_.imageHeight()});
}

Img Renderer::render(const GameSnapshot& snapshot, int nowMs) const {
    Img canvas = background_.clone();
    drawMoveTargets(canvas, snapshot.moveTargets);
    for (const PieceView& piece : snapshot.pieces) {
        drawPiece(canvas, piece, nowMs);
        if (isResting(piece.state)) drawRestBar(canvas, piece);
    }
    if (snapshot.selectedCell) drawSelection(canvas, *snapshot.selectedCell);
    if (snapshot.gameOver) drawGameOver(canvas);
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

    Img frame =
        frames[frameIndex(piece, animation, static_cast<int>(frames.size()),
                          nowMs)];
    Pixel at = centeredIn(pixelOf(piece), frame);
    frame.draw_on(canvas, at.x, at.y);
}

// Keeping its proportions means a sprite rarely fills the cell exactly, so it
// is centred on the square instead of hanging off the top-left corner.
Pixel Renderer::centeredIn(Pixel cellTopLeft, const Img& sprite) const {
    return {cellTopLeft.x + (geometry_.cellWidth() - sprite.get_mat().cols) / 2,
            cellTopLeft.y +
                (geometry_.cellHeight() - sprite.get_mat().rows) / 2};
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

// Looping animations page on the shared display clock at their configured
// rate. A one-shot animation instead plays once across the whole state, so it
// is paced by how far through the piece reports itself to be: the state's
// duration is the rules' to decide, and the animation stretches to fill it
// rather than finishing early and freezing on its last frame.
int Renderer::frameIndex(const PieceView& piece, Animation animation,
                         int frameCount, int nowMs) const {
    const AnimationSpec& spec =
        sprites_.spec(piece.kind, piece.color, animation);
    if (spec.isLoop) {
        return static_cast<int>(nowMs * spec.framesPerSec / 1000.0) %
               frameCount;
    }
    int frame = static_cast<int>(piece.progress * frameCount);
    return std::min(frame, frameCount - 1);
}

// The bar drains as the rest runs out: the piece reports how far through it
// is, and how long a rest lasts stays a matter for the rules alone.
void Renderer::drawRestBar(Img& canvas, const PieceView& piece) const {
    cv::Mat pixels = canvas.get_mat();
    Pixel at = geometry_.topLeftOf(piece.cell);

    int width = geometry_.cellWidth() - 2 * kRestBarInset;
    int x = at.x + kRestBarInset;
    int y = at.y + geometry_.cellHeight() - kRestBarInset - kRestBarHeight;

    cv::rectangle(pixels, cv::Rect(x, y, width, kRestBarHeight),
                  kRestBarTrackColour, cv::FILLED);

    int left = static_cast<int>(width * (1.0 - piece.progress));
    if (left > 0) {
        cv::rectangle(pixels, cv::Rect(x, y, left, kRestBarHeight),
                      kRestBarColour, cv::FILLED);
    }
}

// Drawn onto a copy that is then blended back, so the dots read as translucent
// over whatever square they land on.
void Renderer::drawMoveTargets(
    Img& canvas, const std::vector<model::Position>& cells) const {
    if (cells.empty()) return;

    cv::Mat pixels = canvas.get_mat();
    cv::Mat overlay = pixels.clone();
    int radius = static_cast<int>(
        std::min(geometry_.cellWidth(), geometry_.cellHeight()) *
        kMoveTargetRadiusFraction);

    for (model::Position cell : cells) {
        Pixel at = geometry_.topLeftOf(cell);
        cv::Point centre{at.x + geometry_.cellWidth() / 2,
                         at.y + geometry_.cellHeight() / 2};
        cv::circle(overlay, centre, radius, kMoveTargetColour, cv::FILLED,
                   cv::LINE_AA);
    }

    cv::addWeighted(overlay, kMoveTargetAlpha, pixels, 1.0 - kMoveTargetAlpha,
                    0.0, pixels);
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

// Scaled to span most of the board whatever size it is drawn at, and outlined
// so it stays readable over both the light and the dark squares.
void Renderer::drawGameOver(Img& canvas) const {
    int baseline = 0;
    cv::Size unscaled =
        cv::getTextSize(kGameOverText, kTextFont, 1.0, kGameOverThickness,
                        &baseline);
    double scale = kGameOverWidthFraction * geometry_.imageWidth() /
                   unscaled.width;

    cv::Size text = cv::getTextSize(kGameOverText, kTextFont, scale,
                                    kGameOverThickness, &baseline);
    int x = (geometry_.imageWidth() - text.width) / 2;
    int y = (geometry_.imageHeight() + text.height) / 2;

    canvas.put_text(kGameOverText, x, y, scale, kGameOverOutlineColour,
                    kGameOverOutlineThickness);
    canvas.put_text(kGameOverText, x, y, scale, kGameOverColour,
                    kGameOverThickness);
}

}
