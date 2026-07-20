#pragma once

#include <ostream>

#include "io/board_parser.hpp"
#include "model/board.hpp"

namespace kfc::io {

void printBoard(const model::Board& board, std::ostream& out);
void printParseError(const ParseError& error, std::ostream& out);

}
