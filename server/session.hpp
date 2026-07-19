#pragma once

#include <string>

#include "model/piece.hpp"
#include "net/transport.hpp"

namespace kfc::server {

// One connected player's identity on the server: which socket they are on, which
// colour they were given, and the name they chose. Later steps add ownership
// checks and disconnect state around this same identity.
struct Session {
    net::ConnectionId connection;
    model::Color color;
    std::string name;
};

}
