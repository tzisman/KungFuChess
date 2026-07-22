#include "view/screens/lobby_renderer.hpp"

#include <string>

#include "view/screens/rating_tier.hpp"
#include "view/screens/screen_chrome.hpp"
#include "view/text.hpp"
#include "view/theme.hpp"

namespace kfc::view {
namespace {

constexpr char kPlayLabel[] = "PLAY";
constexpr char kEnterRoomLabel[] = "ENTER ROOM";
constexpr char kRatingLabel[] = "ELO";

constexpr double kButtonTextHeightFraction = 0.3;
constexpr double kCardPaddingFraction = 0.18;
constexpr double kRatingTextHeightFraction = 0.36;
constexpr double kLabelBaselineFraction = 0.34;
constexpr double kRatingBaselineFraction = 0.85;
constexpr double kPillHeightFraction = 0.34;
constexpr double kPillPaddingFraction = 0.7;

int fractionOf(const ScreenRect& box, double fraction) {
    return static_cast<int>(box.height * fraction);
}

int lineIn(const ScreenRect& box, double fraction) {
    return box.y + static_cast<int>(box.height * fraction);
}

}  // namespace

LobbyRenderer::LobbyRenderer(int canvasWidth, int canvasHeight)
    : layout_(canvasWidth, canvasHeight) {}

Img LobbyRenderer::render(const LobbyFrame& frame) const {
    Img canvas = blankScreen(layout_.metrics());
    drawTitle(canvas, layout_.metrics());

    if (frame.rating) drawStandingCard(canvas, *frame.rating);
    drawPrimaryButton(canvas, layout_.play(), kPlayLabel);
    drawSecondaryButton(canvas, layout_.enterRoom(), kEnterRoomLabel);
    if (frame.statusMessage) {
        drawStatus(canvas, layout_.metrics(), *frame.statusMessage);
    }
    return canvas;
}

// A white surface lifted off the ground by a hairline, carrying the rating as
// the one large thing on it and the belt as a quiet pill beside it: what the
// number means is never further away than the number itself.
void LobbyRenderer::drawStandingCard(Img& canvas, int rating) const {
    const ScreenRect& card = layout_.card();
    const ScreenMetrics& metrics = layout_.metrics();
    cv::Rect box{card.x, card.y, card.width, card.height};

    fillRounded(canvas, box, metrics.cornerRadius(), kSurfaceColour);
    strokeRounded(canvas, box, metrics.cornerRadius(), kHairlineColour,
                  metrics.borderThickness());

    int padding = fractionOf(card, kCardPaddingFraction);
    drawText(canvas, kRatingLabel, card.x + padding,
             lineIn(card, kLabelBaselineFraction),
             scaleForHeight(metrics.labelTextHeight(), kTextThin),
             kTextMutedColour, kTextThin);
    drawText(canvas, std::to_string(rating), card.x + padding,
             lineIn(card, kRatingBaselineFraction),
             scaleForHeight(fractionOf(card, kRatingTextHeightFraction), kTextBold),
             kTextStrongColour, kTextBold);
    drawBeltPill(canvas, beltFor(rating));
}

void LobbyRenderer::drawBeltPill(Img& canvas, const std::string& belt) const {
    const ScreenRect& card = layout_.card();
    int height = fractionOf(card, kPillHeightFraction);
    int textHeight = height / 2;
    double scale = scaleForHeight(textHeight, kTextThin);
    int width = widthOf(belt, scale, kTextThin) +
                2 * static_cast<int>(height * kPillPaddingFraction);

    cv::Rect pill{card.x + card.width - fractionOf(card, kCardPaddingFraction) - width,
                  card.y + (card.height - height) / 2, width, height};
    // A radius of half its height rounds the ends fully, which is what makes a
    // pill read as a label rather than as another button.
    fillRounded(canvas, pill, height / 2, kAccentWashColour);
    drawCentredText(canvas, belt, pill.x + pill.width / 2,
                    pill.y + (pill.height + textHeight) / 2, scale, kAccentColour,
                    kTextThin);
}

// The two buttons are told apart by weight: the one that starts a game is the
// only filled thing on the screen, the one that leads elsewhere is a plain
// surface like the card above it.
void LobbyRenderer::drawPrimaryButton(Img& canvas, const ScreenRect& box,
                                      const std::string& label) const {
    fillRounded(canvas, {box.x, box.y, box.width, box.height},
                layout_.metrics().cornerRadius(), kAccentColour);
    drawButtonLabel(canvas, box, label, kOnAccentColour);
}

void LobbyRenderer::drawSecondaryButton(Img& canvas, const ScreenRect& box,
                                        const std::string& label) const {
    cv::Rect rect{box.x, box.y, box.width, box.height};
    fillRounded(canvas, rect, layout_.metrics().cornerRadius(), kSurfaceColour);
    strokeRounded(canvas, rect, layout_.metrics().cornerRadius(), kHairlineColour,
                  layout_.metrics().borderThickness());
    drawButtonLabel(canvas, box, label, kTextStrongColour);
}

void LobbyRenderer::drawButtonLabel(Img& canvas, const ScreenRect& box,
                                    const std::string& label,
                                    const cv::Scalar& colour) const {
    int textHeight = fractionOf(box, kButtonTextHeightFraction);
    drawCentredText(canvas, label, box.x + box.width / 2,
                    box.y + (box.height + textHeight) / 2,
                    scaleForHeight(textHeight, kTextBold), colour, kTextBold);
}

}
