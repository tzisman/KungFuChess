#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "common/logger.hpp"
#include "model/board.hpp"
#include "net/transport.hpp"
#include "realtime/motion.hpp"
#include "server/game_session.hpp"
#include "server/user_store.hpp"

namespace kfc::server {

using MatchId = std::uint64_t;

// Registry and lifecycle manager for concurrent GameSessions. createSession
// runs each match on its own thread; that thread erases its own entry from
// sessions_ right before it returns, so a match that ends naturally (forfeit
// or king capture) cleans itself up without anyone polling from outside.
// Threads stay joinable rather than detached: the destructor stops whatever
// is still running and joins every thread it ever started, so a
// SessionManager never outlives the game threads it owns.
class SessionManager {
public:
    SessionManager(net::ServerTransport& transport, UserStore& users, common::Logger& log);
    ~SessionManager();

    MatchId createSession(model::Board board, realtime::MotionProfiles profiles = {});

    // Stops the session and removes it. Safe to call twice, or after the
    // session has already cleaned itself up: removal is idempotent.
    void destroySession(MatchId id);

    GameSession* find(MatchId id);
    GameSession* sessionFor(net::ConnectionId connection);
    std::optional<MatchId> matchIdFor(net::ConnectionId connection) const;

private:
    net::ServerTransport& transport_;
    UserStore& users_;
    common::Logger& log_;
    mutable std::mutex mutex_;
    std::map<MatchId, std::shared_ptr<GameSession>> sessions_;
    std::vector<std::thread> threads_;
    MatchId nextId_ = 1;
};

}
