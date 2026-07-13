#pragma once

#include <istream>
#include <string>
#include <vector>

#include "model/board.hpp"

namespace kfc::io {

struct ParseError {
    std::string code;
};

struct ParsedInput {
    model::Board board;
    std::vector<std::string> commands;
};

ParsedInput parseInput(std::istream& in);

}
