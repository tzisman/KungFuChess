#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include "common/logger.hpp"
#include "fake_transport.hpp"
#include "fake_user_store.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "product/game_state_view.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/wire_snapshot.hpp"
#include "server/game_session.hpp"
#include "server/matchmaker.hpp"
#include "server/server_app.hpp"
#include "server/session_manager.hpp"
#include "server/user_store.hpp"

using kfc::common::Logger;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::net::ConnectionId;
using kfc::product::GameStateView;
using kfc::protocol::AuthRejected;
using kfc::protocol::decode;
using kfc::protocol::decodeSnapshot;
using kfc::protocol::encode;
using kfc::protocol::JumpIntent;
using kfc::protocol::LoggedIn;
using kfc::protocol::LoginRequest;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
using kfc::protocol::PlayRequest;
using kfc::protocol::Registered;
using kfc::protocol::RegisterRequest;
using kfc::server::GameSession;
using kfc::server::kDefaultRating;
using kfc::server::Matchmaker;
using kfc::server::MatchId;
using kfc::server::Pairing;
using kfc::server::ServerApp;
using kfc::server::SessionManager;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;

namespace {

constexpr char kPassword[] = "hunter2";

std::string registerMessage(const std::string& username) {
    return encode(Message{RegisterRequest{username, kPassword}});
}

std::string loginMessage(const std::string& username) {
    return encode(Message{LoginRequest{username, kPassword}});
}

// Registers and logs in as a real client would: two messages over the same
// connection, in order.
void joinAs(FakeServerTransport& transport, ConnectionId id, const std::string& username) {
    transport.connect(id);
    transport.receive(id, registerMessage(username));
    transport.receive(id, loginMessage(username));
}

// The last well-formed control message sent to a connection.
std::optional<Message> lastTo(const FakeServerTransport& transport, ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it)
        if (it->first == id) return decode(it->second);
    return std::nullopt;
}

// The last game-state snapshot sent to a connection, read through the
// transport's thread-safe snapshot — a session created via SessionManager
// sends from its own background thread, so a raw read here would race it.
std::optional<GameStateView> lastSnapshotTo(const FakeServerTransport& transport,
                                            ConnectionId id) {
    auto sent = transport.sentSnapshot();
    for (auto it = sent.rbegin(); it != sent.rend(); ++it)
        if (it->first == id) return decodeSnapshot(it->second);
    return std::nullopt;
}

// The last well-formed control message sent to a connection, reading through
// the transport's thread-safe snapshot. Used once a session has been created
// via SessionManager: its own thread is then ticking (and sending) for real,
// so anything a test reads back must go through the same synchronized copy
// the session's send() calls are guarded by, not the raw vector.
std::optional<Message> lastMessageToSync(const FakeServerTransport& transport, ConnectionId id) {
    auto sent = transport.sentSnapshot();
    for (auto it = sent.rbegin(); it != sent.rend(); ++it) {
        if (it->first != id) continue;
        if (std::optional<Message> message = decode(it->second)) return message;
    }
    return std::nullopt;
}

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

// Polls a short while for a condition that only becomes true once a live
// SessionManager-owned GameSession's own background thread has had a chance
// to tick. Real thread scheduling, not a fixed sleep, is what a test that
// creates a session through SessionManager is inherently at the mercy of.
template <class Predicate>
bool waitUntil(Predicate predicate) {
    for (int i = 0; i < 200; ++i) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

}  // namespace

TEST_CASE("registering an existing username is rejected") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};

    transport.connect(1);
    transport.receive(1, registerMessage("Alice"));
    transport.connect(2);
    transport.receive(2, registerMessage("Alice"));

    std::optional<Message> reply = lastTo(transport, 2);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<AuthRejected>(*reply));
    CHECK(std::get<AuthRejected>(*reply).reason == "username_taken");
}

TEST_CASE("logging in with the wrong password is rejected") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};

    transport.connect(1);
    transport.receive(1, registerMessage("Alice"));
    transport.receive(1, encode(Message{LoginRequest{"Alice", "wrong-password"}}));

    std::optional<Message> reply = lastTo(transport, 1);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<AuthRejected>(*reply));
    CHECK(std::get<AuthRejected>(*reply).reason == "invalid_credentials");
}

