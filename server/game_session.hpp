#pragma once

#include <atomic>

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

// Owns the one authoritative game: the engine, the observers that derive score
// and move log, and the loop that applies queued player commands, advances the
// clock, and broadcasts the state to every connected client. It is the only
// thing that touches the engine, so all engine access stays on the loop's
// thread; commands_ is how the network thread's intents reach it safely.
class GameSession {
public:
    GameSession(net::ServerTransport& transport, CommandQueue& commands,
                model::Board board, realtime::MotionProfiles profiles = {});

    // Apply every command queued since the last tick, advance the clock by
    // elapsedMs, and broadcast the resulting state once.
    void tick(int elapsedMs);

    // Run the tick loop off the wall clock until stop() is called (blocks).
    void run();
    void stop();

private:
    void applyCommands();
    void apply(const MoveCommand& command);
    void apply(const JumpCommand& command);
    bool ownsPieceAt(model::Color color, model::Position cell) const;
    void broadcastState();

    net::ServerTransport& transport_;
    CommandQueue& commands_;
    product::ScoreBoard scores_;
    product::MoveLog moveLog_;
    engine::GameEngine engine_;
    std::atomic<bool> running_{false};
};

}
