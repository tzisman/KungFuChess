#include <doctest/doctest.h>

#include "view/screens/screen_layout.hpp"

using kfc::view::LobbyLayout;
using kfc::view::PromptLayout;
using kfc::view::ScreenRect;

namespace {

int bottomOf(const ScreenRect& rect) { return rect.y + rect.height; }

bool isCentredIn(const ScreenRect& rect, int canvasWidth) {
    int leftGap = rect.x;
    int rightGap = canvasWidth - (rect.x + rect.width);
    return leftGap - rightGap <= 1 && rightGap - leftGap <= 1;
}

}  // namespace

TEST_CASE("the lobby reads top to bottom: title, card, buttons, status") {
    LobbyLayout layout{1200, 800};

    CHECK(layout.metrics().contentTop() > layout.metrics().titleBaseline());
    CHECK(layout.card().y >= layout.metrics().contentTop());
    CHECK(layout.play().y > bottomOf(layout.card()));
    CHECK(layout.enterRoom().y > bottomOf(layout.play()));
    CHECK(layout.metrics().statusBaseline() > bottomOf(layout.enterRoom()));
}

TEST_CASE("the card and both buttons are centred on the canvas") {
    LobbyLayout layout{1200, 800};

    CHECK(isCentredIn(layout.card(), 1200));
    CHECK(isCentredIn(layout.play(), 1200));
    CHECK(isCentredIn(layout.enterRoom(), 1200));
}

TEST_CASE("everything stays inside the canvas") {
    LobbyLayout layout{1200, 800};

    CHECK(layout.card().x > 0);
    CHECK(bottomOf(layout.enterRoom()) < 800);
    CHECK(layout.metrics().statusBaseline() < 800);
}

// The window is resizable, so the lobby has to hold its shape at any size
// rather than only at the one it was designed against.
TEST_CASE("the lobby keeps its order on a much smaller canvas") {
    LobbyLayout small{400, 300};

    CHECK(small.play().y > bottomOf(small.card()));
    CHECK(small.enterRoom().y > bottomOf(small.play()));
    CHECK(bottomOf(small.enterRoom()) < 300);
}

TEST_CASE("the card and buttons scale with the canvas rather than being fixed") {
    LobbyLayout small{600, 400};
    LobbyLayout large{1200, 800};

    CHECK(large.card().width == 2 * small.card().width);
    CHECK(large.play().height == 2 * small.play().height);
}
TEST_CASE("the prompt reads top to bottom: label, field, hint") {
    PromptLayout layout{1200, 800};

    CHECK(layout.labelBaseline() > layout.metrics().titleBaseline());
    CHECK(layout.field().y > layout.labelBaseline());
    CHECK(layout.hintBaseline() > layout.field().y + layout.field().height);
    CHECK(layout.hintBaseline() < 800);
}

TEST_CASE("the field is centred on the canvas") {
    PromptLayout layout{1200, 800};

    CHECK(layout.field().x == 1200 / 2 - layout.field().width / 2);
}

// Both screens hang off the same metrics, so the title above them and the
// status beneath them land in the same place and switching between the two does
// not shift the frame the player is looking at.
TEST_CASE("the prompt shares the lobby's title and status lines") {
    PromptLayout prompt{1200, 800};
    LobbyLayout lobby{1200, 800};

    CHECK(prompt.metrics().titleBaseline() == lobby.metrics().titleBaseline());
    CHECK(prompt.metrics().contentTop() == lobby.metrics().contentTop());
    CHECK(prompt.metrics().statusBaseline() == lobby.metrics().statusBaseline());
}

TEST_CASE("the prompt keeps its order on a much smaller canvas") {
    PromptLayout small{400, 300};

    CHECK(small.field().y > small.labelBaseline());
    CHECK(small.hintBaseline() > small.field().y + small.field().height);
    CHECK(small.hintBaseline() < 300);
}
