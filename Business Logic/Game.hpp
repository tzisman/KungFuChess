#pragma once

#include <optional>
#include <vector>

#include "Board.hpp"

namespace kfc::logic {

struct PendingMove {
    Position from;
    Position to;
    long long arrivalMs;
};

struct PendingJump {
    Position cell;
    char color;
    long long expiryMs;
};

class Game {
public:
    explicit Game(std::vector<Row> initialRows);

    void handleClickCell(Position p);
    void handleJumpCommand(Position p);
    void advanceClock(long long ms);

    const Board& board() const;
    bool isOver() const;
    std::optional<char> winner() const;

private:
    bool isPending(Position p) const;
    bool hasPieceInTransit() const;
    bool isJumping(Position p) const;
    bool canJump(Position p) const;
    bool isEnemyJumpAt(Position p, char movingColor) const;

    void trySelect(Position p);
    bool tryReselectSameColor(Position p);
    void tryMove(Position p);

    void applyArrivedMoves();
    void resolveArrivingMove(Position from, Position to);
    void checkForKingCapture(Position from, Position to);
    void checkForPromotion(Position to);

    Board board_;
    std::optional<Position> selected_;
    long long clockMs_ = 0;
    std::optional<PendingMove> pendingMove_;
    std::optional<PendingJump> pendingJump_;
    std::optional<char> winner_;
};

}
