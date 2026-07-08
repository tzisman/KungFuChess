#pragma once

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <vector>

#include "BoardParser.h"
#include "PieceTypes.h"

namespace kfc::logic {

struct Position {
    int row;
    int col;
};

inline bool operator==(Position a, Position b) { return a.row == b.row && a.col == b.col; }

inline int pawnDirection(char color) { return (color == kWhiteColor) ? -1 : 1; }

// Row index a pawn's start row is inset from its own back edge of the board.
// 0 = pawns start right on the edge row; bump to 1 for a standard-chess back rank in front of them.
constexpr int kPawnStartRowInsetFromEdge = 0;

inline int pawnStartRow(char color, int boardHeight) {
    return (color == kWhiteColor) ? (boardHeight - 1 - kPawnStartRowInsetFromEdge)
                                   : kPawnStartRowInsetFromEdge;
}

inline int pawnPromotionRow(char color, int boardHeight) {
    return (color == kWhiteColor) ? 0 : boardHeight - 1;
}

inline bool isShapeLegal(PieceType piece, char color, Position from, Position to, int boardHeight) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    if (dr == 0 && dc == 0) return false;

    int adr = std::abs(dr);
    int adc = std::abs(dc);

    switch (piece) {
        case PieceType::King: return adr <= 1 && adc <= 1;
        case PieceType::Rook: return dr == 0 || dc == 0;
        case PieceType::Bishop: return adr == adc;
        case PieceType::Queen: return dr == 0 || dc == 0 || adr == adc;
        case PieceType::Knight: return (adr == 1 && adc == 2) || (adr == 2 && adc == 1);
        case PieceType::Pawn: {
            int dir = pawnDirection(color);
            if (adc == 1) return dr == dir;
            if (dc != 0) return false;
            if (dr == dir) return true;
            if (dr == 2 * dir) return from.row == pawnStartRow(color, boardHeight);
            return false;
        }
        default: return false;
    }
}

inline bool requiresClearPath(PieceType piece) {
    return piece == PieceType::Rook || piece == PieceType::Bishop || piece == PieceType::Queen ||
           piece == PieceType::Pawn;
}

class Board {
public:
    explicit Board(std::vector<Row> rows) : rows_(std::move(rows)) {}

    int height() const { return static_cast<int>(rows_.size()); }

    int width() const { return rows_.empty() ? 0 : static_cast<int>(rows_.front().size()); }

    bool inBounds(Position p) const {
        return p.row >= 0 && p.row < height() && p.col >= 0 && p.col < width();
    }

    bool isEmpty(Position p) const { return at(p) == kEmptyCellToken; }

    bool sameColor(Position a, Position b) const { return colorAt(a) == colorAt(b); }

    PieceType pieceTypeAt(Position p) const { return *charToPieceType(at(p)[1]); }

    char colorAt(Position p) const { return at(p)[0]; }

    const std::string& tokenAt(Position p) const { return at(p); }

    bool isKing(Position p) const { return !isEmpty(p) && pieceTypeAt(p) == PieceType::King; }

    void movePiece(Position from, Position to) {
        rows_[to.row][to.col] = rows_[from.row][from.col];
        rows_[from.row][from.col] = kEmptyCellToken;
    }

    void promoteToQueen(Position p) {
        rows_[p.row][p.col] = std::string(1, colorAt(p)) + pieceTypeToChar(PieceType::Queen);
    }

private:
    const std::string& at(Position p) const { return rows_[p.row][p.col]; }

    std::vector<Row> rows_;
};

inline bool isPathClear(const Board& board, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    int stepR = (dr > 0) - (dr < 0);
    int stepC = (dc > 0) - (dc < 0);

    Position cur{from.row + stepR, from.col + stepC};
    while (cur.row != to.row || cur.col != to.col) {
        if (!board.isEmpty(cur)) return false;
        cur.row += stepR;
        cur.col += stepC;
    }
    return true;
}

constexpr long long kSquareTravelMs = 1000;

inline int chebyshevDistance(Position from, Position to) {
    return std::max(std::abs(to.row - from.row), std::abs(to.col - from.col));
}

inline long long travelDurationMs(Position from, Position to) {
    return chebyshevDistance(from, to) * kSquareTravelMs;
}

inline bool isMoveLegal(const Board& board, PieceType piece, char color, Position from, Position to) {
    if (!isShapeLegal(piece, color, from, to, board.height())) return false;
    if (requiresClearPath(piece) && !isPathClear(board, from, to)) return false;

    if (piece == PieceType::Pawn) {
        bool isDiagonal = (to.col != from.col);
        if (isDiagonal) {
            if (board.isEmpty(to)) return false;
        } else {
            if (!board.isEmpty(to)) return false;
        }
    }

    return true;
}

inline void printBoard(const Board& board, std::ostream& out) {
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            if (col > 0) out << ' ';
            out << board.tokenAt(Position{row, col});
        }
        out << '\n';
    }
}

struct PendingMove {
    Position from;
    Position to;
    long long arrivalMs;
};

class Game {
public:
    explicit Game(std::vector<Row> initialRows) : board_(std::move(initialRows)) {}

    void handleClickCell(Position p) {
        if (isOver()) return;
        if (!board_.inBounds(p)) return;

        if (!selected_.has_value()) {
            trySelect(p);
            return;
        }

        if (tryReselectSameColor(p)) return;

        tryMove(p);
    }

    void advanceClock(long long ms) {
        clockMs_ += ms;
        applyArrivedMoves();
    }

    const Board& board() const { return board_; }

    bool isOver() const { return winner_.has_value(); }

    std::optional<char> winner() const { return winner_; }

private:
    bool isPending(Position p) const {
        return pendingMove_.has_value() && pendingMove_->from == p;
    }

    bool hasPieceInTransit() const { return pendingMove_.has_value(); }

    void trySelect(Position p) {
        if (!board_.isEmpty(p) && !isPending(p)) selected_ = p;
    }

    bool tryReselectSameColor(Position p) {
        if (board_.isEmpty(p) || isPending(p) || !board_.sameColor(*selected_, p)) return false;
        selected_ = p;
        return true;
    }

    void tryMove(Position p) {
        if (hasPieceInTransit()) return;
        if (!isMoveLegal(board_, board_.pieceTypeAt(*selected_), board_.colorAt(*selected_), *selected_, p)) return;

        pendingMove_ = PendingMove{*selected_, p, clockMs_ + travelDurationMs(*selected_, p)};
        selected_.reset();
    }

    void applyArrivedMoves() {
        if (pendingMove_.has_value() && pendingMove_->arrivalMs <= clockMs_) {
            checkForKingCapture(pendingMove_->from, pendingMove_->to);
            board_.movePiece(pendingMove_->from, pendingMove_->to);
            checkForPromotion(pendingMove_->to);
            pendingMove_.reset();
        }
    }

    void checkForKingCapture(Position from, Position to) {
        if (board_.isKing(to)) {
            winner_ = board_.colorAt(from);
        }
    }

    void checkForPromotion(Position to) {
        if (board_.pieceTypeAt(to) != PieceType::Pawn) return;
        char color = board_.colorAt(to);
        if (to.row == pawnPromotionRow(color, board_.height())) {
            board_.promoteToQueen(to);
        }
    }

    Board board_;
    std::optional<Position> selected_;
    long long clockMs_ = 0;
    std::optional<PendingMove> pendingMove_;
    std::optional<char> winner_;
};

}
