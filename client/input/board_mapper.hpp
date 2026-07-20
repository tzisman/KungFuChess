#pragma once

#include <optional>

#include "model/position.hpp"
#include "view/board_geometry.hpp"

namespace kfc::input {

// Turns a pixel the user clicked into a board cell. The measurements come from
// the geometry it is given, so the same mapper serves any board size; call
// setGeometry when the board is displayed at a different size.
class BoardMapper {
public:
    explicit BoardMapper(view::BoardGeometry geometry);

    void setGeometry(view::BoardGeometry geometry);
    const view::BoardGeometry& geometry() const;

    std::optional<model::Position> toCell(int x, int y) const;

private:
    view::BoardGeometry geometry_;
};

}
