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
#include "server/command_queue.hpp"
#include "server/session.hpp"
#include "server/user_store.hpp"

namespace kfc::server {

// Accepts players over the transport, authenticates them against users_,
// gives the first two logged-in accounts a colour (white then black) and
// turns further logins away. It owns the roster of sessions and resolves
// incoming move/jump intents to the sender's colour before handing them to
// commands_, since it is the only place that knows which connection plays
// which colour. It depends only on the transport interface and these shared,
// thread-safe seams, so it can be driven by a fake transport and a fake user
// store in tests.
class ServerApp {
public:
    // Wires the transport's connection handlers on construction. The composition
    // root drives listen/run; the game loop runs elsewhere.
    ServerApp(net::ServerTransport& transport, common::Logger& log,
              CommandQueue& commands, UserStore& users);

private:
    void onOpen(net::ConnectionId id);
    void onMessage(net::ConnectionId id, const std::string& text);
    void onClose(net::ConnectionId id);
    void handleRegister(net::ConnectionId id, const protocol::RegisterRequest& request);
    void handleLogin(net::ConnectionId id, const protocol::LoginRequest& request);
    void handleMove(net::ConnectionId id, const protocol::MoveIntent& intent);
    void handleJump(net::ConnectionId id, const protocol::JumpIntent& intent);
    std::optional<model::Color> freeColor() const;
    std::optional<model::Color> colorOf(net::ConnectionId id) const;

    net::ServerTransport& transport_;
    common::Logger& log_;
    CommandQueue& commands_;
    UserStore& users_;
    std::map<net::ConnectionId, Session> sessions_;
};

}
