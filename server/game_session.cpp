#include "server/game_session.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "product/game_state_view.hpp"
#include "protocol/wire_snapshot.hpp"

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
                         realtime::MotionProfiles profiles)
    : transport_(transport), engine_(std::move(board), std::move(profiles)) {
    scores_.subscribeTo(engine_.events());
    moveLog_.subscribeTo(engine_.events());
}

bool GameSession::claimColor(model::Color color, net::ConnectionId id,
                             std::string username, int rating) {
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
    for (const PlayerSlot& player : players_) {
        if (player.connection == id) return player.color;
    }
    return std::nullopt;
}

void GameSession::tick(int elapsedMs) {
    applyCommands();
    engine_.advance(elapsedMs);
    broadcastState();
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
    running_ = true;
    Clock::time_point last = Clock::now();
    while (running_) {
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
