#include "input/board_mapper.hpp"

#include <utility>

namespace kfc::input {

BoardMapper::BoardMapper(view::BoardGeometry geometry)
    : geometry_(std::move(geometry)) {}

void BoardMapper::setGeometry(view::BoardGeometry geometry) {
    geometry_ = std::move(geometry);
}

const view::BoardGeometry& BoardMapper::geometry() const { return geometry_; }

std::optional<model::Position> BoardMapper::toCell(int x, int y) const {
    return geometry_.cellAt(x, y);
}

}
