#include <doctest/doctest.h>

#include <sstream>

#include "model/position.hpp"

using kfc::model::Position;

TEST_CASE("positions with the same row and column are equal") {
    CHECK(Position{2, 3} == Position{2, 3});
}

TEST_CASE("positions differing in row or column are not equal") {
    CHECK(Position{2, 3} != Position{5, 3});  // different row
    CHECK(Position{2, 3} != Position{2, 7});  // different col
    CHECK_FALSE(Position{2, 3} == Position{5, 7});
}

TEST_CASE("positions have a readable representation for assertion failures") {
    std::ostringstream os;
    os << Position{2, 3};
    CHECK(os.str() == "Position(row=2, col=3)");
}
