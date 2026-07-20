#include <doctest/doctest.h>

#include <optional>
#include <string>

#include "model/piece.hpp"
#include "server/player_names.hpp"

using kfc::model::Color;
using kfc::server::PlayerNames;

TEST_CASE("a colour nobody has joined as has no registered name") {
    PlayerNames names;

    CHECK_FALSE(names.get(Color::kWhite).has_value());
}

TEST_CASE("a name set for a colour is the one later returned") {
    PlayerNames names;

    names.set(Color::kWhite, "Alice");

    std::optional<std::string> got = names.get(Color::kWhite);
    REQUIRE(got.has_value());
    CHECK(*got == "Alice");
}

TEST_CASE("setting a colour again replaces its previous name") {
    PlayerNames names;
    names.set(Color::kBlack, "Bob");

    names.set(Color::kBlack, "Robert");

    CHECK(*names.get(Color::kBlack) == "Robert");
}

TEST_CASE("names for each colour are independent") {
    PlayerNames names;
    names.set(Color::kWhite, "Alice");
    names.set(Color::kBlack, "Bob");

    CHECK(*names.get(Color::kWhite) == "Alice");
    CHECK(*names.get(Color::kBlack) == "Bob");
}
