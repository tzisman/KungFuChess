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

// The screens outside the game are built from almost no colour at all: a
// near-white ground, white surfaces raised off it by a hairline, near-black
// text with a grey for anything secondary — and one accent. Because the accent
// is the only saturated thing on the screen, whatever carries it reads as the
// thing to do next without needing to be loud about it.
inline const cv::Scalar kScreenBackdrop{251, 250, 248, 255};
inline const cv::Scalar kSurfaceColour{255, 255, 255, 255};
inline const cv::Scalar kHairlineColour{233, 230, 226, 255};
inline const cv::Scalar kTextStrongColour{38, 36, 33, 255};
inline const cv::Scalar kTextMutedColour{145, 140, 133, 255};
inline const cv::Scalar kAccentColour{235, 99, 37, 255};
inline const cv::Scalar kAccentWashColour{254, 238, 230, 255};
inline const cv::Scalar kOnAccentColour{255, 255, 255, 255};

}
