#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "common/logger.hpp"
#include "engine/game_engine.hpp"
#include "engine/game_events.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "realtime/motion.hpp"
#include "server/command_queue.hpp"
#include "server/user_store.hpp"

namespace kfc::server {

inline constexpr int kDisconnectCountdownMs = 20'000;
inline constexpr int kCountdownBroadcastIntervalMs = 1'000;

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
                realtime::MotionProfiles profiles, UserStore& users, common::Logger& log);

    // Seats a connection at the given colour. Fails (returns false) if that
    // colour is already claimed by someone else.
    bool claimColor(model::Color color, net::ConnectionId id,
                    std::string username, int rating);

    // Queues a command for the next tick, tagged with the sender's colour.
    void submit(PlayerCommand command);

    // Apply every command queued since the last tick, advance the clock by
    // elapsedMs, and send the resulting state to every seated participant.
    void tick(int elapsedMs);

    // Run the tick loop off the wall clock until stop() is called, or the
    // game ends on its own (forfeit or king capture) (blocks).
    void run();
    void stop();

    bool ownsConnection(net::ConnectionId id) const;
    std::optional<model::Color> colorOf(net::ConnectionId id) const;

    // Starts a 20-second countdown toward a forfeit for the connection's
    // colour. A no-op if the connection isn't seated.
    void onDisconnect(net::ConnectionId id);

    // The same username reconnects before the countdown reaches zero: rebinds
    // its colour to the new connection and cancels the countdown.
    void reconnect(net::ConnectionId newId, model::Color color);

    // True once the game has ended, by either a king capture or a forfeit.
    bool isFinished() const;

private:
    void applyCommands();
    void apply(const MoveCommand& command);
    void apply(const JumpCommand& command);
    bool ownsPieceAt(model::Color color, model::Position cell) const;
    void broadcastState();
    void tickDisconnectCountdown(int elapsedMs);
    void broadcastCountdown() const;
    void onGameOver(const engine::GameOverEvent& event);

    net::ServerTransport& transport_;
    UserStore& users_;
    common::Logger& log_;
    CommandQueue commands_;
    std::vector<PlayerSlot> players_;
    product::ScoreBoard scores_;
    product::MoveLog moveLog_;
    engine::GameEngine engine_;
    std::optional<model::Color> disconnectedColor_;
    int countdownRemainingMs_ = 0;
    int msSinceLastCountdownBroadcast_ = 0;
    bool finished_ = false;
    // Starts true so a stop() that lands before the owning thread gets around
    // to calling run() (SessionManager spawns the thread and hands control
    // back immediately) is not clobbered by run() re-asserting it.
    std::atomic<bool> running_{true};
};

}
