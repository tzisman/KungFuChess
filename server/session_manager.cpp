#include "server/session_manager.hpp"

#include <thread>
#include <utility>

namespace kfc::server {

SessionManager::SessionManager(net::ServerTransport& transport, UserStore& users,
                               common::Logger& log)
    : transport_(transport), users_(users), log_(log) {}

SessionManager::~SessionManager() {
    {
        std::lock_guard<std::mutex> lock{mutex_};
        for (auto& [id, session] : sessions_) {
            session->stop();
        }
    }
    for (std::thread& thread : threads_) {
        if (thread.joinable()) thread.join();
    }
}

MatchId SessionManager::createSession(model::Board board, realtime::MotionProfiles profiles) {
    std::lock_guard<std::mutex> lock{mutex_};
    MatchId id = nextId_++;
    std::shared_ptr<GameSession> session = std::make_shared<GameSession>(
        transport_, std::move(board), std::move(profiles), users_, log_);
    sessions_.emplace(id, session);
    threads_.emplace_back([this, id, session] {
        session->run();
        std::lock_guard<std::mutex> lock{mutex_};
        sessions_.erase(id);
    });
    return id;
}

void SessionManager::destroySession(MatchId id) {
    std::shared_ptr<GameSession> session;
    {
        std::lock_guard<std::mutex> lock{mutex_};
        auto it = sessions_.find(id);
        if (it == sessions_.end()) return;
        session = it->second;
        sessions_.erase(it);
    }
    session->stop();
}

GameSession* SessionManager::find(MatchId id) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = sessions_.find(id);
    return it == sessions_.end() ? nullptr : it->second.get();
}

GameSession* SessionManager::sessionFor(net::ConnectionId connection) {
    std::lock_guard<std::mutex> lock{mutex_};
    for (auto& [id, session] : sessions_) {
        if (session->ownsConnection(connection)) return session.get();
    }
    return nullptr;
}

std::optional<MatchId> SessionManager::matchIdFor(net::ConnectionId connection) const {
    std::lock_guard<std::mutex> lock{mutex_};
    for (const auto& [id, session] : sessions_) {
        if (session->ownsConnection(connection)) return id;
    }
    return std::nullopt;
}

}
