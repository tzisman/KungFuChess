#pragma once

#include <optional>
#include <string>
#include <vector>

#include "net/transport.hpp"

namespace kfc::server {

inline constexpr int kMatchRatingBand = 100;
inline constexpr int kMatchTimeoutMs = 30'000;

// Someone waiting for a PLAY match, tagged with the identity and rating the
// pairing decision (and, once paired, the seat in the match) needs.
struct WaitingPlayer {
    net::ConnectionId connection;
    std::string username;
    int rating;
};

struct Pairing {
    WaitingPlayer a;
    WaitingPlayer b;
};

// A queue of players waiting for an opponent within kMatchRatingBand rating
// points of themselves, or a timeout if none is found within kMatchTimeoutMs.
// Pure: it knows nothing of sockets, sessions, or the wire, so it is driven
// entirely through enqueue/dequeue/tryPair/expire by whatever polls it.
class Matchmaker {
public:
    void enqueue(net::ConnectionId connection, std::string username, int rating);
    void dequeue(net::ConnectionId connection);
    bool isWaiting(net::ConnectionId connection) const;

    // Removes and returns the first pair found within the rating band, or
    // nullopt if no such pair exists yet.
    std::optional<Pairing> tryPair();

    // Advances every waiting player's clock by elapsedMs; removes and returns
    // whoever has now waited kMatchTimeoutMs or longer.
    std::vector<WaitingPlayer> expire(int elapsedMs);

private:
    struct Entry {
        WaitingPlayer player;
        int waitedMs = 0;
    };

    std::vector<Entry> waiting_;
};

}
