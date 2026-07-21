#pragma once

#include <string>

#include "model/piece.hpp"
#include "net/transport.hpp"

namespace kfc::server {

// One connected player's identity on the server: which socket they are on,
// the account they logged in as, and which colour they were given. Later
// steps add ownership checks and disconnect state around this same identity.
struct Session {
    net::ConnectionId connection;
    std::string username;
    model::Color color;
};

}
