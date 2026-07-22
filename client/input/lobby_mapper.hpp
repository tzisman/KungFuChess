#pragma once

#include "view/screens/screen_layout.hpp"

namespace kfc::input {

enum class LobbyAction { kNone, kPlay, kEnterRoom };

// Turns a pixel the user clicked into a lobby action, using the same button
// geometry the lobby was drawn with. kNone if the click landed on neither
// button.
LobbyAction hitTest(const view::LobbyLayout& layout, int x, int y);

}
