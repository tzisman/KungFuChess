#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "model/piece.hpp"
#include "model/position.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

using kfc::model::Color;
using kfc::model::Position;
using kfc::protocol::AuthRejected;
using kfc::protocol::CountdownTick;
using kfc::protocol::decode;
using kfc::protocol::encode;
using kfc::protocol::EnterRoomRequest;
using kfc::protocol::JumpIntent;
using kfc::protocol::LoggedIn;
using kfc::protocol::LoginRequest;
using kfc::protocol::Matched;
using kfc::protocol::Message;
using kfc::protocol::MoveIntent;
using kfc::protocol::NoOpponent;
using kfc::protocol::PlayRequest;
using kfc::protocol::Registered;
using kfc::protocol::RegisterRequest;
using kfc::protocol::Role;
using kfc::protocol::RoomJoined;

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

TEST_CASE("a logged-in confirmation round-trips carrying the rating") {
    std::optional<Message> decoded = decode(encode(Message{LoggedIn{1200}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<LoggedIn>(*decoded));
    CHECK(std::get<LoggedIn>(*decoded).rating == 1200);
}

TEST_CASE("a play request round-trips") {
    std::optional<Message> decoded = decode(encode(Message{PlayRequest{}}));

    REQUIRE(decoded.has_value());
    CHECK(std::holds_alternative<PlayRequest>(*decoded));
}

TEST_CASE("a matched notice round-trips carrying the colour and opponent name") {
    for (Color color : {Color::kWhite, Color::kBlack}) {
        std::optional<Message> decoded =
            decode(encode(Message{Matched{color, "Alice"}}));

        REQUIRE(decoded.has_value());
        REQUIRE(std::holds_alternative<Matched>(*decoded));
        CHECK(std::get<Matched>(*decoded).color == color);
        CHECK(std::get<Matched>(*decoded).opponentName == "Alice");
    }
}

TEST_CASE("a no-opponent notice round-trips") {
    std::optional<Message> decoded = decode(encode(Message{NoOpponent{}}));

    REQUIRE(decoded.has_value());
    CHECK(std::holds_alternative<NoOpponent>(*decoded));
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

TEST_CASE("a countdown tick round-trips carrying the seconds left") {
    std::optional<Message> decoded = decode(encode(Message{CountdownTick{12}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<CountdownTick>(*decoded));
    CHECK(std::get<CountdownTick>(*decoded).secondsLeft == 12);
}

TEST_CASE("an enter-room request round-trips carrying the room name") {
    std::optional<Message> decoded =
        decode(encode(Message{EnterRoomRequest{"lounge"}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<EnterRoomRequest>(*decoded));
    CHECK(std::get<EnterRoomRequest>(*decoded).roomName == "lounge");
}

TEST_CASE("a room-joined notice round-trips carrying the colour for a player seat") {
    for (Color color : {Color::kWhite, Color::kBlack}) {
        Role role = color == Color::kWhite ? Role::kWhite : Role::kBlack;
        std::optional<Message> decoded =
            decode(encode(Message{RoomJoined{role, color}}));

        REQUIRE(decoded.has_value());
        REQUIRE(std::holds_alternative<RoomJoined>(*decoded));
        CHECK(std::get<RoomJoined>(*decoded).role == role);
        REQUIRE(std::get<RoomJoined>(*decoded).color.has_value());
        CHECK(*std::get<RoomJoined>(*decoded).color == color);
    }
}

TEST_CASE("a room-joined notice round-trips with no colour for a spectator seat") {
    std::optional<Message> decoded =
        decode(encode(Message{RoomJoined{Role::kSpectator, std::nullopt}}));

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<RoomJoined>(*decoded));
    CHECK(std::get<RoomJoined>(*decoded).role == Role::kSpectator);
    CHECK_FALSE(std::get<RoomJoined>(*decoded).color.has_value());
}

TEST_CASE("decoding rejects malformed or unknown input") {
    CHECK_FALSE(decode("not json").has_value());
    CHECK_FALSE(decode("{}").has_value());
    CHECK_FALSE(decode(R"({"type":"unknown"})").has_value());
    CHECK_FALSE(decode(R"({"type":"register","username":"alice"})").has_value());  // missing password
    CHECK_FALSE(decode(R"({"type":"login"})").has_value());  // missing username and password
    CHECK_FALSE(decode(R"({"type":"matched","color":"x"})").has_value());
    CHECK_FALSE(decode(R"({"type":"move","from":{"row":1,"col":4}})")
                    .has_value());  // missing to
    CHECK_FALSE(decode(R"({"type":"jump"})").has_value());  // missing cell
    CHECK_FALSE(decode(R"({"type":"enter_room"})").has_value());  // missing room_name
    CHECK_FALSE(decode(R"({"type":"room_joined","role":"x"})").has_value());  // bad role
}
