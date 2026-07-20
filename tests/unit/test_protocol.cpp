#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "model/piece.hpp"
#include "model/position.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

using kfc::model::Color;
using kfc::model::Position;
using kfc::protocol::Assigned;
using kfc::protocol::decode;
using kfc::protocol::encode;
using kfc::protocol::JoinRequest;
using kfc::protocol::JumpIntent;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
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

TEST_CASE("a move intent round-trips carrying from and to") {
    std::optional<Message> decoded = decode(
        encode(Message{MoveIntent{Position{1, 4}, Position{3, 4}}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<MoveIntent>(*decoded));
    CHECK(std::get<MoveIntent>(*decoded).from == Position{1, 4});
    CHECK(std::get<MoveIntent>(*decoded).to == Position{3, 4});
}

TEST_CASE("a jump intent round-trips carrying the cell") {
    std::optional<Message> decoded =
        decode(encode(Message{JumpIntent{Position{6, 2}}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<JumpIntent>(*decoded));
    CHECK(std::get<JumpIntent>(*decoded).cell == Position{6, 2});
}

TEST_CASE("decoding rejects malformed or unknown input") {
    CHECK_FALSE(decode("not json").has_value());
    CHECK_FALSE(decode("{}").has_value());
    CHECK_FALSE(decode(R"({"type":"unknown"})").has_value());
    CHECK_FALSE(decode(R"({"type":"join"})").has_value());  // missing name
    CHECK_FALSE(decode(R"({"type":"assigned","color":"x"})").has_value());
    CHECK_FALSE(decode(R"({"type":"move","from":{"row":1,"col":4}})")
                    .has_value());  // missing to
    CHECK_FALSE(decode(R"({"type":"jump"})").has_value());  // missing cell
}
