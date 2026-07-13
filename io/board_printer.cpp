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

void printBoard(const engine::GameSnapshot& snapshot, std::ostream& out) {
    for (int row = 0; row < snapshot.height(); ++row) {
        for (int col = 0; col < snapshot.width(); ++col) {
            if (col > 0) out << ' ';
            out << encodeCell(snapshot.pieceAt(model::Position{row, col}));
        }
        out << '\n';
    }
}

}
