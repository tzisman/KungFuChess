#include <doctest/doctest.h>

#include "input/board_mapper.hpp"
#include "model/position.hpp"
#include "view/board_geometry.hpp"

using kfc::input::BoardMapper;
using kfc::model::Position;
using kfc::view::BoardGeometry;

namespace {

// An 8x8 board drawn at 100 pixels per cell.
BoardMapper mapperOn100PxGrid() {
    return BoardMapper{BoardGeometry{800, 800, 8, 8}};
}

}  // namespace

TEST_CASE("a click inside a cell maps to that cell") {
    BoardMapper mapper = mapperOn100PxGrid();

    CHECK(mapper.toCell(50, 50) == Position{0, 0});
    CHECK(mapper.toCell(150, 50) == Position{0, 1});
    CHECK(mapper.toCell(50, 150) == Position{1, 0});
}

TEST_CASE("the top-left of a cell maps to the same cell as its interior") {
    BoardMapper mapper = mapperOn100PxGrid();

    CHECK(mapper.toCell(100, 100) == Position{1, 1});
    CHECK(mapper.toCell(199, 199) == Position{1, 1});
}

TEST_CASE("a click past the right or bottom edge is outside the board") {
    BoardMapper mapper = mapperOn100PxGrid();

    CHECK_FALSE(mapper.toCell(800, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, 800).has_value());
}

TEST_CASE("a negative pixel is outside the board") {
    BoardMapper mapper = mapperOn100PxGrid();

    CHECK_FALSE(mapper.toCell(-1, 50).has_value());
    CHECK_FALSE(mapper.toCell(50, -1).has_value());
}

TEST_CASE("the last cell is still inside the board") {
    BoardMapper mapper = mapperOn100PxGrid();

    CHECK(mapper.toCell(750, 750) == Position{7, 7});
}

TEST_CASE("cells are spread evenly when the image does not divide exactly") {
    BoardMapper mapper{BoardGeometry{822, 828, 8, 8}};

    CHECK(mapper.toCell(0, 0) == Position{0, 0});
    CHECK(mapper.toCell(821, 827) == Position{7, 7});
    CHECK_FALSE(mapper.toCell(822, 400).has_value());
}

TEST_CASE("a click follows the board when the board is drawn away from the corner") {
    BoardMapper mapper{BoardGeometry{800, 800, 8, 8, kfc::view::Pixel{300, 0}}};

    CHECK(mapper.toCell(350, 50) == Position{0, 0});
    CHECK(mapper.toCell(450, 50) == Position{0, 1});
    CHECK(mapper.toCell(1050, 750) == Position{7, 7});
}

TEST_CASE("a click beside an offset board is outside it") {
    BoardMapper mapper{BoardGeometry{800, 800, 8, 8, kfc::view::Pixel{300, 0}}};

    CHECK_FALSE(mapper.toCell(50, 50).has_value());
    CHECK_FALSE(mapper.toCell(1150, 50).has_value());
}
