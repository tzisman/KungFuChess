#pragma once

#include <optional>
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

// Server -> Client: login succeeded; this is the account's current rating.
// Logging in no longer seats a player into a match by itself — that is what
// PlayRequest is for.
struct LoggedIn {
    int rating;
};

// Client -> Server: queue for a match against a similarly-rated opponent.
struct PlayRequest {};

// Server -> Client: matched into a fresh game; this is the colour the player
// takes and the name of whoever they are playing.
struct Matched {
    model::Color color;
    std::string opponentName;
};

// Server -> Client: no opponent was found within the matchmaking timeout.
struct NoOpponent {};

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

// Server -> Client: the opponent disconnected and this many seconds remain
// before the disconnect is scored as a forfeit. A network fact, not part of
// the game-state snapshot, so it travels as its own message alongside it.
struct CountdownTick {
    int secondsLeft;
};

// A connection's seat in a room. Spectator is a network/session concept, not a
// model::Color, so it lives here rather than in Business Logic.
enum class Role { kWhite, kBlack, kSpectator };

// Client -> Server: join (or create) the named room.
struct EnterRoomRequest {
    std::string roomName;
};

// Server -> Client: the seat granted in the room just entered. `color` is
// empty when `role` is kSpectator.
struct RoomJoined {
    Role role;
    std::optional<model::Color> color;
};

// One of anything that can cross the wire. Decoding yields whichever it is; the
// receiver dispatches on the alternative.
using Message = std::variant<RegisterRequest, LoginRequest, Registered, AuthRejected,
                              LoggedIn, PlayRequest, Matched, NoOpponent, MoveIntent,
                              JumpIntent, CountdownTick, EnterRoomRequest, RoomJoined>;

}
