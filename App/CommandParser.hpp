#pragma once

#include <optional>
#include <string>
#include <variant>

#include "Business Logic/Board.hpp"

namespace kfc::app {

// Pixel size of a single board cell — a display-layer fact, kept out of Business Logic.
inline constexpr int kCellSizePixels = 100;

logic::Position pixelToCell(int x, int y);

// A structured command: pure data, coordinates already decoded to a cell.
struct ClickCommand      { logic::Position cell; };
struct JumpCommand       { logic::Position cell; };
struct WaitCommand       { long long ms; };
struct PrintBoardCommand {};

using Command = std::variant<ClickCommand, JumpCommand, WaitCommand, PrintBoardCommand>;

// Text -> structured command. Pure: no Game, no streams. Returns nullopt on
// an unrecognized command, wrong argument count, or a malformed number.
std::optional<Command> parseCommand(const std::string& line);

}
