#pragma once

#include <atomic>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "net/transport.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "realtime/motion.hpp"

namespace kfc::server {

// Owns the one authoritative game: the engine, the observers that derive score
// and move log, and the loop that advances the clock and broadcasts the state to
// every connected client. It is the only thing that touches the engine, so all
// engine access stays on the loop's thread.
class GameSession {
public:
    GameSession(net::ServerTransport& transport, model::Board board,
                realtime::MotionProfiles profiles = {});

    // Advance the clock by elapsedMs and broadcast the resulting state once.
    void tick(int elapsedMs);

    // Run the tick loop off the wall clock until stop() is called (blocks).
    void run();
    void stop();

private:
    void broadcastState();

    net::ServerTransport& transport_;
    product::ScoreBoard scores_;
    product::MoveLog moveLog_;
    engine::GameEngine engine_;
    std::atomic<bool> running_{false};
};

}
