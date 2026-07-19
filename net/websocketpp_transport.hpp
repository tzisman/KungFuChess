#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "net/transport.hpp"

namespace kfc::net {

// The one implementation of ServerTransport, backed by websocketpp + standalone
// asio. The library headers are confined to the .cpp via the pimpl below, so
// including this header pulls in none of them.
class WebsocketppServer : public ServerTransport {
public:
    WebsocketppServer();
    ~WebsocketppServer() override;

    void onOpen(OpenHandler handler) override;
    void onMessage(MessageHandler handler) override;
    void onClose(CloseHandler handler) override;

    void send(ConnectionId connection, const std::string& message) override;
    void broadcast(const std::string& message) override;

    void listen(std::uint16_t port) override;
    void run() override;
    void stop() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