TEST_CASE("logging in returns the account's current rating") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};

    joinAs(transport, 1, "Alice");

    std::optional<Message> reply = lastTo(transport, 1);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<LoggedIn>(*reply));
    CHECK(std::get<LoggedIn>(*reply).rating == kDefaultRating);
}

TEST_CASE("PLAY queues the connection with the account's current rating") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    joinAs(transport, 1, "Alice");

    transport.receive(1, encode(Message{PlayRequest{}}));

    REQUIRE(matchmaker.isWaiting(1));
    matchmaker.enqueue(2, "Bob", kDefaultRating);
    std::optional<Pairing> pairing = matchmaker.tryPair();
    REQUIRE(pairing.has_value());
    CHECK(pairing->a.rating == kDefaultRating);
}

TEST_CASE("PLAY from a connection that never logged in is ignored") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};

    transport.connect(1);
    transport.receive(1, encode(Message{PlayRequest{}}));

    CHECK_FALSE(matchmaker.isWaiting(1));
}

TEST_CASE("a move intent from a connection with no active session is dropped") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    joinAs(transport, 1, "Alice");

    transport.receive(1, encode(Message{MoveIntent{Position{1, 4}, Position{3, 4}}}));

    CHECK_FALSE(lastSnapshotTo(transport, 1).has_value());
}

TEST_CASE("a move intent from a seated connection is routed to its session and applied") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    MatchId matchId = sessions.createSession(boardWithRook());
    GameSession* session = sessions.find(matchId);
    REQUIRE(session != nullptr);
    session->claimColor(Color::kWhite, 1, "Alice", kDefaultRating);

    // A single-square hop, so the route's next cell is the final destination.
    // The session runs on its own thread once created, so this only sends the
    // intent — the session's own ticking (not a direct tick() call here,
    // which would race it) is what applies it.
    transport.receive(1, encode(Message{MoveIntent{Position{4, 4}, Position{4, 5}}}));

    REQUIRE(waitUntil([&] {
        std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
        return state && !state->pieces.empty() &&
               state->pieces.front().movingTo == Position{4, 5};
    }));
}

TEST_CASE("a jump intent from a seated connection is routed to its session and applied") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    MatchId matchId = sessions.createSession(boardWithRook());
    GameSession* session = sessions.find(matchId);
    REQUIRE(session != nullptr);
    session->claimColor(Color::kWhite, 1, "Alice", kDefaultRating);

    transport.receive(1, encode(Message{JumpIntent{Position{4, 4}}}));

    REQUIRE(waitUntil([&] {
        std::optional<GameStateView> state = lastSnapshotTo(transport, 1);
        return state && !state->pieces.empty() &&
               state->pieces.front().state == kfc::model::PieceState::kAirborne;
    }));
}

TEST_CASE("a disconnect while waiting for a match removes the connection from the queue") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    joinAs(transport, 1, "Alice");
    transport.receive(1, encode(Message{PlayRequest{}}));
    REQUIRE(matchmaker.isWaiting(1));

    transport.disconnect(1);

    CHECK_FALSE(matchmaker.isWaiting(1));
}

TEST_CASE("a disconnect mid-game starts that session's forfeit countdown instead of ending it immediately") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    FakeUserStore users;
    Matchmaker matchmaker;
    SessionManager sessions{transport, users, log};
    ServerApp app{transport, log, matchmaker, sessions, users};
    MatchId matchId = sessions.createSession(boardWithRook());
    GameSession* session = sessions.find(matchId);
    REQUIRE(session != nullptr);
    session->claimColor(Color::kWhite, 1, "Alice", kDefaultRating);
    session->claimColor(Color::kBlack, 2, "Bob", kDefaultRating);

    transport.disconnect(1);

    CHECK_FALSE(session->isFinished());
    // The full 20-second countdown-to-forfeit mechanics are already covered
    // synchronously in test_game_session.cpp; here it is enough to see that
    // ServerApp actually delegated to onDisconnect — evidenced by Bob (the
    // connected side) receiving the countdown ticks that only start once it
    // has.
    REQUIRE(waitUntil([&] {
        std::optional<Message> message = lastMessageToSync(transport, 2);
        return message && std::holds_alternative<kfc::protocol::CountdownTick>(*message);
    }));
}
