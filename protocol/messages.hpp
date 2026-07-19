#pragma once

#include <string>
#include <variant>

#include "model/piece.hpp"

namespace kfc::protocol {

// Client -> Server: a player asks to join, giving the name to show.
struct JoinRequest {
    std::string name;
};

// Server -> Client: the player is in; this is the colour they play.
struct Assigned {
    model::Color color;
};

// Server -> Client: the player cannot join, and why (e.g. the game is full).
struct Rejected {
    std::string reason;
};

// One of anything that can cross the wire. Decoding yields whichever it is; the
// receiver dispatches on the alternative.
using Message = std::variant<JoinRequest, Assigned, Rejected>;

}
