#include <doctest/doctest.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <utility>
#include <variant>

#include "common/logger.hpp"
#include "fake_transport.hpp"
#include "fake_user_store.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/wire_snapshot.hpp"
#include "realtime/motion.hpp"
#include "server/game_session.hpp"

using kfc::common::Logger;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::net::ConnectionId;
using kfc::product::GameStateView;
using kfc::protocol::CountdownTick;
using kfc::protocol::decode;
using kfc::protocol::decodeSnapshot;
using kfc::protocol::Message;
using kfc::realtime::kSquareTravelMs;
using kfc::server::GameSession;
using kfc::server::JumpCommand;
using kfc::server::kCountdownBroadcastIntervalMs;
using kfc::server::kDisconnectCountdownMs;
using kfc::server::MoveCommand;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;

namespace {

constexpr int kStartingRating = 1000;

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

// A snapshot's own piece, found by its current cell (or absent if it moved
// away or was never there).
std::optional<kfc::product::PieceSnapshot> pieceAt(const GameStateView& state,
                                                    Position cell) {
    for (const kfc::product::PieceSnapshot& piece : state.pieces)
        if (piece.cell == cell) return piece;
    return std::nullopt;
}

// The last snapshot sent to a given connection.
std::optional<GameStateView> lastSnapshotTo(const FakeServerTransport& transport,
                                            ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it)
        if (it->first == id) return decodeSnapshot(it->second);
    return std::nullopt;
}

// The last CountdownTick sent to a given connection (snapshots and countdown
// ticks travel as different payload shapes over the same send() channel, so
// this skips anything that doesn't decode as a CountdownTick).
std::optional<CountdownTick> lastCountdownTo(const FakeServerTransport& transport,
                                              ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it) {
        if (it->first != id) continue;
        std::optional<Message> message = decode(it->second);
        if (message && std::holds_alternative<CountdownTick>(*message))
            return std::get<CountdownTick>(*message);
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("claiming an unclaimed colour succeeds and seats the connection") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};

    CHECK(game.claimColor(Color::kWhite, 1, "Alice", kStartingRating));

    CHECK(game.ownsConnection(1));
    REQUIRE(game.colorOf(1).has_value());
    CHECK(*game.colorOf(1) == Color::kWhite);
}

TEST_CASE("claiming an already-claimed colour fails") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);

    CHECK_FALSE(game.claimColor(Color::kWhite, 2, "Eve", kStartingRating));

    CHECK_FALSE(game.ownsConnection(2));
}

TEST_CASE("a tick sends a decodable snapshot to every seated participant") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);

    game.tick(0);

    std::optional<GameStateView> forWhite = lastSnapshotTo(transport, 1);
    std::optional<GameStateView> forBlack = lastSnapshotTo(transport, 2);
    REQUIRE(forWhite.has_value());
    REQUIRE(forBlack.has_value());
    CHECK(forWhite->boardWidth == 8);
    CHECK(forWhite->boardHeight == 8);
    REQUIRE(forWhite->pieces.size() == 1);
    CHECK(forWhite->pieces.front().cell == Position{4, 4});
    CHECK(forWhite->pieces.front().kind == PieceKind::kRook);
}

TEST_CASE("successive ticks each send a fresh snapshot to every client") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);

    game.tick(0);
    game.tick(kSquareTravelMs);

    CHECK(std::count_if(transport.sent.begin(), transport.sent.end(),
                        [](const auto& entry) { return entry.first == 1; }) == 2);
    CHECK(std::count_if(transport.sent.begin(), transport.sent.end(),
                        [](const auto& entry) { return entry.first == 2; }) == 2);
}

TEST_CASE("a claimed player's username is broadcast instead of the generic colour name") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);

    game.tick(0);

    std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
    REQUIRE(state.has_value());
    auto white = std::find_if(state->players.begin(), state->players.end(),
                              [](const auto& player) { return player.color == Color::kWhite; });
    REQUIRE(white != state->players.end());
    CHECK(white->name == "Alice");
}

