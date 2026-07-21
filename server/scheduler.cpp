#include "server/scheduler.hpp"

#include <chrono>
#include <optional>
#include <thread>
#include <utility>

#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "server/game_session.hpp"

namespace kfc::server {
namespace {
using Clock = std::chrono::steady_clock;
constexpr int kSchedulerTickMs = 200;

int elapsedMsSince(Clock::time_point& last) {
    Clock::time_point now = Clock::now();
    int elapsed = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count());
    last = now;
    return elapsed;
}
}  // namespace

Scheduler::Scheduler(Matchmaker& matchmaker, SessionManager& sessions,
                     net::ServerTransport& transport, model::Board boardTemplate,
                     realtime::MotionProfiles profiles, common::Logger& log)
    : matchmaker_(matchmaker),
      sessions_(sessions),
      transport_(transport),
      boardTemplate_(std::move(boardTemplate)),
      profiles_(std::move(profiles)),
      log_(log) {}

void Scheduler::tick(int elapsedMs) {
    notifyExpired(elapsedMs);
    pairWaitingPlayers();
}

void Scheduler::notifyExpired(int elapsedMs) {
    for (const WaitingPlayer& player : matchmaker_.expire(elapsedMs)) {
        log_.info(player.username + " timed out waiting for a match");
        transport_.send(player.connection,
                        protocol::encode(protocol::Message{protocol::NoOpponent{}}));
    }
}

void Scheduler::pairWaitingPlayers() {
    while (std::optional<Pairing> pairing = matchmaker_.tryPair()) {
        MatchId matchId = sessions_.createSession(boardTemplate_, profiles_);
        seatAndNotify(matchId, pairing->a, pairing->b);
    }
}

void Scheduler::seatAndNotify(MatchId matchId, const WaitingPlayer& white,
                              const WaitingPlayer& black) {
    GameSession* session = sessions_.find(matchId);
    if (!session) return;  // the session already finished before it could be seated

    session->claimColor(model::Color::kWhite, white.connection, white.username, white.rating);
    session->claimColor(model::Color::kBlack, black.connection, black.username, black.rating);

    log_.info(white.username + " (white, rating " + std::to_string(white.rating) +
             ") matched against " + black.username + " (black, rating " +
             std::to_string(black.rating) + ")");
    transport_.send(white.connection, protocol::encode(protocol::Message{
                                          protocol::Matched{model::Color::kWhite, black.username}}));
    transport_.send(black.connection, protocol::encode(protocol::Message{
                                          protocol::Matched{model::Color::kBlack, white.username}}));
}

void Scheduler::run() {
    Clock::time_point last = Clock::now();
    while (running_) {
        tick(elapsedMsSince(last));
        std::this_thread::sleep_for(std::chrono::milliseconds(kSchedulerTickMs));
    }
}

void Scheduler::stop() { running_ = false; }

}
