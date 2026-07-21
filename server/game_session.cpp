#include "server/game_session.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "product/game_state_view.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/wire_snapshot.hpp"
#include "server/elo.hpp"

namespace kfc::server {
namespace {
using Clock = std::chrono::steady_clock;
constexpr int kFrameMs = 30;

int elapsedMsSince(Clock::time_point& last) {
    Clock::time_point now = Clock::now();
    int elapsed = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count());
    last = now;
    return elapsed;
}
}  // namespace

GameSession::GameSession(net::ServerTransport& transport, model::Board board,
                         realtime::MotionProfiles profiles, UserStore& users,
                         common::Logger& log)
    : transport_(transport),
      users_(users),
      log_(log),
      engine_(std::move(board), std::move(profiles)) {
    scores_.subscribeTo(engine_.events());
    moveLog_.subscribeTo(engine_.events());
    engine_.events().subscribe<engine::GameOverEvent>(
        [this](const engine::GameOverEvent& event) { onGameOver(event); });
}

bool GameSession::claimColor(model::Color color, net::ConnectionId id,
                             std::string username, int rating) {
    std::lock_guard<std::mutex> lock{mutex_};
    for (const PlayerSlot& player : players_) {
        if (player.color == color) return false;
    }
    players_.push_back(PlayerSlot{color, id, std::move(username), rating});
    return true;
}

void GameSession::submit(PlayerCommand command) { commands_.push(std::move(command)); }

bool GameSession::ownsConnection(net::ConnectionId id) const {
    return colorOf(id).has_value();
}

std::optional<model::Color> GameSession::colorOf(net::ConnectionId id) const {
    std::lock_guard<std::mutex> lock{mutex_};
    for (const PlayerSlot& player : players_) {
        if (player.connection == id) return player.color;
    }
    return std::nullopt;
}

void GameSession::onDisconnect(net::ConnectionId id) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = std::find_if(players_.begin(), players_.end(),
                           [&](const PlayerSlot& player) { return player.connection == id; });
    if (it == players_.end()) return;
    disconnectedColor_ = it->color;
    countdownRemainingMs_ = kDisconnectCountdownMs;
    msSinceLastCountdownBroadcast_ = 0;
    log_.info(model::nameOf(it->color) + std::string{" disconnected; forfeit countdown started"});
}

void GameSession::reconnect(net::ConnectionId newId, model::Color color) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = std::find_if(players_.begin(), players_.end(),
                           [&](const PlayerSlot& player) { return player.color == color; });
    if (it == players_.end()) return;
    it->connection = newId;
    if (disconnectedColor_ == color) {
        disconnectedColor_.reset();
        countdownRemainingMs_ = 0;
    }
    log_.info(std::string{model::nameOf(color)} + " reconnected");
}

bool GameSession::isFinished() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return finished_;
}

// Locked for its whole body: this runs on the session's own thread once
// SessionManager spawns it, while claimColor/onDisconnect/reconnect/colorOf/
// isFinished above may be called concurrently from ServerApp's or
// Scheduler's thread. Everything this touches — directly or through the
// tickDisconnectCountdown/broadcastState/onGameOver helpers below — is
// private state guarded by the same mutex_, so none of those helpers lock it
// again themselves.
void GameSession::tick(int elapsedMs) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (finished_) return;
    applyCommands();
    engine_.advance(elapsedMs);
    tickDisconnectCountdown(elapsedMs);
    broadcastState();
}

void GameSession::tickDisconnectCountdown(int elapsedMs) {
    if (!disconnectedColor_) return;
    countdownRemainingMs_ -= elapsedMs;
    if (countdownRemainingMs_ <= 0) {
        engine_.declareForfeit(model::opposite(*disconnectedColor_));
        return;
    }
    msSinceLastCountdownBroadcast_ += elapsedMs;
    if (msSinceLastCountdownBroadcast_ < kCountdownBroadcastIntervalMs) return;
    msSinceLastCountdownBroadcast_ = 0;
    broadcastCountdown();
}

void GameSession::broadcastCountdown() const {
    int secondsLeft = countdownRemainingMs_ / 1000;
    std::string payload = protocol::encode(protocol::Message{protocol::CountdownTick{secondsLeft}});
    for (const PlayerSlot& player : players_) {
        transport_.send(player.connection, payload);
    }
}

void GameSession::onGameOver(const engine::GameOverEvent& event) {
    finished_ = true;
    model::Color loserColor = model::opposite(event.winner);
    auto winner = std::find_if(players_.begin(), players_.end(),
                               [&](const PlayerSlot& p) { return p.color == event.winner; });
    auto loser = std::find_if(players_.begin(), players_.end(),
                              [&](const PlayerSlot& p) { return p.color == loserColor; });
    if (winner == players_.end() || loser == players_.end()) return;

    EloUpdate update = computeElo(winner->rating, loser->rating);
    winner->rating = update.winnerRating;
    loser->rating = update.loserRating;
    users_.updateRating(winner->username, update.winnerRating);
    users_.updateRating(loser->username, update.loserRating);
    log_.info(winner->username + " defeated " + loser->username + "; ratings now " +
              std::to_string(update.winnerRating) + "/" + std::to_string(update.loserRating));
}

void GameSession::applyCommands() {
    for (const PlayerCommand& command : commands_.drain()) {
        std::visit([this](const auto& cmd) { apply(cmd); }, command);
    }
}

void GameSession::apply(const MoveCommand& command) {
    if (!ownsPieceAt(command.color, command.from)) return;
    engine_.requestMove(command.from, command.to);
}

void GameSession::apply(const JumpCommand& command) {
    if (!ownsPieceAt(command.color, command.cell)) return;
    engine_.requestJump(command.cell);
}

bool GameSession::ownsPieceAt(model::Color color, model::Position cell) const {
    std::optional<model::Piece> piece = engine_.board().pieceAt(cell);
    return piece && piece->color() == color;
}

void GameSession::run() {
    Clock::time_point last = Clock::now();
    while (running_ && !isFinished()) {
        tick(elapsedMsSince(last));
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }
}

void GameSession::stop() { running_ = false; }

void GameSession::broadcastState() {
    product::GameStateView state =
        product::gameStateView(engine_, scores_, moveLog_);
    for (product::PlayerSnapshot& player : state.players) {
        auto it = std::find_if(players_.begin(), players_.end(),
                               [&](const PlayerSlot& slot) { return slot.color == player.color; });
        if (it != players_.end()) player.name = it->username;
    }
    std::string payload = protocol::encodeSnapshot(state);
    for (const PlayerSlot& player : players_) {
        transport_.send(player.connection, payload);
    }
}

}
