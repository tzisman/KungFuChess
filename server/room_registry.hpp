#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "net/transport.hpp"
#include "protocol/messages.hpp"
#include "realtime/motion.hpp"
#include "server/session_manager.hpp"

namespace kfc::server {

// Named-room seating: the first connection to enter a room name creates its
// match, later entrants join that same match. Seating reuses the exact
// GameSession::claimColor that matchmaking already uses (first free colour
// wins); anyone past White/Black becomes a spectator.
class RoomRegistry {
public:
    RoomRegistry(SessionManager& sessions, model::Board boardTemplate,
                 realtime::MotionProfiles profiles = {});

    struct Entry {
        MatchId match;
        protocol::Role role;
        std::optional<model::Color> color;
    };

    Entry enter(const std::string& roomName, net::ConnectionId connection,
                std::string username, int rating);

private:
    SessionManager& sessions_;
    model::Board boardTemplate_;
    realtime::MotionProfiles profiles_;
    std::mutex mutex_;
    std::map<std::string, MatchId> rooms_;
};

}
