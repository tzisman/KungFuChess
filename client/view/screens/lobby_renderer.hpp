#pragma once

#include <optional>
#include <string>

#include "img.hpp"
#include "view/screens/screen_layout.hpp"

namespace kfc::view {

// What the lobby shows right now: the logged-in account's rating, and an
// optional status line (e.g. "no opponent found"). Pure data, so it can be
// built and asserted on without touching OpenCV.
struct LobbyFrame {
    std::optional<int> rating;
    std::optional<std::string> statusMessage;
};

// Draws the lobby screen: the player's standing, a PLAY button, an "enter
// room" button, and an optional status line. Display only — it decides nothing
// about what a click means; input::hitTest (given the same layout()) does that.
class LobbyRenderer {
public:
    LobbyRenderer(int canvasWidth, int canvasHeight);

    Img render(const LobbyFrame& frame) const;
    const LobbyLayout& layout() const { return layout_; }

private:
    void drawStandingCard(Img& canvas, int rating) const;
    void drawBeltPill(Img& canvas, const std::string& belt) const;
    void drawPrimaryButton(Img& canvas, const ScreenRect& box,
                           const std::string& label) const;
    void drawSecondaryButton(Img& canvas, const ScreenRect& box,
                             const std::string& label) const;
    void drawButtonLabel(Img& canvas, const ScreenRect& box,
                         const std::string& label,
                         const cv::Scalar& colour) const;

    LobbyLayout layout_;
};

}
