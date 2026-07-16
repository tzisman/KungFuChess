#pragma once

#include "model/piece.hpp"

namespace kfc::model {

// What a piece is worth to whoever captures it. The single definition of piece
// worth, so a scoreboard and anything else that weighs material can never
// disagree about it.
int costOf(PieceKind kind);

}
