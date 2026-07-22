#include <doctest/doctest.h>

#include "view/panel_layout.hpp"

using kfc::view::PanelLayout;

TEST_CASE("the board sits between the two panels, inset by its coordinate gutter") {
    PanelLayout layout{800, 800};

    CHECK(layout.coordGutter() > 0);
    CHECK(layout.boardOrigin().x == layout.panelWidth() + layout.coordGutter());
    CHECK(layout.boardOrigin().y == layout.coordGutter());
    CHECK(layout.leftPanelOrigin().x == 0);
    CHECK(layout.rightPanelOrigin().x ==
          layout.panelWidth() + 800 + 2 * layout.coordGutter());
}

// The coordinates are written in the gutter, so it has to be genuinely clear
// space on all four sides of the board — not an inset that only the top-left
// corner honours, which would crop the labels on the other two.
TEST_CASE("the gutter leaves an equal clear strip on every side of the board") {
    PanelLayout layout{800, 800};
    int gutter = layout.coordGutter();

    CHECK(layout.boardOrigin().x - layout.panelWidth() == gutter);
    CHECK(layout.boardOrigin().y == gutter);
    CHECK(layout.rightPanelOrigin().x - (layout.boardOrigin().x + 800) == gutter);
    CHECK(layout.canvasHeight() - (layout.boardOrigin().y + 800) == gutter);
}

TEST_CASE("the panels start level with the board's top edge") {
    PanelLayout layout{800, 800};

    CHECK(layout.leftPanelOrigin().y == layout.boardOrigin().y);
    CHECK(layout.rightPanelOrigin().y == layout.boardOrigin().y);
}

TEST_CASE("the canvas is exactly the board, its gutter and both panels") {
    PanelLayout layout{800, 800};

    CHECK(layout.canvasWidth() ==
          800 + 2 * layout.panelWidth() + 2 * layout.coordGutter());
    CHECK(layout.canvasHeight() == 800 + 2 * layout.coordGutter());
}

// The canvas is built at the window's size so that the toolkit shows it
// unscaled and a click lands where it was aimed; the board has to be centred in
// it rather than clinging to the top-left corner.
TEST_CASE("a canvas larger than the layout needs splits the slack around it") {
    PanelLayout exact{800, 800};
    PanelLayout roomy{800, 800, exact.canvasWidth() + 200,
                      exact.canvasHeight() + 100};

    CHECK(roomy.canvasWidth() == exact.canvasWidth() + 200);
    CHECK(roomy.canvasHeight() == exact.canvasHeight() + 100);
    CHECK(roomy.boardOrigin().x == exact.boardOrigin().x + 100);
    CHECK(roomy.boardOrigin().y == exact.boardOrigin().y + 50);
    CHECK(roomy.leftPanelOrigin().x == exact.leftPanelOrigin().x + 100);
    CHECK(roomy.rightPanelOrigin().x == exact.rightPanelOrigin().x + 100);
    CHECK(roomy.leftPanelOrigin().y == roomy.boardOrigin().y);
}

// Otherwise the board would be drawn off the edge of its own canvas.
TEST_CASE("a canvas smaller than the layout needs is widened to hold it") {
    PanelLayout exact{800, 800};
    PanelLayout cramped{800, 800, 100, 100};

    CHECK(cramped.canvasWidth() == exact.canvasWidth());
    CHECK(cramped.canvasHeight() == exact.canvasHeight());
    CHECK(cramped.boardOrigin().x == exact.boardOrigin().x);
    CHECK(cramped.boardOrigin().y == exact.boardOrigin().y);
}

TEST_CASE("the panels scale with the board rather than being fixed") {
    PanelLayout small{400, 400};
    PanelLayout large{800, 800};

    CHECK(large.panelWidth() == 2 * small.panelWidth());
    CHECK(large.coordGutter() == 2 * small.coordGutter());
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
    CHECK(layout.leftPanelOrigin().y +
              layout.lineY(layout.firstMoveLine() + layout.maxVisibleMoves() - 1) <=
          layout.canvasHeight());
}

TEST_CASE("text is drawn smaller than the line it sits on") {
    PanelLayout layout{800, 800};

    CHECK(layout.textHeight() > 0);
    CHECK(layout.textHeight() < layout.lineHeight());
}
