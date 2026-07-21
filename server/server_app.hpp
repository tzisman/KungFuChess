#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "common/logger.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "protocol/messages.hpp"
#include "server/matchmaker.hpp"
#include "server/session.hpp"
#include "server/session_manager.hpp"
#include "server/user_store.hpp"

namespace kfc::server {

// Accepts players over the transport and authenticates them against users_.
// A login only records the account behind a connection (logins_) — it grants
// no seat in any match; PlayRequest queues the connection with matchmaker_
// for that. Once seated, incoming move/jump intents are resolved to whichever
// GameSession sessions_ says the connection belongs to and handed to it
// directly. Depends only on the transport interface and these shared,
// thread-safe seams, so it can be driven by fakes in tests.
class ServerApp {
public:
    // Wires the transport's connection handlers on construction. The composition
    // root drives listen/run; the game and matchmaking loops run elsewhere.
    ServerApp(net::ServerTransport& transport, common::Logger& log,
              Matchmaker& matchmaker, SessionManager& sessions, UserStore& users);

private:
    void onOpen(net::ConnectionId id);
    void onMessage(net::ConnectionId id, const std::string& text);
    void onClose(net::ConnectionId id);
    void handleRegister(net::ConnectionId id, const protocol::RegisterRequest& request);
    void handleLogin(net::ConnectionId id, const protocol::LoginRequest& request);
    void handlePlay(net::ConnectionId id);
    void handleMove(net::ConnectionId id, const protocol::MoveIntent& intent);
    void handleJump(net::ConnectionId id, const protocol::JumpIntent& intent);

    net::ServerTransport& transport_;
    common::Logger& log_;
    Matchmaker& matchmaker_;
    SessionManager& sessions_;
    UserStore& users_;
    std::map<net::ConnectionId, Session> logins_;
};

}
