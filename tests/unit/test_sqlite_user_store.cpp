#include <doctest/doctest.h>

#include "server/sqlite_user_store.hpp"

using kfc::server::kDefaultRating;
using kfc::server::SqliteUserStore;

TEST_CASE("registering a new username succeeds") {
    SqliteUserStore store{":memory:"};
    CHECK(store.registerUser("alice", "hunter2"));
}

TEST_CASE("registering an existing username is rejected") {
    SqliteUserStore store{":memory:"};
    REQUIRE(store.registerUser("alice", "hunter2"));
    CHECK_FALSE(store.registerUser("alice", "different"));
}

TEST_CASE("authenticate succeeds with the right password and fails with the wrong one") {
    SqliteUserStore store{":memory:"};
    REQUIRE(store.registerUser("alice", "hunter2"));
    CHECK(store.authenticate("alice", "hunter2").has_value());
    CHECK_FALSE(store.authenticate("alice", "wrong").has_value());
    CHECK_FALSE(store.authenticate("nobody", "hunter2").has_value());
}

TEST_CASE("a newly registered user gets the default rating") {
    SqliteUserStore store{":memory:"};
    REQUIRE(store.registerUser("alice", "hunter2"));
    CHECK(store.ratingOf("alice") == kDefaultRating);
    CHECK(store.authenticate("alice", "hunter2")->rating == kDefaultRating);
}

TEST_CASE("updateRating is reflected by both ratingOf and the next authenticate") {
    SqliteUserStore store{":memory:"};
    REQUIRE(store.registerUser("alice", "hunter2"));
    store.updateRating("alice", 1200);
    CHECK(store.ratingOf("alice") == 1200);
    CHECK(store.authenticate("alice", "hunter2")->rating == 1200);
}
