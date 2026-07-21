#include <doctest/doctest.h>

#include <algorithm>
#include <optional>
#include <utility>

#include "fake_transport.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "protocol/wire_snapshot.hpp"
#include "realtime/motion.hpp"
#include "server/game_session.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::net::ConnectionId;
using kfc::product::GameStateView;
using kfc::protocol::decodeSnapshot;
using kfc::realtime::kSquareTravelMs;
using kfc::server::GameSession;
using kfc::server::JumpCommand;
using kfc::server::MoveCommand;
using kfc::test::FakeServerTransport;

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

// The last message sent to a given connection, decoded as a snapshot.
std::optional<GameStateView> lastSnapshotTo(const FakeServerTransport& transport,
                                            ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it)
        if (it->first == id) return decodeSnapshot(it->second);
    return std::nullopt;
}

}  // namespace

TEST_CASE("claiming an unclaimed colour succeeds and seats the connection") {
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};

    CHECK(game.claimColor(Color::kWhite, 1, "Alice", kStartingRating));

    CHECK(game.ownsConnection(1));
    REQUIRE(game.colorOf(1).has_value());
    CHECK(*game.colorOf(1) == Color::kWhite);
}

TEST_CASE("claiming an already-claimed colour fails") {
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);

    CHECK_FALSE(game.claimColor(Color::kWhite, 2, "Eve", kStartingRating));

    CHECK_FALSE(game.ownsConnection(2));
}

TEST_CASE("a tick sends a decodable snapshot to every seated participant") {
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
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
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
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
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
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
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
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
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
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
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};
    game.claimColor(Color::kWhite, 1, "Alice", kStartingRating);
    game.submit(JumpCommand{Color::kWhite, Position{0, 0}});

    game.tick(0);

    std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
    REQUIRE(state.has_value());
    CHECK(state->pieces.size() == 1);
}
