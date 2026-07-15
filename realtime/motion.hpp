#pragma once

#include <optional>

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::realtime {

constexpr int kSquareTravelMs = 1000;
constexpr int kJumpDurationMs = 1000;
constexpr int kCooldownMs = 1000;

int travelDurationMs(model::Position from, model::Position to);


class Motion {
public:
    Motion(model::PieceId pieceId, model::Position from, model::Position to);

    model::PieceId pieceId() const { return pieceId_; }
    model::Position from() const { return from_; }
    model::Position to() const { return to_; }
    int durationMs() const { return durationMs_; }
    int elapsedMs() const { return elapsedMs_; }

    void advance(int deltaMs);
    bool hasArrived() const { return elapsedMs_ >= durationMs_; }

private:
    model::PieceId pieceId_;
    model::Position from_;
    model::Position to_;
    int durationMs_;
    int elapsedMs_ = 0;
};


class Jump {
public:
    explicit Jump(model::Position cell);

    model::Position cell() const { return cell_; }

    void advance(int deltaMs);
    bool hasLanded() const { return elapsedMs_ >= kJumpDurationMs; }

    bool isLifted() const { return lifted_.has_value(); }
    void lift(const model::Piece& piece) { lifted_ = piece; }
    const model::Piece& lifted() const { return *lifted_; }

private:
    model::Position cell_;
    int elapsedMs_ = 0;
    std::optional<model::Piece> lifted_;
};


class Cooldown {
public:
    Cooldown(model::PieceId pieceId, model::Position cell);

    model::PieceId pieceId() const { return pieceId_; }
    model::Position cell() const { return cell_; }

    void advance(int deltaMs);
    bool hasElapsed() const { return elapsedMs_ >= kCooldownMs; }

private:
    model::PieceId pieceId_;
    model::Position cell_;
    int elapsedMs_ = 0;
};

}
