#include "view/renderer.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <utility>
#include <vector>

#include "view/text.hpp"
#include "view/theme.hpp"

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

constexpr double kOverlayWidthFraction = 0.6;
const cv::Scalar kOverlayColour{255, 255, 255, 255};
const cv::Scalar kOverlayOutlineColour{0, 0, 200, 255};
constexpr int kOverlayThickness = 2;
constexpr int kOverlayOutlineThickness = 6;
constexpr int kOverlayMarginBelowTop = 30;

constexpr int kPanelTextThickness = 1;
constexpr char kNameLabel[] = "Name: ";
constexpr char kScoreLabel[] = "Score: ";
constexpr char kTimeHeader[] = "Time";
constexpr char kMoveHeader[] = "Move";
constexpr int kPanelRuleThickness = 1;
constexpr double kPanelRuleDropFraction = 0.35;

// Files run a, b, c... from the left-hand column; ranks count up from the
// bottom row, as they are written on a chessboard.
constexpr char kFirstFileLetter = 'a';
constexpr double kCoordHeightFraction = 0.5;
constexpr int kCoordThickness = 1;

constexpr int kBoardBorderGap = 2;
constexpr double kBoardBorderFraction = 0.16;
constexpr int kMinBoardBorderThickness = 2;

// A capital of the panel font, measured to turn a wanted text height into the
// scale the font must be drawn at.
constexpr char kFontHeightSample[] = "0";

bool isResting(model::PieceState state) {
    return state == model::PieceState::kShortResting ||
           state == model::PieceState::kLongResting;
}

}  // namespace

Renderer::Renderer(const std::string& boardImagePath,
                   const SpriteLibrary& sprites, BoardGeometry geometry,
                   PanelLayout layout)
    : sprites_(sprites),
      geometry_(std::move(geometry)),
      layout_(std::move(layout)) {
    background_.read(boardImagePath,
                     {geometry_.imageWidth(), geometry_.imageHeight()});
}

// The board is laid onto the canvas where the geometry says it sits, and every
// other measurement follows from that same geometry, so nothing here needs to
// know that anything is drawn beside the board.
Img Renderer::render(const GameSnapshot& snapshot, int nowMs,
                     const std::optional<std::string>& overlayText) const {
    Img canvas;
    canvas.blank(layout_.canvasWidth(), layout_.canvasHeight(), kCanvasColour);

    Img board = background_.clone();
    board.draw_on(canvas, geometry_.origin().x, geometry_.origin().y);
    drawBoardFrame(canvas);
    drawCoordinates(canvas);

    drawMoveTargets(canvas, snapshot.moveTargets);
    for (const PieceView& piece : snapshot.pieces) {
        drawPiece(canvas, piece, nowMs);
        if (isResting(piece.state)) drawRestBar(canvas, piece);
    }
    if (snapshot.selectedCell) drawSelection(canvas, *snapshot.selectedCell);
    drawPanels(canvas, snapshot.players);
    if (snapshot.gameOver) drawGameOver(canvas);
    if (overlayText) drawOverlayText(canvas, *overlayText);
    return canvas;
}

// Drawn just outside the board's edge, in the gutter the layout keeps for the
// coordinates, so framing the board never covers a square.
void Renderer::drawBoardFrame(Img& canvas) const {
    cv::Mat pixels = canvas.get_mat();
    int thickness =
        std::max(kMinBoardBorderThickness,
                 static_cast<int>(layout_.coordGutter() * kBoardBorderFraction));
    int inset = kBoardBorderGap + thickness / 2;

    Pixel at = geometry_.origin();
    cv::rectangle(pixels,
                  cv::Rect(at.x - inset, at.y - inset,
                           geometry_.imageWidth() + 2 * inset,
                           geometry_.imageHeight() + 2 * inset),
                  kBoardBorderColour, thickness, cv::LINE_AA);
}

// Labels are written on all four sides so a square can be read off without
// tracing back to a far corner, and each one is placed from the geometry's own
// cell positions, so they stay aligned at any board size or grid shape.
void Renderer::drawCoordinates(Img& canvas) const {
    double scale = coordFontScale();
    Pixel at = geometry_.origin();
    int half = layout_.coordGutter() / 2;
    int above = at.y - half;
    int below = at.y + geometry_.imageHeight() + half;
    int left = at.x - half;
    int right = at.x + geometry_.imageWidth() + half;

    for (int col = 0; col < geometry_.cols(); ++col) {
        int x = geometry_.topLeftOf({0, col}).x + geometry_.cellWidth() / 2;
        std::string file(1, static_cast<char>(kFirstFileLetter + col));
        drawCoordLabel(canvas, file, {x, above}, scale);
        drawCoordLabel(canvas, file, {x, below}, scale);
    }
    for (int row = 0; row < geometry_.rows(); ++row) {
        int y = geometry_.topLeftOf({row, 0}).y + geometry_.cellHeight() / 2;
        std::string rank = std::to_string(geometry_.rows() - row);
        drawCoordLabel(canvas, rank, {left, y}, scale);
        drawCoordLabel(canvas, rank, {right, y}, scale);
    }
}

void Renderer::drawCoordLabel(Img& canvas, const std::string& label,
                              Pixel centre, double scale) const {
    drawCentredText(canvas, label, centre.x,
                    centre.y + coordTextHeight() / 2, scale, kCoordTextColour,
                    kCoordThickness);
}

