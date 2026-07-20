#include <doctest/doctest.h>

#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "common/logger.hpp"
#include "fake_transport.hpp"
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
using kfc::protocol::Assigned;
using kfc::protocol::decode;
using kfc::protocol::encode;
using kfc::protocol::JoinRequest;
using kfc::protocol::JumpIntent;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
using kfc::protocol::Rejected;

namespace {

std::string joinMessage(const std::string& name) {
    return encode(Message{JoinRequest{name}});
}

// The last message the server sent to a given connection.
std::optional<Message> lastTo(const FakeServerTransport& transport,
                              ConnectionId id) {
    for (auto it = transport.sent.rbegin(); it != transport.sent.rend(); ++it)
        if (it->first == id) return decode(it->second);
    return std::nullopt;
}

}  // namespace

TEST_CASE("the first player to join is assigned white") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));

    std::optional<Message> reply = lastTo(transport, 1);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kWhite);
}

TEST_CASE("the second player to join is assigned black") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));
    transport.connect(2);
    transport.receive(2, joinMessage("Bob"));

    std::optional<Message> reply = lastTo(transport, 2);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kBlack);
}

TEST_CASE("a third player to join is rejected as full") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));
    transport.connect(2);
    transport.receive(2, joinMessage("Bob"));
    transport.connect(3);
    transport.receive(3, joinMessage("Carol"));

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
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));  // white
    transport.connect(2);
    transport.receive(2, joinMessage("Bob"));  // black
    transport.disconnect(1);                    // white is freed

    transport.connect(3);
    transport.receive(3, joinMessage("Carol"));

    std::optional<Message> reply = lastTo(transport, 3);
    REQUIRE(reply.has_value());
    REQUIRE(std::holds_alternative<Assigned>(*reply));
    CHECK(std::get<Assigned>(*reply).color == Color::kWhite);
}

TEST_CASE("a joined player's move intent is queued tagged with their colour") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));  // white
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

TEST_CASE("a joined player's jump intent is queued tagged with their colour") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(1, joinMessage("Alice"));  // white
    transport.connect(2);
    transport.receive(2, joinMessage("Bob"));  // black
    transport.receive(2, encode(Message{JumpIntent{Position{6, 2}}}));

    std::vector<kfc::server::PlayerCommand> queued = commands.drain();
    REQUIRE(queued.size() == 1);
    REQUIRE(std::holds_alternative<JumpCommand>(queued.front()));
    const JumpCommand& jump = std::get<JumpCommand>(queued.front());
    CHECK(jump.color == Color::kBlack);
    CHECK(jump.cell == Position{6, 2});
}

TEST_CASE("a move intent from a connection that never joined is dropped") {
    std::ostringstream quiet;
    Logger log{"TEST", quiet};
    FakeServerTransport transport;
    CommandQueue commands;
    ServerApp app{transport, log, commands};

    transport.connect(1);
    transport.receive(
        1, encode(Message{MoveIntent{Position{1, 4}, Position{3, 4}}}));

    CHECK(commands.drain().empty());
}
