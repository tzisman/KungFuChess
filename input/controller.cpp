#include "input/controller.hpp"

#include "model/piece.hpp"

namespace kfc::input {

Controller::Controller(const model::Board& board, CommandSink& commands,
                       BoardMapper mapper)
    : board_(board), commands_(commands), mapper_(mapper) {}

void Controller::handleClick(int x, int y) {
    std::optional<model::Position> cell = mapper_.toCell(x, y);
    if (!selected_) {
        handleFirstClick(cell);
    } else {
        handleSecondClick(cell);
    }
}

void Controller::handleJump(int x, int y) {
    std::optional<model::Position> cell = mapper_.toCell(x, y);
    if (cell) commands_.requestJump(*cell);
}

const std::optional<model::Position>& Controller::selection() const {
    return selected_;
}

void Controller::handleFirstClick(std::optional<model::Position> cell) {
    if (!cell) return;
    if (!board_.pieceAt(*cell)) return;
    selected_ = cell;
}

void Controller::handleSecondClick(std::optional<model::Position> cell) {
    if (!cell) {
        selected_.reset();
        return;
    }
    if (isOwnPiece(*cell)) {
        selected_ = cell;
        return;
    }
    commands_.requestMove(*selected_, *cell);
    selected_.reset();
}

bool Controller::isOwnPiece(model::Position cell) const {
    std::optional<model::Piece> target = board_.pieceAt(cell);
    std::optional<model::Piece> selectedPiece = board_.pieceAt(*selected_);
    return target && selectedPiece && target->color() == selectedPiece->color();
}

}