TEST_CASE("a submitted move for the piece's own colour is applied") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    // A single-square hop, so the route's next cell is the final destination.
    game.submit(MoveCommand{Color::kWhite, Position{4, 4}, Position{4, 5}});

    game.tick(0);

    std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
    REQUIRE(state.has_value());
    std::optional<kfc::product::PieceSnapshot> moving = pieceAt(*state, Position{4, 4});
    REQUIRE(moving.has_value());
    CHECK(moving->movingTo == Position{4, 5});
}

TEST_CASE("a submitted move tagged with the wrong colour for that square is ignored") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    // The rook at {4,4} is white; a black-tagged move claiming to own it must
    // not move the piece — this is the ownership check the server enforces.
    game.submit(MoveCommand{Color::kBlack, Position{4, 4}, Position{4, 6}});

    game.tick(0);

    std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
    REQUIRE(state.has_value());
    std::optional<kfc::product::PieceSnapshot> stillThere = pieceAt(*state, Position{4, 4});
    REQUIRE(stillThere.has_value());
    CHECK_FALSE(stillThere->movingTo.has_value());
}

TEST_CASE("a submitted jump for an empty square is ignored") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.submit(JumpCommand{Color::kWhite, Position{0, 0}});

    game.tick(0);

    std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
    REQUIRE(state.has_value());
    CHECK(state->pieces.size() == 1);
}

TEST_CASE("a disconnect starts a forfeit countdown and broadcasts a CountdownTick once a second") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);

    game.onDisconnect(1);  // Alice (white) drops
    game.tick(kCountdownBroadcastIntervalMs);

    std::optional<CountdownTick> firstTick = lastCountdownTo(transport, 2);
    REQUIRE(firstTick.has_value());
    CHECK(firstTick->secondsLeft == 19);

    game.tick(kCountdownBroadcastIntervalMs);

    std::optional<CountdownTick> secondTick = lastCountdownTo(transport, 2);
    REQUIRE(secondTick.has_value());
    CHECK(secondTick->secondsLeft == 18);
}

TEST_CASE("the countdown reaching zero forfeits to the connected opponent and finishes the game") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);
    game.onDisconnect(1);  // Alice (white) drops; Bob should win

    for (int elapsed = 0; elapsed < kDisconnectCountdownMs; elapsed += kCountdownBroadcastIntervalMs) {
        game.tick(kCountdownBroadcastIntervalMs);
    }

    CHECK(game.isFinished());
    std::optional<GameStateView> state = lastSnapshotTo(transport, 2);
    REQUIRE(state.has_value());
    CHECK(state->gameOver);
}

TEST_CASE("reconnecting before the countdown ends cancels it and the game continues") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);
    game.onDisconnect(1);
    game.tick(kCountdownBroadcastIntervalMs);  // one second into the countdown

    game.reconnect(3, Color::kWhite);  // Alice reconnects on a fresh connection

    for (int i = 0; i < 25; ++i) game.tick(kCountdownBroadcastIntervalMs);

    CHECK_FALSE(game.isFinished());
    CHECK(game.ownsConnection(3));
    CHECK_FALSE(game.ownsConnection(1));
}

TEST_CASE("a game that ends by forfeit updates both players' ratings through the UserStore") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    users.registerUser("Alice", "pw");
    users.registerUser("Bob", "pw");
    GameSession game{transport, boardWithRook(), {}, users, log};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.claimColor(Color::kBlack, 2, "Bob", kStartingRating);
    game.onDisconnect(1);  // Alice (white) drops; Bob wins

    for (int elapsed = 0; elapsed < kDisconnectCountdownMs; elapsed += kCountdownBroadcastIntervalMs) {
        game.tick(kCountdownBroadcastIntervalMs);
    }

    REQUIRE(game.isFinished());
    std::optional<int> aliceRating = users.ratingOf("Alice");
    std::optional<int> bobRating = users.ratingOf("Bob");
    REQUIRE(aliceRating.has_value());
    REQUIRE(bobRating.has_value());
    CHECK(*bobRating > kStartingRating);
    CHECK(*aliceRating < kStartingRating);
}
