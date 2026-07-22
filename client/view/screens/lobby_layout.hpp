#pragma once

namespace kfc::view {

// A clickable rectangle in the lobby, in pixels.
struct LobbyButtonRect {
    int x;
    int y;
    int width;
    int height;
};

// Where the lobby's buttons sit, derived once from the canvas size. Shared
// between LobbyRenderer (draws them) and input::hitTest (tests clicks against
// them), so the two can never disagree about where a button is — the same
// relationship BoardGeometry holds between the board's renderer and mapper.
class LobbyLayout {
public:
    LobbyLayout(int canvasWidth, int canvasHeight);

    int canvasWidth() const { return canvasWidth_; }
    int canvasHeight() const { return canvasHeight_; }
    const LobbyButtonRect& play() const { return play_; }
    const LobbyButtonRect& enterRoom() const { return enterRoom_; }

private:
    int canvasWidth_;
    int canvasHeight_;
    LobbyButtonRect play_;
    LobbyButtonRect enterRoom_;
};

}
