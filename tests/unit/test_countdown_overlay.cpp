#include <doctest/doctest.h>

#include "view/countdown_overlay.hpp"

using kfc::view::countdownText;

TEST_CASE("the countdown overlay formats seconds left as plain text") {
    CHECK(countdownText(0) == "COUNTDOWN: 0");
    CHECK(countdownText(1) == "COUNTDOWN: 1");
    CHECK(countdownText(20) == "COUNTDOWN: 20");
}
