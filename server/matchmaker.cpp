#include "server/matchmaker.hpp"

#include <algorithm>
#include <cstdlib>

namespace kfc::server {

void Matchmaker::enqueue(net::ConnectionId connection, std::string username, int rating) {
    waiting_.push_back(Entry{WaitingPlayer{connection, std::move(username), rating}, 0});
}

void Matchmaker::dequeue(net::ConnectionId connection) {
    waiting_.erase(std::remove_if(waiting_.begin(), waiting_.end(),
                                  [&](const Entry& entry) {
                                      return entry.player.connection == connection;
                                  }),
                   waiting_.end());
}

bool Matchmaker::isWaiting(net::ConnectionId connection) const {
    return std::any_of(waiting_.begin(), waiting_.end(), [&](const Entry& entry) {
        return entry.player.connection == connection;
    });
}

std::optional<Pairing> Matchmaker::tryPair() {
    for (std::size_t i = 0; i < waiting_.size(); ++i) {
        for (std::size_t j = i + 1; j < waiting_.size(); ++j) {
            int ratingGap = std::abs(waiting_[i].player.rating - waiting_[j].player.rating);
            if (ratingGap > kMatchRatingBand) continue;

            Pairing pairing{waiting_[i].player, waiting_[j].player};
            waiting_.erase(waiting_.begin() + static_cast<long>(j));
            waiting_.erase(waiting_.begin() + static_cast<long>(i));
            return pairing;
        }
    }
    return std::nullopt;
}

std::vector<WaitingPlayer> Matchmaker::expire(int elapsedMs) {
    for (Entry& entry : waiting_) entry.waitedMs += elapsedMs;

    std::vector<WaitingPlayer> expired;
    for (const Entry& entry : waiting_) {
        if (entry.waitedMs >= kMatchTimeoutMs) expired.push_back(entry.player);
    }
    waiting_.erase(std::remove_if(waiting_.begin(), waiting_.end(),
                                  [](const Entry& entry) {
                                      return entry.waitedMs >= kMatchTimeoutMs;
                                  }),
                   waiting_.end());
    return expired;
}

}
