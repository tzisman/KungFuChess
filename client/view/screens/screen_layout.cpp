#include "view/screens/screen_layout.hpp"

#include <algorithm>
#include <cmath>

namespace kfc::view {
namespace {

int scale(int base, double fraction) {
    return static_cast<int>(std::lround(base * fraction));
}

// Generous, and deliberately so: the space between things is what makes a
// screen with this little on it read as calm rather than as unfinished.
constexpr double kTitleBaselineFraction = 0.19;
constexpr double kTitleTextHeightFraction = 0.062;
constexpr double kContentTopFraction = 0.34;
constexpr double kStatusBaselineFraction = 0.90;
constexpr double kBodyTextHeightFraction = 0.036;
constexpr double kLabelTextHeightFraction = 0.024;
constexpr double kCornerRadiusFraction = 0.018;
constexpr double kBorderThicknessFraction = 0.0025;
constexpr int kMinBorderThickness = 1;

constexpr double kCardWidthFraction = 0.38;
constexpr double kCardHeightFraction = 0.15;
constexpr double kButtonWidthFraction = 0.30;
constexpr double kButtonHeightFraction = 0.105;
constexpr double kCardToButtonsFraction = 0.09;
constexpr double kBetweenButtonsFraction = 0.035;

constexpr double kFieldWidthFraction = 0.38;
constexpr double kFieldHeightFraction = 0.115;
constexpr double kLabelAboveFieldFraction = 0.05;
constexpr double kFieldBelowContentTopFraction = 0.22;
constexpr double kHintBelowFieldFraction = 0.09;

ScreenRect centred(const ScreenMetrics& metrics, double widthFraction, int y,
                   int height) {
    int width = scale(metrics.canvasWidth(), widthFraction);
    return {metrics.centreX() - width / 2, y, width, height};
}

}  // namespace

ScreenMetrics::ScreenMetrics(int canvasWidth, int canvasHeight)
    : canvasWidth_(canvasWidth), canvasHeight_(canvasHeight) {}

int ScreenMetrics::centreX() const { return canvasWidth_ / 2; }

int ScreenMetrics::titleBaseline() const {
    return scale(canvasHeight_, kTitleBaselineFraction);
}

int ScreenMetrics::titleTextHeight() const {
    return scale(canvasHeight_, kTitleTextHeightFraction);
}

int ScreenMetrics::contentTop() const {
    return scale(canvasHeight_, kContentTopFraction);
}

int ScreenMetrics::statusBaseline() const {
    return scale(canvasHeight_, kStatusBaselineFraction);
}

int ScreenMetrics::bodyTextHeight() const {
    return scale(canvasHeight_, kBodyTextHeightFraction);
}

int ScreenMetrics::labelTextHeight() const {
    return scale(canvasHeight_, kLabelTextHeightFraction);
}

int ScreenMetrics::cornerRadius() const {
    return scale(canvasHeight_, kCornerRadiusFraction);
}

int ScreenMetrics::borderThickness() const {
    return std::max(kMinBorderThickness,
                    scale(canvasHeight_, kBorderThicknessFraction));
}

LobbyLayout::LobbyLayout(int canvasWidth, int canvasHeight)
    : metrics_(canvasWidth, canvasHeight),
      card_(centred(metrics_, kCardWidthFraction, metrics_.contentTop(),
                    scale(canvasHeight, kCardHeightFraction))),
      play_(centred(metrics_, kButtonWidthFraction,
                    card_.y + card_.height +
                        scale(canvasHeight, kCardToButtonsFraction),
                    scale(canvasHeight, kButtonHeightFraction))),
      enterRoom_(centred(metrics_, kButtonWidthFraction,
                         play_.y + play_.height +
                             scale(canvasHeight, kBetweenButtonsFraction),
                         scale(canvasHeight, kButtonHeightFraction))) {}

PromptLayout::PromptLayout(int canvasWidth, int canvasHeight)
    : metrics_(canvasWidth, canvasHeight),
      field_(centred(metrics_, kFieldWidthFraction,
                     metrics_.contentTop() +
                         scale(canvasHeight, kFieldBelowContentTopFraction),
                     scale(canvasHeight, kFieldHeightFraction))) {}

int PromptLayout::labelBaseline() const {
    return field_.y - scale(canvasHeight(), kLabelAboveFieldFraction);
}

int PromptLayout::hintBaseline() const {
    return field_.y + field_.height +
           scale(canvasHeight(), kHintBelowFieldFraction);
}

}
