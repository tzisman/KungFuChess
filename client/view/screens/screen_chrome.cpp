#include "view/screens/screen_chrome.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>

#include "view/text.hpp"
#include "view/theme.hpp"

namespace kfc::view {
namespace {

// Set as two runs rather than one so the last word can carry the accent: the
// game's name doubles as the only ornament either screen has.
constexpr char kTitleLead[] = "KUNG FU ";
constexpr char kTitleTail[] = "CHESS";

int clampRadius(const cv::Rect& box, int radius) {
    return std::max(0, std::min({radius, box.width / 2, box.height / 2}));
}

// The four corner centres, in the order their arcs sweep: top-left first, then
// clockwise, so an arc's start angle follows from its index.
std::array<cv::Point, 4> corners(const cv::Rect& box, int radius) {
    return {cv::Point{box.x + radius, box.y + radius},
            cv::Point{box.x + box.width - radius - 1, box.y + radius},
            cv::Point{box.x + box.width - radius - 1,
                      box.y + box.height - radius - 1},
            cv::Point{box.x + radius, box.y + box.height - radius - 1}};
}

}  // namespace

Img blankScreen(const ScreenMetrics& metrics) {
    Img canvas;
    canvas.blank(metrics.canvasWidth(), metrics.canvasHeight(), kScreenBackdrop);
    return canvas;
}

void drawTitle(Img& canvas, const ScreenMetrics& metrics) {
    double scale = scaleForHeight(metrics.titleTextHeight(), kTextBold);
    int leadWidth = widthOf(kTitleLead, scale, kTextBold);
    int tailWidth = widthOf(kTitleTail, scale, kTextBold);
    int left = metrics.centreX() - (leadWidth + tailWidth) / 2;

    drawText(canvas, kTitleLead, left, metrics.titleBaseline(), scale,
             kTextStrongColour, kTextBold);
    drawText(canvas, kTitleTail, left + leadWidth, metrics.titleBaseline(),
             scale, kAccentColour, kTextBold);
}

void drawStatus(Img& canvas, const ScreenMetrics& metrics,
                const std::string& message) {
    drawCentredText(canvas, message, metrics.centreX(), metrics.statusBaseline(),
                    scaleForHeight(metrics.bodyTextHeight(), kTextThin),
                    kTextMutedColour, kTextThin);
}

void fillRounded(Img& canvas, const cv::Rect& box, int radius,
                 const cv::Scalar& colour) {
    cv::Mat pixels = canvas.get_mat();
    int r = clampRadius(box, radius);

    cv::rectangle(pixels, cv::Rect(box.x + r, box.y, box.width - 2 * r, box.height),
                  colour, cv::FILLED);
    cv::rectangle(pixels, cv::Rect(box.x, box.y + r, box.width, box.height - 2 * r),
                  colour, cv::FILLED);
    for (const cv::Point& centre : corners(box, r)) {
        cv::circle(pixels, centre, r, colour, cv::FILLED, cv::LINE_AA);
    }
}

void strokeRounded(Img& canvas, const cv::Rect& box, int radius,
                   const cv::Scalar& colour, int thickness) {
    cv::Mat pixels = canvas.get_mat();
    int r = clampRadius(box, radius);
    int right = box.x + box.width - 1;
    int bottom = box.y + box.height - 1;

    cv::line(pixels, {box.x + r, box.y}, {right - r, box.y}, colour, thickness,
             cv::LINE_AA);
    cv::line(pixels, {box.x + r, bottom}, {right - r, bottom}, colour, thickness,
             cv::LINE_AA);
    cv::line(pixels, {box.x, box.y + r}, {box.x, bottom - r}, colour, thickness,
             cv::LINE_AA);
    cv::line(pixels, {right, box.y + r}, {right, bottom - r}, colour, thickness,
             cv::LINE_AA);

    std::array<cv::Point, 4> centre = corners(box, r);
    for (int i = 0; i < 4; ++i) {
        cv::ellipse(pixels, centre[i], {r, r}, 180.0 + i * 90.0, 0.0, 90.0,
                    colour, thickness, cv::LINE_AA);
    }
}

}
