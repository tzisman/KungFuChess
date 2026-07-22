#pragma once

namespace kfc::view {

// A rectangle on a screen, in pixels: a card, a button, an input field.
struct ScreenRect {
    int x;
    int y;
    int width;
    int height;
};

// Where everything sits on the screens outside the game. Kept clear of the
// image backend on purpose: the unit tests link the logic library but never
// OpenCV, so geometry that lives here can be tested and geometry that lives in
// a renderer cannot. It is also what lets input::hitTest read a button's place
// from the very object the renderer drew it from, so the two cannot disagree.
//
// Every measurement is a fraction of the canvas, so the screens scale with the
// window instead of holding pixel counts that only suit one size of it.

// The rhythm both screens share: they hang their content off the same title
// band and write their status on the same line, so the two read as one product
// rather than as two screens that happen to follow each other.
class ScreenMetrics {
public:
    ScreenMetrics(int canvasWidth, int canvasHeight);

    int canvasWidth() const { return canvasWidth_; }
    int canvasHeight() const { return canvasHeight_; }
    int centreX() const;

    int titleBaseline() const;
    int titleTextHeight() const;

    // The first line a screen's own content may claim, clear of the title.
    int contentTop() const;
    int statusBaseline() const;

    int bodyTextHeight() const;
    int labelTextHeight() const;
    int cornerRadius() const;
    int borderThickness() const;

private:
    int canvasWidth_;
    int canvasHeight_;
};

// The lobby: the player's standing on a card, above the two buttons.
class LobbyLayout {
public:
    LobbyLayout(int canvasWidth, int canvasHeight);

    int canvasWidth() const { return metrics_.canvasWidth(); }
    int canvasHeight() const { return metrics_.canvasHeight(); }
    const ScreenMetrics& metrics() const { return metrics_; }

    const ScreenRect& card() const { return card_; }
    const ScreenRect& play() const { return play_; }
    const ScreenRect& enterRoom() const { return enterRoom_; }

private:
    ScreenMetrics metrics_;
    ScreenRect card_;
    ScreenRect play_;
    ScreenRect enterRoom_;
};

// The room prompt: a label, the field it introduces, and a hint below.
class PromptLayout {
public:
    PromptLayout(int canvasWidth, int canvasHeight);

    int canvasWidth() const { return metrics_.canvasWidth(); }
    int canvasHeight() const { return metrics_.canvasHeight(); }
    const ScreenMetrics& metrics() const { return metrics_; }

    int labelBaseline() const;
    const ScreenRect& field() const { return field_; }
    int hintBaseline() const;

private:
    ScreenMetrics metrics_;
    ScreenRect field_;
};

}
