#include "view/lobby_renderer.hpp"

#include <opencv2/imgproc.hpp>

namespace kfc::view {
namespace {

const cv::Scalar kCanvasColour{240, 240, 240, 255};
const cv::Scalar kButtonColour{200, 160, 90, 255};
const cv::Scalar kButtonOutlineColour{20, 20, 20, 255};
const cv::Scalar kButtonTextColour{20, 20, 20, 255};
const cv::Scalar kStatusTextColour{40, 40, 200, 255};
constexpr int kButtonOutlineThickness = 2;
constexpr int kTextFont = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kButtonTextScale = 0.7;
constexpr int kButtonTextThickness = 2;
constexpr double kStatusTextScale = 0.6;
constexpr int kStatusTextThickness = 1;
constexpr int kStatusMarginBelowButtons = 40;
const cv::Scalar kRatingTextColour{20, 20, 20, 255};
constexpr double kRatingTextScale = 0.6;
constexpr int kRatingTextThickness = 1;
constexpr int kRatingMarginX = 20;
constexpr int kRatingMarginY = 30;

constexpr char kPlayLabel[] = "PLAY";
constexpr char kEnterRoomLabel[] = "ENTER ROOM";
constexpr char kRatingLabel[] = "ELO: ";

}  // namespace

LobbyRenderer::LobbyRenderer(int canvasWidth, int canvasHeight)
    : layout_(canvasWidth, canvasHeight) {}

Img LobbyRenderer::render(const LobbyFrame& frame) const {
    Img canvas;
    canvas.blank(layout_.canvasWidth(), layout_.canvasHeight(), kCanvasColour);

    if (frame.rating) drawRating(canvas, *frame.rating);
    drawButton(canvas, layout_.play(), kPlayLabel);
    drawButton(canvas, layout_.enterRoom(), kEnterRoomLabel);
    if (frame.statusMessage) drawStatus(canvas, *frame.statusMessage);
    return canvas;
}

void LobbyRenderer::drawButton(Img& canvas, const LobbyButtonRect& rect,
                               const std::string& label) const {
    cv::Mat pixels = canvas.get_mat();
    cv::Rect box{rect.x, rect.y, rect.width, rect.height};
    cv::rectangle(pixels, box, kButtonColour, cv::FILLED);
    cv::rectangle(pixels, box, kButtonOutlineColour, kButtonOutlineThickness);

    int baseline = 0;
    cv::Size text = cv::getTextSize(label, kTextFont, kButtonTextScale,
                                    kButtonTextThickness, &baseline);
    int x = rect.x + (rect.width - text.width) / 2;
    int y = rect.y + (rect.height + text.height) / 2;
    canvas.put_text(label, x, y, kButtonTextScale, kButtonTextColour, kButtonTextThickness);
}

void LobbyRenderer::drawRating(Img& canvas, int rating) const {
    canvas.put_text(std::string(kRatingLabel) + std::to_string(rating), kRatingMarginX,
                    kRatingMarginY, kRatingTextScale, kRatingTextColour, kRatingTextThickness);
}

void LobbyRenderer::drawStatus(Img& canvas, const std::string& message) const {
    int baseline = 0;
    cv::Size text = cv::getTextSize(message, kTextFont, kStatusTextScale,
                                    kStatusTextThickness, &baseline);
    int x = (layout_.canvasWidth() - text.width) / 2;
    int y = layout_.enterRoom().y + layout_.enterRoom().height + kStatusMarginBelowButtons;
    canvas.put_text(message, x, y, kStatusTextScale, kStatusTextColour, kStatusTextThickness);
}

}
