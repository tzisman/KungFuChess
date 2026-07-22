#pragma once

#include <string>

namespace kfc::view {

// The belt a rating earns, for the lobby to show beside the number itself —
// 1247 says little on its own, "Green Belt" says where its owner stands. A
// label on a screen and nothing more: no game rule reads it, and the rating it
// is read from is still the server's to compute.
std::string beltFor(int rating);

}
