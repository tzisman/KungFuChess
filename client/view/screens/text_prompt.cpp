#include "view/screens/text_prompt.hpp"

#include <opencv2/imgproc.hpp>

#include "view/screens/screen_chrome.hpp"
#include "view/text.hpp"
#include "view/theme.hpp"

namespace kfc::view {
namespace {

constexpr char kHint[] = "Esc to quit";
constexpr double kFieldPaddingFraction = 0.26;
constexpr double kFieldTextHeightFraction = 0.34;
constexpr double kCaretGapFraction = 0.45;
constexpr double kCaretHeightFraction = 1.2;

}  // namespace

TextPromptRenderer::TextPromptRenderer(int canvasWidth, int canvasHeight)
    : layout_(canvasWidth, canvasHeight) {}

Img TextPromptRenderer::render(const std::string& prompt,
                               const std::string& typedSoFar) const {
    const ScreenMetrics& metrics = layout_.metrics();
    Img canvas = blankScreen(metrics);
    drawTitle(canvas, metrics);

    drawCentredText(canvas, prompt, metrics.centreX(), layout_.labelBaseline(),
                    scaleForHeight(metrics.bodyTextHeight(), kTextThin),
                    kTextMutedColour, kTextThin);
    drawField(canvas, typedSoFar);
    drawCentredText(canvas, kHint, metrics.centreX(), layout_.hintBaseline(),
                    scaleForHeight(metrics.labelTextHeight(), kTextThin),
                    kTextMutedColour, kTextThin);
    return canvas;
}

// The field is outlined in the accent rather than the hairline the other
// surfaces use: it is the only thing on the screen waiting for the player, so
// it is the only thing drawn as focused.
void TextPromptRenderer::drawField(Img& canvas,
                                   const std::string& typedSoFar) const {
    const ScreenRect& field = layout_.field();
    const ScreenMetrics& metrics = layout_.metrics();
    cv::Rect box{field.x, field.y, field.width, field.height};

    fillRounded(canvas, box, metrics.cornerRadius(), kSurfaceColour);
    strokeRounded(canvas, box, metrics.cornerRadius(), kAccentColour,
                  metrics.borderThickness());

    int padding = static_cast<int>(field.height * kFieldPaddingFraction);
    int textHeight = static_cast<int>(field.height * kFieldTextHeightFraction);
    double scale = scaleForHeight(textHeight, kTextThin);
    int baseline = field.y + (field.height + textHeight) / 2;
    drawText(canvas, typedSoFar, field.x + padding, baseline, scale,
             kTextStrongColour, kTextThin);

    int caretX = field.x + padding + widthOf(typedSoFar, scale, kTextThin) +
                 static_cast<int>(textHeight * kCaretGapFraction);
    int caretHeight = static_cast<int>(textHeight * kCaretHeightFraction);
    cv::line(canvas.get_mat(), {caretX, baseline + (caretHeight - textHeight) / 2},
             {caretX, baseline - textHeight - (caretHeight - textHeight) / 2},
             kAccentColour, metrics.borderThickness(), cv::LINE_AA);
}

}
