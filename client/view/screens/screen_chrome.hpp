#pragma once

#include <opencv2/core.hpp>
#include <string>

#include "img.hpp"
#include "view/screens/screen_layout.hpp"

namespace kfc::view {

// The drawing both screens share, so neither carries a copy: the ground they
// are painted on, the game's name across the top, the status line at the foot,
// and the softened rectangle every surface on them is built from.

Img blankScreen(const ScreenMetrics& metrics);
void drawTitle(Img& canvas, const ScreenMetrics& metrics);
void drawStatus(Img& canvas, const ScreenMetrics& metrics,
                const std::string& message);

// OpenCV has no rounded-rectangle primitive. The radius is clamped to what the
// box can carry, so a small button rounds into a lozenge instead of
// overshooting its own edges.
void fillRounded(Img& canvas, const cv::Rect& box, int radius,
                 const cv::Scalar& colour);
void strokeRounded(Img& canvas, const cv::Rect& box, int radius,
                   const cv::Scalar& colour, int thickness);

}
