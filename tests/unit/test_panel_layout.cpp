#include <doctest/doctest.h>

#include "view/panel_layout.hpp"

using kfc::view::PanelLayout;

TEST_CASE("the board sits between the two panels") {
    PanelLayout layout{800, 800};

    CHECK(layout.boardOrigin().x == layout.panelWidth());
    CHECK(layout.boardOrigin().y == 0);
    CHECK(layout.leftPanelOrigin().x == 0);
    CHECK(layout.rightPanelOrigin().x == layout.panelWidth() + 800);
}

TEST_CASE("the canvas is exactly the board and both panels") {
    PanelLayout layout{800, 800};

    CHECK(layout.canvasWidth() == 800 + 2 * layout.panelWidth());
    CHECK(layout.canvasHeight() == 800);
}

TEST_CASE("the panels scale with the board rather than being fixed") {
    PanelLayout small{400, 400};
    PanelLayout large{800, 800};

    CHECK(large.panelWidth() == 2 * small.panelWidth());
    CHECK(large.lineHeight() > small.lineHeight());
    CHECK(large.textHeight() > small.textHeight());
}

TEST_CASE("lines run down the panel in order and clear its top edge") {
    PanelLayout layout{800, 800};

    CHECK(layout.lineY(layout.nameLine()) > 0);
    CHECK(layout.lineY(layout.scoreLine()) > layout.lineY(layout.nameLine()));
    CHECK(layout.lineY(layout.headerLine()) > layout.lineY(layout.scoreLine()));
    CHECK(layout.lineY(layout.firstMoveLine()) >
          layout.lineY(layout.headerLine()));
}

TEST_CASE("the move column sits to the right of the time column") {
    PanelLayout layout{800, 800};

    CHECK(layout.timeColumnX() >= 0);
    CHECK(layout.moveColumnX() > layout.timeColumnX());
    CHECK(layout.moveColumnX() < layout.panelWidth());
}

TEST_CASE("every visible move fits inside the panel") {
    PanelLayout layout{800, 800};

    CHECK(layout.maxVisibleMoves() > 0);
    CHECK(layout.lineY(layout.firstMoveLine() + layout.maxVisibleMoves() - 1) <=
          layout.canvasHeight());
}

TEST_CASE("text is drawn smaller than the line it sits on") {
    PanelLayout layout{800, 800};

    CHECK(layout.textHeight() > 0);
    CHECK(layout.textHeight() < layout.lineHeight());
}
