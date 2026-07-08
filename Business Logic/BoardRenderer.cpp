#include "BoardRenderer.hpp"

namespace kfc::logic {

void printBoard(const Board& board, std::ostream& out) {
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            if (col > 0) out << ' ';
            out << board.tokenAt(Position{row, col});
        }
        out << '\n';
    }
}

}
