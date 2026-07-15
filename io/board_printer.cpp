#include "io/board_printer.hpp"

#include "io/piece_codec.hpp"
#include "model/position.hpp"

namespace kfc::io {
namespace {

constexpr char kErrorPrefix[] = "ERROR ";

}

void printParseError(const ParseError& error, std::ostream& out) {
    out << kErrorPrefix << error.code << '\n';
}

void printBoard(const model::Board& board, std::ostream& out) {
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            if (col > 0) out << ' ';
            out << encodeCell(board.pieceAt(model::Position{row, col}));
        }
        out << '\n';
    }
}

}
