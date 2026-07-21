#pragma once

#include <atomic>

#include "common/logger.hpp"
#include "model/board.hpp"
#include "net/transport.hpp"
#include "realtime/motion.hpp"
#include "server/matchmaker.hpp"
#include "server/session_manager.hpp"

namespace kfc::server {

// Services the Matchmaker on its own timer: notifies whoever timed out,
// pairs whoever it can, and spins up a fresh GameSession per pairing (through
// sessions_, the same registry rooms will share in Phase D). Unrelated to
// running GameSessions themselves — each already runs on its own thread —
// this is only the periodic housekeeping matchmaking needs.
class Scheduler {
public:
    Scheduler(Matchmaker& matchmaker, SessionManager& sessions, net::ServerTransport& transport,
              model::Board boardTemplate, realtime::MotionProfiles profiles, common::Logger& log);

    void tick(int elapsedMs);

    // Runs tick() off its own wall clock until stop() is called (blocks).
    void run();
    void stop();

private:
    void notifyExpired(int elapsedMs);
    void pairWaitingPlayers();
    void seatAndNotify(MatchId matchId, const WaitingPlayer& white, const WaitingPlayer& black);

    Matchmaker& matchmaker_;
    SessionManager& sessions_;
    net::ServerTransport& transport_;
    model::Board boardTemplate_;
    realtime::MotionProfiles profiles_;
    common::Logger& log_;
    // Starts true for the same reason GameSession::running_ does: a stop()
    // that lands before the owning thread gets around to calling run() must
    // not be clobbered by run() re-asserting it.
    std::atomic<bool> running_{true};
};

}
