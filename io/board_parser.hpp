#pragma once

#include <istream>
#include <string>
#include <vector>

#include "io/parse_error.hpp"
#include "model/board.hpp"

namespace kfc::io {

struct ParsedInput {
    model::Board board;
    std::vector<std::string> commands;
};

ParsedInput parseInput(std::istream& in);

}
