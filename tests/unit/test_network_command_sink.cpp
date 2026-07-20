#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "fake_transport.hpp"
#include "input/network_command_sink.hpp"
#include "model/position.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

using kfc::input::NetworkCommandSink;
using kfc::model::Position;
using kfc::protocol::decode;
using kfc::protocol::JumpIntent;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
using kfc::test::FakeClientTransport;

TEST_CASE("requesting a move sends a decodable MoveIntent") {
    FakeClientTransport transport;
    NetworkCommandSink sink{transport};

    sink.requestMove(Position{1, 4}, Position{3, 4});

    REQUIRE(transport.sent.size() == 1);
    std::optional<Message> message = decode(transport.sent.front());
    REQUIRE(message.has_value());
    REQUIRE(std::holds_alternative<MoveIntent>(*message));
    CHECK(std::get<MoveIntent>(*message).from == Position{1, 4});
    CHECK(std::get<MoveIntent>(*message).to == Position{3, 4});
}

TEST_CASE("requesting a jump sends a decodable JumpIntent") {
    FakeClientTransport transport;
    NetworkCommandSink sink{transport};

    sink.requestJump(Position{6, 2});

    REQUIRE(transport.sent.size() == 1);
    std::optional<Message> message = decode(transport.sent.front());
    REQUIRE(message.has_value());
    REQUIRE(std::holds_alternative<JumpIntent>(*message));
    CHECK(std::get<JumpIntent>(*message).cell == Position{6, 2});
}
