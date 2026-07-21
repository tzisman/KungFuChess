#include "server/server_app.hpp"

#include <string>
#include <variant>

#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "server/command_queue.hpp"

namespace kfc::server {

namespace {
constexpr char kInvalidCredentialsReason[] = "invalid_credentials";
constexpr char kUsernameTakenReason[] = "username_taken";
}  // namespace

ServerApp::ServerApp(net::ServerTransport& transport, common::Logger& log,
                     Matchmaker& matchmaker, SessionManager& sessions, UserStore& users,
                     RoomRegistry& rooms)
    : transport_(transport),
      log_(log),
      matchmaker_(matchmaker),
      sessions_(sessions),
      users_(users),
      rooms_(rooms) {
    transport_.onOpen([this](net::ConnectionId id) { onOpen(id); });
    transport_.onMessage([this](net::ConnectionId id, const std::string& text) {
        onMessage(id, text);
    });
    transport_.onClose([this](net::ConnectionId id) { onClose(id); });
}

void ServerApp::onOpen(net::ConnectionId id) {
    log_.info("connection " + std::to_string(id) + " opened");
}

void ServerApp::onMessage(net::ConnectionId id, const std::string& text) {
    std::optional<protocol::Message> message = protocol::decode(text);
    if (!message) {
        log_.info("connection " + std::to_string(id) + " sent malformed input");
        return;
    }
    if (const auto* registerRequest = std::get_if<protocol::RegisterRequest>(&*message)) {
        handleRegister(id, *registerRequest);
        return;
    }
    if (const auto* login = std::get_if<protocol::LoginRequest>(&*message)) {
        handleLogin(id, *login);
        return;
    }
    if (std::get_if<protocol::PlayRequest>(&*message)) {
        handlePlay(id);
        return;
    }
    if (const auto* move = std::get_if<protocol::MoveIntent>(&*message)) {
        handleMove(id, *move);
        return;
    }
    if (const auto* jump = std::get_if<protocol::JumpIntent>(&*message)) {
        handleJump(id, *jump);
        return;
    }
    if (const auto* enterRoom = std::get_if<protocol::EnterRoomRequest>(&*message)) {
        handleEnterRoom(id, *enterRoom);
        return;
    }
    log_.info("connection " + std::to_string(id) + " sent an unexpected message");
}

void ServerApp::handleRegister(net::ConnectionId id, const protocol::RegisterRequest& request) {
    if (users_.registerUser(request.username, request.password)) {
        log_.info(request.username + " registered (connection " + std::to_string(id) + ")");
        transport_.send(id, protocol::encode(protocol::Registered{}));
        return;
    }
    log_.info(request.username + " registration rejected: username taken (connection " +
              std::to_string(id) + ")");
    transport_.send(id, protocol::encode(protocol::AuthRejected{kUsernameTakenReason}));
}

void ServerApp::handleLogin(net::ConnectionId id, const protocol::LoginRequest& request) {
    if (logins_.count(id)) {
        log_.info("connection " + std::to_string(id) + " already logged in");
        return;
    }
    std::optional<UserRecord> user = users_.authenticate(request.username, request.password);
    if (!user) {
        log_.info(request.username + " login rejected: invalid credentials (connection " +
                  std::to_string(id) + ")");
        transport_.send(id, protocol::encode(protocol::AuthRejected{kInvalidCredentialsReason}));
        return;
    }
    logins_.emplace(id, Session{id, user->username});
    log_.info(user->username + " logged in (connection " + std::to_string(id) + ")");
    transport_.send(id, protocol::encode(protocol::LoggedIn{user->rating}));

    reconnectIfSeated(id, user->username);
}

void ServerApp::reconnectIfSeated(net::ConnectionId id, const std::string& username) {
    std::optional<SessionManager::Seat> seat = sessions_.seatFor(username);
    if (!seat) return;
    seat->session->reconnect(id, seat->color);
    std::string opponent = seat->session->usernameOf(model::opposite(seat->color)).value_or("");
    log_.info(username + " reconnected to a game in progress (connection " + std::to_string(id) + ")");
    transport_.send(id, protocol::encode(protocol::Matched{seat->color, opponent}));
}

void ServerApp::handlePlay(net::ConnectionId id) {
    auto it = logins_.find(id);
    if (it == logins_.end()) {
        log_.info("connection " + std::to_string(id) + " requested PLAY without logging in");
        return;
    }
    std::optional<int> rating = users_.ratingOf(it->second.username);
    if (!rating) return;
    matchmaker_.enqueue(id, it->second.username, *rating);
    log_.info(it->second.username + " queued for a match (rating " + std::to_string(*rating) + ")");
}

void ServerApp::handleMove(net::ConnectionId id, const protocol::MoveIntent& intent) {
    GameSession* session = sessions_.sessionFor(id);
    if (!session) return;
    std::optional<model::Color> color = session->colorOf(id);
    if (!color) return;
    session->submit(MoveCommand{*color, intent.from, intent.to});
}

void ServerApp::handleJump(net::ConnectionId id, const protocol::JumpIntent& intent) {
    GameSession* session = sessions_.sessionFor(id);
    if (!session) return;
    std::optional<model::Color> color = session->colorOf(id);
    if (!color) return;
    session->submit(JumpCommand{*color, intent.cell});
}

namespace {
std::string roleName(protocol::Role role) {
    switch (role) {
        case protocol::Role::kWhite: return "White";
        case protocol::Role::kBlack: return "Black";
        case protocol::Role::kSpectator: return "Spectator";
    }
    return "Spectator";
}
}  // namespace

void ServerApp::handleEnterRoom(net::ConnectionId id, const protocol::EnterRoomRequest& request) {
    auto it = logins_.find(id);
    if (it == logins_.end()) {
        log_.info("connection " + std::to_string(id) + " requested to enter a room without logging in");
        return;
    }
    std::optional<int> rating = users_.ratingOf(it->second.username);
    if (!rating) return;

    RoomRegistry::Entry entry = rooms_.enter(request.roomName, id, it->second.username, *rating);
    if (entry.role == protocol::Role::kSpectator) {
        if (GameSession* session = sessions_.find(entry.match)) session->addSpectator(id);
    }
    log_.info(it->second.username + " entered room '" + request.roomName + "' as " +
              roleName(entry.role));
    transport_.send(id, protocol::encode(protocol::RoomJoined{entry.role, entry.color}));
}

void ServerApp::onClose(net::ConnectionId id) {
    if (matchmaker_.isWaiting(id)) {
        matchmaker_.dequeue(id);
        log_.info("connection " + std::to_string(id) + " left the matchmaking queue");
        logins_.erase(id);
        return;
    }
    if (GameSession* session = sessions_.sessionFor(id)) {
        session->onDisconnect(id);
        log_.info("connection " + std::to_string(id) +
                  " disconnected mid-game; forfeit countdown started");
        logins_.erase(id);
        return;
    }
    log_.info("connection " + std::to_string(id) + " closed");
    logins_.erase(id);
}

}
