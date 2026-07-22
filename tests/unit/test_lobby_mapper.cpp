#include <doctest/doctest.h>

#include "input/lobby_mapper.hpp"
#include "view/screens/screen_layout.hpp"

using kfc::input::hitTest;
using kfc::input::LobbyAction;
using kfc::view::LobbyLayout;

namespace {
// A 400x300 canvas produces a fixed, known button layout.
LobbyLayout sampleLayout() { return LobbyLayout{400, 300}; }
}  // namespace

TEST_CASE("a click inside the play button hits play") {
    LobbyLayout layout = sampleLayout();
    const auto& play = layout.play();

    CHECK(hitTest(layout, play.x + 1, play.y + 1) == LobbyAction::kPlay);
}

TEST_CASE("a click inside the enter-room button hits enter room") {
    LobbyLayout layout = sampleLayout();
    const auto& enterRoom = layout.enterRoom();

    CHECK(hitTest(layout, enterRoom.x + 1, enterRoom.y + 1) == LobbyAction::kEnterRoom);
}

TEST_CASE("a click outside both buttons hits nothing") {
    LobbyLayout layout = sampleLayout();

    CHECK(hitTest(layout, 0, 0) == LobbyAction::kNone);
}

TEST_CASE("a button's top-left corner is inside it, one pixel past its edge is outside") {
    LobbyLayout layout = sampleLayout();
    const auto& play = layout.play();

    CHECK(hitTest(layout, play.x, play.y) == LobbyAction::kPlay);
    CHECK(hitTest(layout, play.x + play.width, play.y) == LobbyAction::kNone);
    CHECK(hitTest(layout, play.x, play.y + play.height) == LobbyAction::kNone);
}

TEST_CASE("the play and enter-room buttons do not overlap") {
    LobbyLayout layout = sampleLayout();
    const auto& play = layout.play();
    const auto& enterRoom = layout.enterRoom();

    CHECK(hitTest(layout, enterRoom.x + 1, enterRoom.y + 1) != LobbyAction::kPlay);
    CHECK(hitTest(layout, play.x + 1, play.y + 1) != LobbyAction::kEnterRoom);
}
