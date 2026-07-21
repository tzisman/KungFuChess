#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "fake_transport.hpp"
#include "input/network_lobby_sink.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

using kfc::input::NetworkLobbySink;
using kfc::protocol::decode;
using kfc::protocol::Message;
using kfc::protocol::PlayRequest;
using kfc::test::FakeClientTransport;

TEST_CASE("requesting to play sends a decodable PlayRequest") {
    FakeClientTransport transport;
    NetworkLobbySink sink{transport};

    sink.requestPlay();

    REQUIRE(transport.sent.size() == 1);
    std::optional<Message> message = decode(transport.sent.front());
    REQUIRE(message.has_value());
    CHECK(std::holds_alternative<PlayRequest>(*message));
}

TEST_CASE("requesting to enter a room sends nothing yet") {
    FakeClientTransport transport;
    NetworkLobbySink sink{transport};

    sink.requestEnterRoom("dojo");

    CHECK(transport.sent.empty());
}
