#pragma once

#include <string>
#include <variant>

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::protocol {

// Client -> Server: create an account with this username and password.
struct RegisterRequest {
    std::string username;
    std::string password;
};

// Client -> Server: authenticate an existing account.
struct LoginRequest {
    std::string username;
    std::string password;
};

// Server -> Client: the account was created.
struct Registered {};

// Server -> Client: registration or login failed, and why (e.g. bad
// credentials, or a username already taken).
struct AuthRejected {
    std::string reason;
};

// Server -> Client: the player is in; this is the colour they play.
struct Assigned {
    model::Color color;
};

// Server -> Client: the player cannot join, and why (e.g. the game is full).
struct Rejected {
    std::string reason;
};

// Client -> Server: move the piece standing at `from` toward `to`. The server
// resolves whose piece it actually is; the client only states the intent.
struct MoveIntent {
    model::Position from;
    model::Position to;
};

// Client -> Server: jump the piece standing at `cell` in place.
struct JumpIntent {
    model::Position cell;
};

// One of anything that can cross the wire. Decoding yields whichever it is; the
// receiver dispatches on the alternative.
using Message = std::variant<RegisterRequest, LoginRequest, Registered, AuthRejected,
                              Assigned, Rejected, MoveIntent, JumpIntent>;

}
