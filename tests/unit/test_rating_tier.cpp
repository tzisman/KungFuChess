#include <doctest/doctest.h>

#include "view/screens/rating_tier.hpp"

using kfc::view::beltFor;

TEST_CASE("a rating earns the belt of the band it falls in") {
    CHECK(beltFor(1247) == "Green Belt");
    CHECK(beltFor(1000) == "Yellow Belt");
    CHECK(beltFor(1850) == "Black Belt");
}

// Each band has to end exactly where the next begins, or a rating between the
// two would be labelled by whichever comparison happened to be written first.
TEST_CASE("the bands meet without a gap or an overlap") {
    CHECK(beltFor(1199) == "Yellow Belt");
    CHECK(beltFor(1200) == "Green Belt");
    CHECK(beltFor(1399) == "Green Belt");
    CHECK(beltFor(1400) == "Blue Belt");
}

TEST_CASE("a rating below or above every band still earns a belt") {
    CHECK(beltFor(0) == "White Belt");
    CHECK(beltFor(999) == "White Belt");
    CHECK(beltFor(2000) == "Master");
    CHECK(beltFor(4000) == "Master");
}
