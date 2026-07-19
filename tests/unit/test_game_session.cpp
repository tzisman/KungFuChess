#include <doctest/doctest.h>

#include <optional>
#include <utility>

#include "fake_transport.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "protocol/wire_snapshot.hpp"
#include "realtime/motion.hpp"
#include "server/game_session.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::GameStateView;
using kfc::protocol::decodeSnapshot;
using kfc::realtime::kSquareTravelMs;
using kfc::server::GameSession;
using kfc::test::FakeServerTransport;

namespace {

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

}  // namespace

TEST_CASE("a tick broadcasts a decodable snapshot of the current state") {
    FakeServerTransport transport;
    GameSession game{transport, boardWithRook()};

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
    GameSession game{transport, boardWithRook()};

    game.tick(0);
    game.tick(kSquareTravelMs);

    CHECK(transport.broadcasts.size() == 2);
}
