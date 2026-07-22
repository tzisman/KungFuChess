#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>

#include "common/logger.hpp"
#include "net/websocketpp_transport.hpp"
#include "product/game_state_view.hpp"
#include "protocol/messages.hpp"

namespace kfc::app {

// Owns every callback the transport fires and every piece of state the server
// pushes asynchronously (auth answers, matchmaking/room answers, disconnect
// countdown ticks, game snapshots), so nothing above this class ever touches a
// mutex or a raw transport callback. Registers onOpen/onMessage/onClose once,
// from its constructor; every accessor below is safe to call from the main
// thread while the network thread keeps calling into it from the transport's
// io loop.
class NetworkSession {
public:
    NetworkSession(net::WebsocketppClient& transport, common::Logger& log);

    // Auth: call resetAuth() before each register/login attempt, then poll
    // the getters below until one of them, or closed(), has an answer.
    void resetAuth();
    std::optional<int> loginRating() const;
    std::optional<std::string> authRejection() const;
    bool registered() const;
    bool closed() const;

    // Matchmaking / rooms: each take* consumes the answer it returns, so a
    // caller polling every frame sees it exactly once.
    std::optional<protocol::Matched> takeMatched();
    bool takeNoOpponent();
    std::optional<protocol::RoomJoined> takeRoomJoined();

    // The disconnect-forfeit countdown, or nullopt once no tick has arrived
    // recently enough for it to still be current.
    std::optional<int> countdownSecondsLeft() const;

    // The most recent broadcast game state, if one has arrived yet.
    std::optional<product::GameStateView> latestSnapshot() const;

private:
    void handleMessage(const std::string& text);

    common::Logger& log_;

    mutable std::mutex mutex_;
    std::optional<int> rating_;
    std::optional<std::string> authRejection_;
    bool registered_ = false;
    bool closed_ = false;
    std::optional<protocol::Matched> matched_;
    bool noOpponent_ = false;
    std::optional<protocol::RoomJoined> roomJoined_;
    std::optional<int> countdownSecondsLeft_;
    std::chrono::steady_clock::time_point countdownLastUpdate_;
    std::optional<product::GameStateView> latestSnapshot_;
};

}
