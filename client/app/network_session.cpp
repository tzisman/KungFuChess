#include "app/network_session.hpp"

#include "model/piece.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/wire_snapshot.hpp"

namespace kfc::app {
namespace {

using Clock = std::chrono::steady_clock;

// The server broadcasts a CountdownTick once a second while a disconnect
// countdown runs; if none has arrived for longer than that, the countdown is
// over (reconnect or forfeit) and countdownSecondsLeft() should stop
// reporting it.
constexpr int kCountdownStaleMs = 1500;

std::string roomRoleName(protocol::Role role) {
    switch (role) {
        case protocol::Role::kWhite: return "White";
        case protocol::Role::kBlack: return "Black";
        case protocol::Role::kSpectator: return "Spectator";
    }
    return "Spectator";
}

}  // namespace

NetworkSession::NetworkSession(net::WebsocketppClient& transport, common::Logger& log)
    : log_(log) {
    transport.onOpen([this] { log_.info("connected"); });
    transport.onClose([this] {
        log_.info("closed");
        std::lock_guard<std::mutex> lock{mutex_};
        closed_ = true;
    });
    transport.onMessage([this](const std::string& text) { handleMessage(text); });
}

void NetworkSession::handleMessage(const std::string& text) {
    if (std::optional<protocol::Message> message = protocol::decode(text)) {
        std::lock_guard<std::mutex> lock{mutex_};
        if (const auto* loggedIn = std::get_if<protocol::LoggedIn>(&*message)) {
            rating_ = loggedIn->rating;
        } else if (std::get_if<protocol::Registered>(&*message)) {
            registered_ = true;
        } else if (const auto* authRejected = std::get_if<protocol::AuthRejected>(&*message)) {
            authRejection_ = authRejected->reason;
        } else if (const auto* matched = std::get_if<protocol::Matched>(&*message)) {
            log_.info(std::string{"matched as "} + model::nameOf(matched->color) +
                      " against " + matched->opponentName);
            matched_ = *matched;
        } else if (std::get_if<protocol::NoOpponent>(&*message)) {
            log_.info("no opponent found");
            noOpponent_ = true;
        } else if (const auto* roomJoined = std::get_if<protocol::RoomJoined>(&*message)) {
            log_.info("room joined as " + roomRoleName(roomJoined->role));
            roomJoined_ = *roomJoined;
        } else if (const auto* tick = std::get_if<protocol::CountdownTick>(&*message)) {
            if (!countdownSecondsLeft_) log_.info("disconnect countdown observed");
            countdownSecondsLeft_ = tick->secondsLeft;
            countdownLastUpdate_ = Clock::now();
        }
        return;
    }
    std::optional<product::GameStateView> decoded = protocol::decodeSnapshot(text);
    if (!decoded) return;
    std::lock_guard<std::mutex> lock{mutex_};
    latestSnapshot_ = std::move(decoded);
}

void NetworkSession::resetAuth() {
    std::lock_guard<std::mutex> lock{mutex_};
    rating_.reset();
    authRejection_.reset();
    registered_ = false;
}

std::optional<int> NetworkSession::loginRating() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return rating_;
}

std::optional<std::string> NetworkSession::authRejection() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return authRejection_;
}

bool NetworkSession::registered() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return registered_;
}

bool NetworkSession::closed() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return closed_;
}

std::optional<protocol::Matched> NetworkSession::takeMatched() {
    std::lock_guard<std::mutex> lock{mutex_};
    std::optional<protocol::Matched> result = matched_;
    matched_.reset();
    return result;
}

bool NetworkSession::takeNoOpponent() {
    std::lock_guard<std::mutex> lock{mutex_};
    bool result = noOpponent_;
    noOpponent_ = false;
    return result;
}

std::optional<protocol::RoomJoined> NetworkSession::takeRoomJoined() {
    std::lock_guard<std::mutex> lock{mutex_};
    std::optional<protocol::RoomJoined> result = roomJoined_;
    roomJoined_.reset();
    return result;
}

std::optional<int> NetworkSession::countdownSecondsLeft() const {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!countdownSecondsLeft_) return std::nullopt;
    int elapsedMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - countdownLastUpdate_)
            .count());
    if (elapsedMs >= kCountdownStaleMs) return std::nullopt;
    return countdownSecondsLeft_;
}

std::optional<product::GameStateView> NetworkSession::latestSnapshot() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return latestSnapshot_;
}

}
