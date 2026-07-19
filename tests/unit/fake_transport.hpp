#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "net/transport.hpp"

namespace kfc::test {

// A server transport that never touches a socket. It records what the server
// sends and broadcasts, and lets a test replay the connection lifecycle by hand.
class FakeServerTransport : public net::ServerTransport {
public:
    void onOpen(OpenHandler handler) override { open_ = std::move(handler); }
    void onMessage(MessageHandler handler) override {
        message_ = std::move(handler);
    }
    void onClose(CloseHandler handler) override { close_ = std::move(handler); }

    void send(net::ConnectionId id, const std::string& message) override {
        sent.push_back({id, message});
    }
    void broadcast(const std::string& message) override {
        broadcasts.push_back(message);
    }
    void listen(std::uint16_t) override {}
    void run() override {}
    void stop() override {}

    // Drivers used by the tests.
    void connect(net::ConnectionId id) {
        if (open_) open_(id);
    }
    void receive(net::ConnectionId id, const std::string& message) {
        if (message_) message_(id, message);
    }
    void disconnect(net::ConnectionId id) {
        if (close_) close_(id);
    }

    std::vector<std::pair<net::ConnectionId, std::string>> sent;
    std::vector<std::string> broadcasts;

private:
    OpenHandler open_;
    MessageHandler message_;
    CloseHandler close_;
};

}
