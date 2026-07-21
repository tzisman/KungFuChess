#pragma once

#include <string>

#include "net/transport.hpp"

namespace kfc::server {

// One connected, logged-in account: which socket it is on and which username
// it authenticated as. Which colour (if any) it plays lives in the
// GameSession it is seated in, not here — logging in no longer implies a
// seat in any match; PlayRequest and matchmaking are what grant one.
struct Session {
    net::ConnectionId connection;
    std::string username;
};

}
