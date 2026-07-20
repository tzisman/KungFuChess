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
#include "server/player_names.hpp"

namespace kfc::server {


class GameSession {
public:
    GameSession(net::ServerTransport& transport, CommandQueue& commands,
                PlayerNames& names, model::Board board,
                realtime::MotionProfiles profiles = {});

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
    PlayerNames& names_;
    product::ScoreBoard scores_;
    product::MoveLog moveLog_;
    engine::GameEngine engine_;
    std::atomic<bool> running_{false};
};

}
