#include "input/lobby_mapper.hpp"

namespace kfc::input {
namespace {

bool contains(const view::ScreenRect& rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.width && y >= rect.y &&
           y < rect.y + rect.height;
}

}  // namespace

LobbyAction hitTest(const view::LobbyLayout& layout, int x, int y) {
    if (contains(layout.play(), x, y)) return LobbyAction::kPlay;
    if (contains(layout.enterRoom(), x, y)) return LobbyAction::kEnterRoom;
    return LobbyAction::kNone;
}

}
