#pragma once

#include <ostream>

#include "engine/game_engine.hpp"
#include "io/board_parser.hpp"

namespace kfc::io {

void printBoard(const engine::GameSnapshot& snapshot, std::ostream& out);
void printParseError(const ParseError& error, std::ostream& out);

}
