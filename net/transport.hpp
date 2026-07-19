#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace kfc::net {

// A stable handle for one client connection, assigned by the transport. It hides
// the library's own socket handle so nothing above net speaks the library's
// types.
using ConnectionId = std::uint64_t;

// The server side of the transport, behind which the concrete WebSocket library
// lives. Everything above net (server, protocol) depends only on this seam, so
// the library can be swapped by editing one implementation file. It carries only
// connection ids and strings — no knowledge of the game.
class ServerTransport {
public:
    using OpenHandler = std::function<void(ConnectionId)>;
    using MessageHandler = std::function<void(ConnectionId, const std::string&)>;
    using CloseHandler = std::function<void(ConnectionId)>;

    virtual ~ServerTransport() = default;

    virtual void onOpen(OpenHandler handler) = 0;
    virtual void onMessage(MessageHandler handler) = 0;
    virtual void onClose(CloseHandler handler) = 0;

    virtual void send(ConnectionId connection, const std::string& message) = 0;
    virtual void broadcast(const std::string& message) = 0;

    virtual void listen(std::uint16_t port) = 0;
    virtual void run() = 0;
    virtual void stop() = 0;
};

}
