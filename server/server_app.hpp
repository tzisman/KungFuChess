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

namespace kfc::server {

// Accepts players over the transport, gives the first two a colour (white then
// black) and turns further joiners away. It owns the roster of sessions and
// resolves incoming move/jump intents to the sender's colour before handing
// them to commands_, since it is the only place that knows which connection
// plays which colour. It depends only on the transport interface and the
// command queue, so it can be driven by a fake transport in tests.
class ServerApp {
public:
    // Wires the transport's connection handlers on construction. The composition
    // root drives listen/run; the game loop runs elsewhere.
    ServerApp(net::ServerTransport& transport, common::Logger& log,
              CommandQueue& commands);

private:
    void onOpen(net::ConnectionId id);
    void onMessage(net::ConnectionId id, const std::string& text);
    void onClose(net::ConnectionId id);
    void handleJoin(net::ConnectionId id, const std::string& name);
    void handleMove(net::ConnectionId id, const protocol::MoveIntent& intent);
    void handleJump(net::ConnectionId id, const protocol::JumpIntent& intent);
    std::optional<model::Color> freeColor() const;
    std::optional<model::Color> colorOf(net::ConnectionId id) const;

    net::ServerTransport& transport_;
    common::Logger& log_;
    CommandQueue& commands_;
    std::map<net::ConnectionId, Session> sessions_;
};

}
