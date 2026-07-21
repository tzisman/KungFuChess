#pragma once

#include <string>

#include "img.hpp"
#include "view/lobby_layout.hpp"
#include "view/lobby_snapshot.hpp"

namespace kfc::view {

// Draws the lobby screen: a PLAY button, an "enter room" button, and an
// optional status line. Display only — it decides nothing about what a click
// means; input::hitTest (given the same layout()) does that.
class LobbyRenderer {
public:
    LobbyRenderer(int canvasWidth, int canvasHeight);

    Img render(const LobbyFrame& frame) const;
    const LobbyLayout& layout() const { return layout_; }

private:
    void drawButton(Img& canvas, const LobbyButtonRect& rect,
                    const std::string& label) const;
    void drawStatus(Img& canvas, const std::string& message) const;
    void drawRating(Img& canvas, int rating) const;

    LobbyLayout layout_;
};

}
