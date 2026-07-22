#include "view/text_prompt.hpp"

#include <opencv2/imgproc.hpp>

#include "view/theme.hpp"

namespace kfc::view {
namespace {

const cv::Scalar kBoxColour{255, 255, 255, 255};
const cv::Scalar kBoxOutlineColour{20, 20, 20, 255};
const cv::Scalar kPromptTextColour{20, 20, 20, 255};
const cv::Scalar kTypedTextColour{20, 20, 20, 255};
constexpr int kBoxOutlineThickness = 2;
constexpr int kTextFont = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kPromptTextScale = 0.6;
constexpr int kPromptTextThickness = 1;
constexpr double kTypedTextScale = 0.7;
constexpr int kTypedTextThickness = 2;
constexpr int kBoxWidth = 240;
constexpr int kBoxHeight = 40;
constexpr int kBoxTextPadding = 10;
constexpr int kPromptMarginAboveBox = 20;
constexpr int kTypedTextBaselineNudge = 6;

}  // namespace

TextPromptRenderer::TextPromptRenderer(int canvasWidth, int canvasHeight)
    : canvasWidth_(canvasWidth), canvasHeight_(canvasHeight) {}

Img TextPromptRenderer::render(const std::string& prompt, const std::string& typedSoFar) const {
    Img canvas;
    canvas.blank(canvasWidth_, canvasHeight_, kCanvasColour);

    cv::Rect box{(canvasWidth_ - kBoxWidth) / 2, (canvasHeight_ - kBoxHeight) / 2,
                kBoxWidth, kBoxHeight};
    cv::Mat pixels = canvas.get_mat();
    cv::rectangle(pixels, box, kBoxColour, cv::FILLED);
    cv::rectangle(pixels, box, kBoxOutlineColour, kBoxOutlineThickness);

    int baseline = 0;
    cv::Size promptSize = cv::getTextSize(prompt, kTextFont, kPromptTextScale,
                                          kPromptTextThickness, &baseline);
    canvas.put_text(prompt, (canvasWidth_ - promptSize.width) / 2,
                    box.y - kPromptMarginAboveBox, kPromptTextScale, kPromptTextColour,
                    kPromptTextThickness);

    canvas.put_text(typedSoFar, box.x + kBoxTextPadding,
                    box.y + box.height / 2 + kTypedTextBaselineNudge, kTypedTextScale,
                    kTypedTextColour, kTypedTextThickness);
    return canvas;
}

}
