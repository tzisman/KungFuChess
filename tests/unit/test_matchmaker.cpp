#include <doctest/doctest.h>

#include <optional>

#include "server/matchmaker.hpp"

using kfc::server::kMatchTimeoutMs;
using kfc::server::Matchmaker;
using kfc::server::Pairing;
using kfc::server::WaitingPlayer;

TEST_CASE("two players within the rating band are paired") {
    Matchmaker matchmaker;
    matchmaker.enqueue(1, "Alice", 1000);
    matchmaker.enqueue(2, "Bob", 1050);

    std::optional<Pairing> pairing = matchmaker.tryPair();

    REQUIRE(pairing.has_value());
    CHECK(pairing->a.username == "Alice");
    CHECK(pairing->b.username == "Bob");
    CHECK_FALSE(matchmaker.isWaiting(1));
    CHECK_FALSE(matchmaker.isWaiting(2));
}

TEST_CASE("two players outside the rating band are not paired until someone closer arrives") {
    Matchmaker matchmaker;
    matchmaker.enqueue(1, "Alice", 1000);
    matchmaker.enqueue(2, "Bob", 1150);  // 150 apart: outside the 100 band

    CHECK_FALSE(matchmaker.tryPair().has_value());

    matchmaker.enqueue(3, "Carol", 1030);  // within band of both

    std::optional<Pairing> pairing = matchmaker.tryPair();
    REQUIRE(pairing.has_value());
    CHECK(pairing->a.username == "Alice");
    CHECK(pairing->b.username == "Carol");
    CHECK(matchmaker.isWaiting(2));  // Bob is still waiting
}

TEST_CASE("a single waiting player is never paired") {
    Matchmaker matchmaker;
    matchmaker.enqueue(1, "Alice", 1000);

    CHECK_FALSE(matchmaker.tryPair().has_value());
}

TEST_CASE("a player who has waited past the timeout expires and is removed") {
    Matchmaker matchmaker;
    matchmaker.enqueue(1, "Alice", 1000);

    std::vector<WaitingPlayer> expired = matchmaker.expire(kMatchTimeoutMs - 1);
    CHECK(expired.empty());
    CHECK(matchmaker.isWaiting(1));

    expired = matchmaker.expire(1);
    REQUIRE(expired.size() == 1);
    CHECK(expired.front().username == "Alice");
    CHECK_FALSE(matchmaker.isWaiting(1));
}

TEST_CASE("dequeue removes a waiting player") {
    Matchmaker matchmaker;
    matchmaker.enqueue(1, "Alice", 1000);

    matchmaker.dequeue(1);

    CHECK_FALSE(matchmaker.isWaiting(1));
    CHECK(matchmaker.expire(kMatchTimeoutMs).empty());
}
