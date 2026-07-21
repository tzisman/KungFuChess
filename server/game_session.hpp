#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "realtime/motion.hpp"
#include "server/command_queue.hpp"

namespace kfc::server {

// One connected player's seat in this match: which colour they claimed, which
// socket they are on, and the account identity behind it. Owned per-session
// (a vector here, not a shared global map) so concurrent matches never
// collide with each other's seating.
struct PlayerSlot {
    model::Color color;
    net::ConnectionId connection;
    std::string username;
    int rating;
};

class GameSession {
public:
    GameSession(net::ServerTransport& transport, model::Board board,
                realtime::MotionProfiles profiles = {});

    // Seats a connection at the given colour. Fails (returns false) if that
    // colour is already claimed by someone else.
    bool claimColor(model::Color color, net::ConnectionId id,
                    std::string username, int rating);

    // Queues a command for the next tick, tagged with the sender's colour.
    void submit(PlayerCommand command);

    // Apply every command queued since the last tick, advance the clock by
    // elapsedMs, and send the resulting state to every seated participant.
    void tick(int elapsedMs);

    // Run the tick loop off the wall clock until stop() is called (blocks).
    void run();
    void stop();

    bool ownsConnection(net::ConnectionId id) const;
    std::optional<model::Color> colorOf(net::ConnectionId id) const;

private:
    void applyCommands();
    void apply(const MoveCommand& command);
    void apply(const JumpCommand& command);
    bool ownsPieceAt(model::Color color, model::Position cell) const;
    void broadcastState();

    net::ServerTransport& transport_;
    CommandQueue commands_;
    std::vector<PlayerSlot> players_;
    product::ScoreBoard scores_;
    product::MoveLog moveLog_;
    engine::GameEngine engine_;
    std::atomic<bool> running_{false};
};

}
