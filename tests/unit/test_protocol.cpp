#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "model/piece.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

using kfc::model::Color;
using kfc::protocol::Assigned;
using kfc::protocol::decode;
using kfc::protocol::encode;
using kfc::protocol::JoinRequest;
using kfc::protocol::Message;
using kfc::protocol::Rejected;

TEST_CASE("a join request round-trips through the codec") {
    std::optional<Message> decoded = decode(encode(Message{JoinRequest{"Alice"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<JoinRequest>(*decoded));
    CHECK(std::get<JoinRequest>(*decoded).name == "Alice");
}

TEST_CASE("an assignment round-trips carrying the colour") {
    for (Color color : {Color::kWhite, Color::kBlack}) {
        std::optional<Message> decoded = decode(encode(Message{Assigned{color}}));

        REQUIRE(decoded.has_value());
        REQUIRE(std::holds_alternative<Assigned>(*decoded));
        CHECK(std::get<Assigned>(*decoded).color == color);
    }
}

TEST_CASE("a rejection round-trips carrying the reason") {
    std::optional<Message> decoded = decode(encode(Message{Rejected{"full"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<Rejected>(*decoded));
    CHECK(std::get<Rejected>(*decoded).reason == "full");
}

TEST_CASE("decoding rejects malformed or unknown input") {
    CHECK_FALSE(decode("not json").has_value());
    CHECK_FALSE(decode("{}").has_value());
    CHECK_FALSE(decode(R"({"type":"unknown"})").has_value());
    CHECK_FALSE(decode(R"({"type":"join"})").has_value());  // missing name
    CHECK_FALSE(decode(R"({"type":"assigned","color":"x"})").has_value());
}
