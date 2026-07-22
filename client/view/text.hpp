#pragma once

#include <opencv2/core.hpp>
#include <string>

#include "img.hpp"

namespace kfc::view {

inline constexpr int kTextThin = 1;
inline constexpr int kTextBold = 2;

// The scale a label must be drawn at to come out textHeight pixels tall. Every
// screen sizes its text this way, so a label grows with the window instead of
// staying a fixed size on a canvas that does not.
double scaleForHeight(int textHeight, int thickness);

int widthOf(const std::string& text, double scale, int thickness);

// All three take the text's baseline, and differ only in what x means: the
// label's left edge, the centre it is balanced around, or its right edge.
void drawText(Img& canvas, const std::string& text, int x, int baselineY,
              double scale, const cv::Scalar& colour, int thickness);
void drawCentredText(Img& canvas, const std::string& text, int centreX,
                     int baselineY, double scale, const cv::Scalar& colour,
                     int thickness);
void drawRightAlignedText(Img& canvas, const std::string& text, int rightX,
                          int baselineY, double scale, const cv::Scalar& colour,
                          int thickness);

}
