#pragma once

#include <optional>
#include <string>

namespace kfc::view {

// What the lobby screen shows right now: the logged-in account's rating, and
// an optional status line (e.g. "no opponent found"). Pure data, so it can be
// built and asserted on without touching OpenCV.
struct LobbyFrame {
    std::optional<int> rating;
    std::optional<std::string> statusMessage;
};

}
