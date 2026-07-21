#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <sstream>
#include <thread>

#include "common/logger.hpp"
#include "fake_transport.hpp"
#include "fake_user_store.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "server/game_session.hpp"
#include "server/session_manager.hpp"

using kfc::common::Logger;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::server::GameSession;
using kfc::server::MatchId;
using kfc::server::SessionManager;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;

namespace {

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

// Polls a short while for a condition that only becomes true once a session's
// own thread has had a chance to run. Real thread scheduling, not a fixed
// sleep, is what a test of createSession is inherently at the mercy of.
template <class Predicate>
bool waitUntil(Predicate predicate) {
    for (int i = 0; i < 200; ++i) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

}  // namespace

TEST_CASE("session ids are assigned in increasing order and can be found") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager manager{transport, users, log};

    MatchId first = manager.createSession(boardWithRook());
    MatchId second = manager.createSession(boardWithRook());

    CHECK(second > first);
    CHECK(manager.find(first) != nullptr);
    CHECK(manager.find(second) != nullptr);

    manager.destroySession(first);
    manager.destroySession(second);
}

TEST_CASE("destroySession removes the session and is safe to call twice") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager manager{transport, users, log};
    MatchId id = manager.createSession(boardWithRook());

    manager.destroySession(id);

    CHECK(manager.find(id) == nullptr);
    manager.destroySession(id);  // must not crash
}

TEST_CASE("sessionFor resolves a claimed connection and nullptr for an unknown one") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager manager{transport, users, log};
    MatchId id = manager.createSession(boardWithRook());
    GameSession* session = manager.find(id);
    REQUIRE(session != nullptr);
    session->claimColor(Color::kWhite, 1, "Alice", 1000);

    CHECK(manager.sessionFor(1) == session);
    CHECK(manager.sessionFor(42) == nullptr);
    CHECK(manager.matchIdFor(1) == id);
    CHECK_FALSE(manager.matchIdFor(42).has_value());

    manager.destroySession(id);
}

TEST_CASE("a created session actually runs on its own thread, and stopping it "
         "lets that thread clean up the registry by itself") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    SessionManager manager{transport, users, log};
    MatchId id = manager.createSession(boardWithRook());
    GameSession* session = manager.find(id);
    REQUIRE(session != nullptr);
    session->claimColor(Color::kWhite, 1, "Alice", 1000);

    REQUIRE(waitUntil([&] {
        auto sent = transport.sentSnapshot();
        return std::any_of(sent.begin(), sent.end(),
                           [](const auto& entry) { return entry.first == 1; });
    }));

    session->stop();

    REQUIRE(waitUntil([&] { return manager.find(id) == nullptr; }));
}
