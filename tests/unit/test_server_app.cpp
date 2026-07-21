#include <doctest/doctest.h>

#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "common/logger.hpp"
#include "fake_transport.hpp"
#include "fake_user_store.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "server/command_queue.hpp"
#include "server/server_app.hpp"

using kfc::common::Logger;
using kfc::model::Color;
using kfc::model::Position;
using kfc::net::ConnectionId;
using kfc::server::CommandQueue;
using kfc::server::JumpCommand;
using kfc::server::MoveCommand;
using kfc::server::ServerApp;
using kfc::test::FakeServerTransport;
using kfc::test::FakeUserStore;
using kfc::protocol::Assigned;
using kfc::protocol::AuthRejected;
using kfc::protocol::decode;
using kfc::protocol::encode;
using kfc::protocol::JumpIntent;
using kfc::protocol::LoginRequest;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
using kfc::protocol::Registered;
using kfc::protocol::RegisterRequest;
using kfc::protocol::Rejected;

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

// The last message the server sent to a given connection.
std::optional<Message> lastTo(const FakeServerTransport& transport,
                              ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it)
        if (it->first == id) return decode(it->second);
    return std::nullopt;
}

}  // namespace

TEST_CASE("the first player to log in is assigned white") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");

    std::optional<Message> reply = lastTo(transport, 1);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kWhite);
}

TEST_CASE("the second player to log in is assigned black") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");
    joinAs(transport, 2, "Bob");

    std::optional<Message> reply = lastTo(transport, 2);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kBlack);
}

TEST_CASE("a third player to log in is rejected as full") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");
    joinAs(transport, 2, "Bob");
    joinAs(transport, 3, "Carol");

    std::optional<Message> reply = lastTo(transport, 3);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Rejected>(*reply));
    CHECK(std::get<Rejected>(*reply).reason == "full");
}

TEST_CASE("a disconnect frees the colour for the next joiner") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");  // white
    joinAs(transport, 2, "Bob");    // black
    transport.disconnect(1);        // white is freed

    joinAs(transport, 3, "Carol");

    std::optional<Message> reply = lastTo(transport, 3);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kWhite);
}

TEST_CASE("a logged-in player's move intent is queued tagged with their colour") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");  // white
    transport.receive(
        1, encode(Message{MoveIntent{Position{1, 4}, Position{3, 4}}}));

    std::vector<kfc::server::PlayerCommand> queued = commands.drain();
    REQUIRE(queued.size() == 1);
    REQUIRE(std::holds_alternative<MoveCommand>(queued.front()));
    const MoveCommand& move = std::get<MoveCommand>(queued.front());
    CHECK(move.color == Color::kWhite);
    CHECK(move.from == Position{1, 4});
    CHECK(move.to == Position{3, 4});
}

TEST_CASE("a logged-in player's jump intent is queued tagged with their colour") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    joinAs(transport, 1, "Alice");  // white
    joinAs(transport, 2, "Bob");    // black
    transport.receive(2, encode(Message{JumpIntent{Position{6, 2}}}));

    std::vector<kfc::server::PlayerCommand> queued = commands.drain();
    REQUIRE(queued.size() == 1);
    REQUIRE(std::holds_alternative<JumpCommand>(queued.front()));
    const JumpCommand& jump = std::get<JumpCommand>(queued.front());
    CHECK(jump.color == Color::kBlack);
    CHECK(jump.cell == Position{6, 2});
}

TEST_CASE("a move intent from a connection that never logged in is dropped") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    transport.connect(1);
    transport.receive(
        1, encode(Message{MoveIntent{Position{1, 4}, Position{3, 4}}}));

    CHECK(commands.drain().empty());
}

TEST_CASE("registering an existing username is rejected") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

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
    CommandQueue commands;
    FakeUserStore users;
    ServerApp app{transport, log, commands, users};

    transport.connect(1);
    transport.receive(1, registerMessage("Alice"));
    transport.receive(1, encode(Message{LoginRequest{"Alice", "wrong-password"}}));

    std::optional<Message> reply = lastTo(transport, 1);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<AuthRejected>(*reply));
    CHECK(std::get<AuthRejected>(*reply).reason == "invalid_credentials");
}
