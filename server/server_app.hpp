#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "common/logger.hpp"
#include "model/piece.hpp"
#include "net/transport.hpp"
#include "server/session.hpp"

namespace kfc::server {

// Accepts players over the transport, gives the first two a colour (white then
// black) and turns further joiners away. It owns the roster of sessions; the
// game itself is wired in later steps. It depends only on the transport
// interface, so it can be driven by a fake transport in tests.
class ServerApp {
public:
    // Wires the transport's connection handlers on construction. The composition
    // root drives listen/run; the game loop runs elsewhere.
    ServerApp(net::ServerTransport& transport, common::Logger& log);

private:
    void onOpen(net::ConnectionId id);
    void onMessage(net::ConnectionId id, const std::string& text);
    void onClose(net::ConnectionId id);
    void handleJoin(net::ConnectionId id, const std::string& name);
    std::optional<model::Color> freeColor() const;

    net::ServerTransport& transport_;
    common::Logger& log_;
    std::map<net::ConnectionId, Session> sessions_;
};

}