int Renderer::coordTextHeight() const {
    return static_cast<int>(layout_.coordGutter() * kCoordHeightFraction);
}

double Renderer::coordFontScale() const {
    return scaleForHeight(coordTextHeight(), kCoordThickness);
}

double Renderer::fontScale() const {
    return scaleForHeight(layout_.textHeight(), kPanelTextThickness);
}

void Renderer::drawLine(Img& canvas, const std::string& text, Pixel at) const {
    canvas.put_text(text, at.x, at.y, fontScale(), kPanelTextColour,
                    kPanelTextThickness);
}

void Renderer::drawPanels(Img& canvas,
                          const std::vector<PlayerView>& players) const {
    for (const PlayerView& player : players) {
        drawPanel(canvas, player,
                  player.color == model::Color::kBlack
                      ? layout_.leftPanelOrigin()
                      : layout_.rightPanelOrigin());
    }
}

void Renderer::drawPanel(Img& canvas, const PlayerView& player,
                         Pixel at) const {
    int timeX = at.x + layout_.timeColumnX();
    drawLine(canvas, std::string(kNameLabel) + player.name,
             {timeX, at.y + layout_.lineY(layout_.nameLine())});
    drawLine(canvas, std::string(kScoreLabel) + std::to_string(player.score),
             {timeX, at.y + layout_.lineY(layout_.scoreLine())});
    drawLine(canvas, kTimeHeader,
             {timeX, at.y + layout_.lineY(layout_.headerLine())});
    drawLine(canvas, kMoveHeader,
             {at.x + layout_.moveColumnX(),
              at.y + layout_.lineY(layout_.headerLine())});
    drawPanelRule(canvas, at);
    drawMoveTable(canvas, player.moves, at);
}

// Separates the column headers from the moves beneath them, so a long log
// reads as a table rather than as one unbroken run of lines.
void Renderer::drawPanelRule(Img& canvas, Pixel at) const {
    cv::Mat pixels = canvas.get_mat();
    int y = at.y + layout_.lineY(layout_.headerLine()) +
            static_cast<int>(layout_.lineHeight() * kPanelRuleDropFraction);
    int left = at.x + layout_.timeColumnX();
    int right = at.x + layout_.panelWidth() - layout_.timeColumnX();

    cv::line(pixels, cv::Point(left, y), cv::Point(right, y), kPanelRuleColour,
             kPanelRuleThickness, cv::LINE_AA);
}

// Only the newest actions fit once a game runs long, so the table shows the
// tail of the log rather than its head.
void Renderer::drawMoveTable(Img& canvas, const std::vector<MoveLine>& moves,
                             Pixel at) const {
    int visible = std::min(static_cast<int>(moves.size()),
                           layout_.maxVisibleMoves());
    int first = static_cast<int>(moves.size()) - visible;

    for (int i = 0; i < visible; ++i) {
        const MoveLine& line = moves[first + i];
        int y = at.y + layout_.lineY(layout_.firstMoveLine() + i);
        drawLine(canvas, line.time, {at.x + layout_.timeColumnX(), y});
        drawLine(canvas, line.move, {at.x + layout_.moveColumnX(), y});
    }
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
// so it stays readable over both the light and the dark squares. It is centred
// on the board rather than on the canvas, so the panels beside it do not push
// it off the game it is announcing the end of.
void Renderer::drawGameOver(Img& canvas) const {
    int baseline = 0;
    cv::Size unscaled =
        cv::getTextSize(kGameOverText, kTextFont, 1.0, kGameOverThickness,
                        &baseline);
    double scale = kGameOverWidthFraction * geometry_.imageWidth() /
                   unscaled.width;

    cv::Size text = cv::getTextSize(kGameOverText, kTextFont, scale,
                                    kGameOverThickness, &baseline);
    int x = geometry_.origin().x + (geometry_.imageWidth() - text.width) / 2;
    int y = geometry_.origin().y + (geometry_.imageHeight() + text.height) / 2;

    canvas.put_text(kGameOverText, x, y, scale, kGameOverOutlineColour,
                    kGameOverOutlineThickness);
    canvas.put_text(kGameOverText, x, y, scale, kGameOverColour,
                    kGameOverThickness);
}

// Sits above the board rather than over its centre, so it never fights
// drawGameOver for the same space when a countdown's forfeit ends the game.
void Renderer::drawOverlayText(Img& canvas, const std::string& text) const {
    int baseline = 0;
    cv::Size unscaled = cv::getTextSize(text, kTextFont, 1.0,
                                        kOverlayOutlineThickness, &baseline);
    double scale = kOverlayWidthFraction * geometry_.imageWidth() / unscaled.width;

    cv::Size sized = cv::getTextSize(text, kTextFont, scale,
                                     kOverlayOutlineThickness, &baseline);
    int x = geometry_.origin().x + (geometry_.imageWidth() - sized.width) / 2;
    int y = geometry_.origin().y + kOverlayMarginBelowTop + sized.height;

    canvas.put_text(text, x, y, scale, kOverlayOutlineColour, kOverlayOutlineThickness);
    canvas.put_text(text, x, y, scale, kOverlayColour, kOverlayThickness);
}

}
