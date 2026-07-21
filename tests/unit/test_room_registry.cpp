#include <doctest/doctest.h>

#include <sstream>

#include "common/logger.hpp"
#include "fake_transport.hpp"
#include "fake_user_store.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "protocol/messages.hpp"
#include "server/room_registry.hpp"
#include "server/session_manager.hpp"

using kfc::common::Logger;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::protocol::Role;
using kfc::server::RoomRegistry;
using kfc::server::SessionManager;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;

namespace {

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

}  // namespace

TEST_CASE("the first entrant to a room takes White, the second takes Black, "
         "and everyone after that spectates") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager sessions{transport, users, log};
    RoomRegistry rooms{sessions, boardWithRook()};

    RoomRegistry::Entry first = rooms.enter("lounge", 1, "Alice", 1000);
    RoomRegistry::Entry second = rooms.enter("lounge", 2, "Bob", 1000);
    RoomRegistry::Entry third = rooms.enter("lounge", 3, "Carol", 1000);

    CHECK(first.role == Role::kWhite);
    REQUIRE(first.color.has_value());
    CHECK(*first.color == Color::kWhite);

    CHECK(second.role == Role::kBlack);
    REQUIRE(second.color.has_value());
    CHECK(*second.color == Color::kBlack);

    CHECK(third.role == Role::kSpectator);
    CHECK_FALSE(third.color.has_value());
}

TEST_CASE("entering the same room name twice reuses the same match") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager sessions{transport, users, log};
    RoomRegistry rooms{sessions, boardWithRook()};

    RoomRegistry::Entry first = rooms.enter("lounge", 1, "Alice", 1000);
    RoomRegistry::Entry second = rooms.enter("lounge", 2, "Bob", 1000);

    CHECK(first.match == second.match);
}

TEST_CASE("different room names get different matches") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager sessions{transport, users, log};
    RoomRegistry rooms{sessions, boardWithRook()};

    RoomRegistry::Entry inLounge = rooms.enter("lounge", 1, "Alice", 1000);
    RoomRegistry::Entry inStudy = rooms.enter("study", 2, "Bob", 1000);

    CHECK(inLounge.match != inStudy.match);
}
