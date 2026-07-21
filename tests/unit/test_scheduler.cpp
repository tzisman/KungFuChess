#include <doctest/doctest.h>

#include <optional>
#include <sstream>
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
#include "server/matchmaker.hpp"
#include "server/scheduler.hpp"
#include "server/session_manager.hpp"

using kfc::common::Logger;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::net::ConnectionId;
using kfc::protocol::decode;
using kfc::protocol::Matched;
using kfc::protocol::Message;
using kfc::protocol::NoOpponent;
using kfc::server::kMatchTimeoutMs;
using kfc::server::Matchmaker;
using kfc::server::Scheduler;
using kfc::server::SessionManager;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;

namespace {

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

// The last well-formed control message sent to a connection, skipping any
// game-state snapshot payloads a freshly spawned GameSession thread may have
// interleaved (those decode to nothing here, since they use a different
// wire shape). Reads through the transport's thread-safe snapshot, since a
// real SessionManager thread may be writing concurrently.
std::optional<Message> lastMessageTo(const FakeServerTransport& transport, ConnectionId id) {
    auto sent = transport.sentSnapshot();
    for (auto it = sent.rbegin(); it != sent.rend(); ++it) {
        if (it->first != id) continue;
        if (std::optional<Message> message = decode(it->second)) return message;
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("two waiting players within the rating band are matched into a fresh session") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    Scheduler scheduler{matchmaker, sessions, transport, boardWithRook(), {}, log};
    matchmaker.enqueue(1, "Alice", 1000);
    matchmaker.enqueue(2, "Bob", 1050);

    scheduler.tick(0);

    std::optional<Message> toAlice = lastMessageTo(transport, 1);
    std::optional<Message> toBob = lastMessageTo(transport, 2);
    REQUIRE(toAlice.has_value());
    REQUIRE(toBob.has_value());
    REQUIRE(std::holds_alternative<Matched>(*toAlice));
    REQUIRE(std::holds_alternative<Matched>(*toBob));
    CHECK(std::get<Matched>(*toAlice).color == Color::kWhite);
    CHECK(std::get<Matched>(*toAlice).opponentName == "Bob");
    CHECK(std::get<Matched>(*toBob).color == Color::kBlack);
    CHECK(std::get<Matched>(*toBob).opponentName == "Alice");
}

TEST_CASE("a lone waiting player past the timeout receives NoOpponent and is removed") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    Scheduler scheduler{matchmaker, sessions, transport, boardWithRook(), {}, log};
    matchmaker.enqueue(1, "Alice", 1000);

    scheduler.tick(kMatchTimeoutMs);

    std::optional<Message> toAlice = lastMessageTo(transport, 1);
    REQUIRE(toAlice.has_value());
    CHECK(std::holds_alternative<NoOpponent>(*toAlice));
    CHECK_FALSE(matchmaker.isWaiting(1));
}
