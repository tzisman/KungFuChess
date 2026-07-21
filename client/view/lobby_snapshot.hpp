#pragma once

#include <optional>
#include <string>

namespace kfc::view {

// What the lobby screen shows right now: nothing but an optional status line
// (e.g. "no opponent found"). Pure data, so it can be built and asserted on
// without touching OpenCV.
struct LobbyFrame {
    std::optional<std::string> statusMessage;
};

}
