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

TEST_CASE("a register request round-trips carrying username and password") {
    std::optional<Message> decoded =
        decode(encode(Message{RegisterRequest{"alice", "hunter2"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<RegisterRequest>(*decoded));
    CHECK(std::get<RegisterRequest>(*decoded).username == "alice");
    CHECK(std::get<RegisterRequest>(*decoded).password == "hunter2");
}

TEST_CASE("a login request round-trips carrying username and password") {
    std::optional<Message> decoded =
        decode(encode(Message{LoginRequest{"alice", "hunter2"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<LoginRequest>(*decoded));
    CHECK(std::get<LoginRequest>(*decoded).username == "alice");
    CHECK(std::get<LoginRequest>(*decoded).password == "hunter2");
}

TEST_CASE("a registered confirmation round-trips") {
    std::optional<Message> decoded = decode(encode(Message{Registered{}}));

    REQUIRE(decoded.has_value());
    CHECK(std::holds_alternative<Registered>(*decoded));
}

TEST_CASE("an auth rejection round-trips carrying the reason") {
    std::optional<Message> decoded =
        decode(encode(Message{AuthRejected{"invalid_credentials"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<AuthRejected>(*decoded));
    CHECK(std::get<AuthRejected>(*decoded).reason == "invalid_credentials");
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
    CHECK_FALSE(decode(R"({"type":"register","username":"alice"})").has_value());  // missing password
    CHECK_FALSE(decode(R"({"type":"login"})").has_value());  // missing username and password
    CHECK_FALSE(decode(R"({"type":"assigned","color":"x"})").has_value());
    CHECK_FALSE(decode(R"({"type":"move","from":{"row":1,"col":4}})")
                    .has_value());  // missing to
    CHECK_FALSE(decode(R"({"type":"jump"})").has_value());  // missing cell
}
