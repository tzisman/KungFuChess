#pragma once

#include <string>

#include "model/position.hpp"
#include "product/move_log.hpp"

namespace kfc::io {

// The text of a square, e.g. "e4".
std::string squareName(model::Position cell, int boardHeight);

// The text of a logged action, e.g. "Nb1-c3", "e2-e4", or "Jd4" for a jump.
std::string notationOf(const product::MoveRecord& record, int boardHeight);

// A game-clock reading as "mm:ss.mmm".
std::string clockText(int ms);

}
