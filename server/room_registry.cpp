#include "server/room_registry.hpp"

namespace kfc::server {

RoomRegistry::RoomRegistry(SessionManager& sessions, model::Board boardTemplate,
                            realtime::MotionProfiles profiles)
    : sessions_(sessions), boardTemplate_(std::move(boardTemplate)), profiles_(profiles) {}

RoomRegistry::Entry RoomRegistry::enter(const std::string& roomName,
                                          net::ConnectionId connection,
                                          std::string username, int rating) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto existing = rooms_.find(roomName);
    MatchId match = existing != rooms_.end()
                        ? existing->second
                        : sessions_.createSession(boardTemplate_, profiles_);
    if (existing == rooms_.end()) rooms_[roomName] = match;

    GameSession* session = sessions_.find(match);
    if (session->claimColor(model::Color::kWhite, connection, username, rating))
        return Entry{match, protocol::Role::kWhite, model::Color::kWhite};
    if (session->claimColor(model::Color::kBlack, connection, username, rating))
        return Entry{match, protocol::Role::kBlack, model::Color::kBlack};
    return Entry{match, protocol::Role::kSpectator, std::nullopt};
}

}
