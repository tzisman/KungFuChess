#pragma once

#include <opencv2/core.hpp>

namespace kfc::view {

// The one palette every screen draws from, so the canvas, the letterbox
// padding beside it and each renderer cannot drift into slightly different
// shades of the same colour. Values are BGRA, the order OpenCV expects.
inline const cv::Scalar kCanvasColour{236, 239, 243, 255};
inline const cv::Scalar kPanelTextColour{40, 38, 35, 255};
inline const cv::Scalar kPanelRuleColour{205, 207, 212, 255};
inline const cv::Scalar kBoardBorderColour{33, 67, 101, 255};
inline const cv::Scalar kCoordTextColour{92, 95, 105, 255};

}
