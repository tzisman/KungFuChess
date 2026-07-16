#include <doctest/doctest.h>

#include "model/position.hpp"
#include "view/board_geometry.hpp"

using kfc::model::Position;
using kfc::view::BoardGeometry;

TEST_CASE("a cell maps to its pixels and back again") {
    BoardGeometry geometry{800, 800, 8, 8};

    CHECK(geometry.cellWidth() == 100);
    CHECK(geometry.cellHeight() == 100);
    CHECK(geometry.topLeftOf(Position{0, 0}).x == 0);
    CHECK(geometry.topLeftOf(Position{0, 0}).y == 0);
    CHECK(geometry.topLeftOf(Position{1, 2}).x == 200);
    CHECK(geometry.topLeftOf(Position{1, 2}).y == 100);
}

TEST_CASE("a pixel outside the image belongs to no cell") {
    BoardGeometry geometry{800, 800, 8, 8};

    CHECK_FALSE(geometry.cellAt(-1, 0).has_value());
    CHECK_FALSE(geometry.cellAt(0, -1).has_value());
    CHECK_FALSE(geometry.cellAt(800, 0).has_value());
    CHECK_FALSE(geometry.cellAt(0, 800).has_value());
}

TEST_CASE("the corners of the board land on the corner cells") {
    BoardGeometry geometry{800, 800, 8, 8};

    CHECK(geometry.cellAt(0, 0) == Position{0, 0});
    CHECK(geometry.cellAt(799, 799) == Position{7, 7});
    CHECK(geometry.cellAt(799, 0) == Position{0, 7});
    CHECK(geometry.cellAt(0, 799) == Position{7, 0});
}

// The real board image is 822x828, which divides by 8 into a fraction. A cell
// size rounded once and reused would drift across the board and misplace the
// far cells, so the mapping is checked on that exact size.
TEST_CASE("cells stay aligned when the image does not divide evenly") {
    BoardGeometry geometry{822, 828, 8, 8};

    CHECK(geometry.cellAt(0, 0) == Position{0, 0});
    CHECK(geometry.cellAt(821, 827) == Position{7, 7});
    CHECK_FALSE(geometry.cellAt(822, 400).has_value());
}

TEST_CASE("every cell's own pixels map back to that cell") {
    BoardGeometry geometry{822, 828, 8, 8};

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Position cell{row, col};
            kfc::view::Pixel topLeft = geometry.topLeftOf(cell);

            REQUIRE(geometry.cellAt(topLeft.x, topLeft.y) == cell);
            REQUIRE(geometry.cellAt(topLeft.x + geometry.cellWidth() / 2,
                                    topLeft.y + geometry.cellHeight() / 2) == cell);
        }
    }
}

TEST_CASE("a board that is not square or not eight by eight still maps") {
    BoardGeometry geometry{300, 200, 3, 2};

    CHECK(geometry.cellWidth() == 100);
    CHECK(geometry.cellHeight() == 100);
    CHECK(geometry.cellAt(250, 150) == Position{1, 2});
    CHECK(geometry.topLeftOf(Position{1, 2}).x == 200);
    CHECK(geometry.topLeftOf(Position{1, 2}).y == 100);
}

TEST_CASE("a board drawn at the corner starts there by default") {
    BoardGeometry geometry{800, 800, 8, 8};

    CHECK(geometry.origin().x == 0);
    CHECK(geometry.origin().y == 0);
}

TEST_CASE("an offset board is drawn from its origin") {
    BoardGeometry geometry{800, 800, 8, 8, kfc::view::Pixel{300, 20}};

    CHECK(geometry.topLeftOf(Position{0, 0}).x == 300);
    CHECK(geometry.topLeftOf(Position{0, 0}).y == 20);
    CHECK(geometry.topLeftOf(Position{1, 2}).x == 500);
    CHECK(geometry.topLeftOf(Position{1, 2}).y == 120);
}

// Drawing and clicking must agree about where a square is, or a board pushed
// aside to make room for anything else would take clicks meant for its
// neighbour.
TEST_CASE("a click on an offset board lands on the square that was drawn there") {
    BoardGeometry geometry{800, 800, 8, 8, kfc::view::Pixel{300, 20}};

    CHECK(geometry.cellAt(300, 20) == Position{0, 0});
    CHECK(geometry.cellAt(1099, 819) == Position{7, 7});
    CHECK(geometry.cellAt(550, 270) == Position{2, 2});
}

TEST_CASE("a pixel beside an offset board belongs to no cell") {
    BoardGeometry geometry{800, 800, 8, 8, kfc::view::Pixel{300, 20}};

    CHECK_FALSE(geometry.cellAt(299, 400).has_value());
    CHECK_FALSE(geometry.cellAt(1100, 400).has_value());
    CHECK_FALSE(geometry.cellAt(400, 19).has_value());
    CHECK_FALSE(geometry.cellAt(400, 820).has_value());
    CHECK_FALSE(geometry.cellAt(0, 0).has_value());
}

TEST_CASE("every cell of an offset board maps back to itself") {
    BoardGeometry geometry{822, 828, 8, 8, kfc::view::Pixel{345, 7}};

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Position cell{row, col};
            kfc::view::Pixel topLeft = geometry.topLeftOf(cell);

            REQUIRE(geometry.cellAt(topLeft.x, topLeft.y) == cell);
            REQUIRE(geometry.cellAt(topLeft.x + geometry.cellWidth() / 2,
                                    topLeft.y + geometry.cellHeight() / 2) == cell);
        }
    }
}
