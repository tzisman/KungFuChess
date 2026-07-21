#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "net/transport.hpp"

namespace kfc::test {

// A server transport that never touches a socket. It records what the server
// sends and broadcasts, and lets a test replay the connection lifecycle by
// hand. The tests that drive a real SessionManager thread (test_session_
// manager.cpp) read what was sent while a GameSession's own thread may still
// be writing to it, so send()/broadcast() and the snapshot accessors below
// are guarded by a mutex; the raw sent/broadcasts members stay unguarded for
// the (still-majority) single-threaded tests that only ever touch them from
// the one thread that also drives the transport.
class FakeServerTransport : public net::ServerTransport {
public:
    void onOpen(OpenHandler handler) override { open_ = std::move(handler); }
    void onMessage(MessageHandler handler) override {
        message_ = std::move(handler);
    }
    void onClose(CloseHandler handler) override { close_ = std::move(handler); }

    void send(net::ConnectionId id, const std::string& message) override {
        std::lock_guard<std::mutex> lock{mutex_};
        sent.push_back({id, message});
    }
    void broadcast(const std::string& message) override {
        std::lock_guard<std::mutex> lock{mutex_};
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

    // Thread-safe copies, for a test that reads while a real GameSession
    // thread (SessionManager) may still be sending.
    std::vector<std::pair<net::ConnectionId, std::string>> sentSnapshot() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return sent;
    }

    std::vector<std::pair<net::ConnectionId, std::string>> sent;
    std::vector<std::string> broadcasts;

private:
    mutable std::mutex mutex_;
    OpenHandler open_;
    MessageHandler message_;
    CloseHandler close_;
};

// A client transport that never touches a socket. It records what was sent,
// for a test to inspect what a CommandSink handed to the wire.
class FakeClientTransport : public net::ClientTransport {
public:
    void onOpen(OpenHandler handler) override { open_ = std::move(handler); }
    void onMessage(MessageHandler handler) override {
        message_ = std::move(handler);
    }
    void onClose(CloseHandler handler) override { close_ = std::move(handler); }

    void connect(const std::string&) override {}
    void send(const std::string& message) override { sent.push_back(message); }
    void run() override {}
    void stop() override {}

    std::vector<std::string> sent;

private:
    OpenHandler open_;
    MessageHandler message_;
    CloseHandler close_;
};

}
