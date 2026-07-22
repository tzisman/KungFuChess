#include "view/text.hpp"

#include <opencv2/imgproc.hpp>

namespace kfc::view {
namespace {

constexpr int kFont = cv::FONT_HERSHEY_SIMPLEX;

// A capital measured at scale 1.0, so a wanted height can be turned into a
// scale. Digits and capitals share this height in the font, so any of them
// would do; measuring the text itself would make labels with no tall letters
// come out larger than their neighbours.
constexpr char kHeightSample[] = "0";

}  // namespace

double scaleForHeight(int textHeight, int thickness) {
    int baseline = 0;
    cv::Size unit = cv::getTextSize(kHeightSample, kFont, 1.0, thickness, &baseline);
    return static_cast<double>(textHeight) / unit.height;
}

int widthOf(const std::string& text, double scale, int thickness) {
    int baseline = 0;
    return cv::getTextSize(text, kFont, scale, thickness, &baseline).width;
}

void drawText(Img& canvas, const std::string& text, int x, int baselineY,
              double scale, const cv::Scalar& colour, int thickness) {
    canvas.put_text(text, x, baselineY, scale, colour, thickness);
}

void drawCentredText(Img& canvas, const std::string& text, int centreX,
                     int baselineY, double scale, const cv::Scalar& colour,
                     int thickness) {
    drawText(canvas, text, centreX - widthOf(text, scale, thickness) / 2,
             baselineY, scale, colour, thickness);
}

void drawRightAlignedText(Img& canvas, const std::string& text, int rightX,
                          int baselineY, double scale, const cv::Scalar& colour,
                          int thickness) {
    drawText(canvas, text, rightX - widthOf(text, scale, thickness), baselineY,
             scale, colour, thickness);
}

}
