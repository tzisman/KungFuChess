#pragma once

#include <optional>
#include <string>
#include <variant>

namespace kfc::texttests {

// Script coordinates are pixels on a fixed grid of this size. It is a
// convention of the script protocol, not a measurement of any board that gets
// displayed, which is why it lives here and not with the board itself.
inline constexpr int kCellSize = 100;

struct ClickCommand {
    int x;
    int y;
};

struct WaitCommand {
    int ms;
};

struct PrintBoardCommand {};

struct JumpCommand {
    int x;
    int y;
};

using Command =
    std::variant<ClickCommand, WaitCommand, PrintBoardCommand, JumpCommand>;

std::optional<Command> parseCommand(const std::string& line);

}
