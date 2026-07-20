#include <doctest/doctest.h>

#include <optional>
#include <utility>

#include "fake_transport.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "protocol/wire_snapshot.hpp"
#include "realtime/motion.hpp"
#include "server/command_queue.hpp"
#include "server/game_session.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::GameStateView;
using kfc::protocol::decodeSnapshot;
using kfc::realtime::kSquareTravelMs;
using kfc::server::CommandQueue;
using kfc::server::GameSession;
using kfc::server::JumpCommand;
using kfc::server::MoveCommand;
using kfc::test::FakeServerTransport;

namespace {

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

}  // namespace

TEST_CASE("a tick broadcasts a decodable snapshot of the current state") {
    FakeServerTransport transport;
    CommandQueue commands;
    GameSession game{transport, commands, boardWithRook()};

    game.tick(0);

    REQUIRE(transport.broadcasts.size() == 1);
    std::optional<GameStateView> state =
        decodeSnapshot(transport.broadcasts.front());
    REQUIRE(state.has_value());
    CHECK(state->boardWidth == 8);
    CHECK(state->boardHeight == 8);
    REQUIRE(state->pieces.size() == 1);
    CHECK(state->pieces.front().cell == Position{4, 4});
    CHECK(state->pieces.front().kind == PieceKind::kRook);
}

TEST_CASE("successive ticks broadcast the advancing state to every client") {
    FakeServerTransport transport;
    CommandQueue commands;
    GameSession game{transport, commands, boardWithRook()};

    game.tick(0);
    game.tick(kSquareTravelMs);

    CHECK(transport.broadcasts.size() == 2);
}

TEST_CASE("a queued move for the piece's own colour is applied") {
    FakeServerTransport transport;
    CommandQueue commands;
    GameSession game{transport, commands, boardWithRook()};
    // A single-square hop, so the route's next cell is the final destination.
    commands.push(MoveCommand{Color::kWhite, Position{4, 4}, Position{4, 5}});

    game.tick(0);

    std::optional<GameStateView> state = decodeSnapshot(transport.broadcasts.back());
    REQUIRE(state.has_value());
    std::optional<kfc::product::PieceSnapshot> moving = pieceAt(*state, Position{4, 4});
    REQUIRE(moving.has_value());
    CHECK(moving->movingTo == Position{4, 5});
}

TEST_CASE("a queued move tagged with the wrong colour for that square is ignored") {
    FakeServerTransport transport;
    CommandQueue commands;
    GameSession game{transport, commands, boardWithRook()};
    // The rook at {4,4} is white; a black-tagged move claiming to own it must
    // not move the piece — this is the ownership check the server enforces.
    commands.push(MoveCommand{Color::kBlack, Position{4, 4}, Position{4, 6}});

    game.tick(0);

    std::optional<GameStateView> state = decodeSnapshot(transport.broadcasts.back());
    REQUIRE(state.has_value());
    std::optional<kfc::product::PieceSnapshot> stillThere = pieceAt(*state, Position{4, 4});
    REQUIRE(stillThere.has_value());
    CHECK_FALSE(stillThere->movingTo.has_value());
}

TEST_CASE("a queued jump for an empty square is ignored") {
    FakeServerTransport transport;
    CommandQueue commands;
    GameSession game{transport, commands, boardWithRook()};
    commands.push(JumpCommand{Color::kWhite, Position{0, 0}});

    game.tick(0);

    std::optional<GameStateView> state = decodeSnapshot(transport.broadcasts.back());
    REQUIRE(state.has_value());
    CHECK(state->pieces.size() == 1);
}
