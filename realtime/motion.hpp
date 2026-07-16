#pragma once

#include <map>
#include <optional>

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::realtime {

constexpr int kSquareTravelMs = 1000;
constexpr int kJumpDurationMs = 1000;
constexpr int kShortRestMs = 3000;
constexpr int kLongRestMs = 6000;

// A jump stays in the air for this multiple of the time its configured speed
// implies, which makes leaving the square a real commitment.
constexpr int kJumpDurationFactor = 4;

int travelDurationMs(model::Position from, model::Position to,
                     int squareTravelMs = kSquareTravelMs);

// How fast each kind of piece travels and jumps. Kinds that were never given
// a timing fall back to the fixed defaults, so a game assembled without any
// configuration behaves exactly as before.
class MotionProfiles {
public:
    void setTiming(model::PieceKind kind, int squareTravelMs,
                   int jumpDurationMs);
    int squareTravelMs(model::PieceKind kind) const;
    int jumpDurationMs(model::PieceKind kind) const;

private:
    struct Timing {
        int squareTravelMs;
        int jumpDurationMs;
    };

    std::map<model::PieceKind, Timing> timings_;
};


class Motion {
public:
    Motion(model::PieceId pieceId, model::Position from, model::Position to,
           int durationMs);

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
    explicit Jump(model::Position cell, int durationMs = kJumpDurationMs);

    model::Position cell() const { return cell_; }
    int durationMs() const { return durationMs_; }
    int elapsedMs() const { return elapsedMs_; }

    void advance(int deltaMs);
    bool hasLanded() const { return elapsedMs_ >= durationMs_; }

    bool isLifted() const { return lifted_.has_value(); }
    void lift(const model::Piece& piece) { lifted_ = piece; }
    const model::Piece& lifted() const { return *lifted_; }

private:
    model::Position cell_;
    int durationMs_;
    int elapsedMs_ = 0;
    std::optional<model::Piece> lifted_;
};


class Cooldown {
public:
    Cooldown(model::PieceId pieceId, model::Position cell, int durationMs);

    model::PieceId pieceId() const { return pieceId_; }
    model::Position cell() const { return cell_; }
    int durationMs() const { return durationMs_; }
    int elapsedMs() const { return elapsedMs_; }

    void advance(int deltaMs);
    bool hasElapsed() const { return elapsedMs_ >= durationMs_; }

private:
    model::PieceId pieceId_;
    model::Position cell_;
    int durationMs_;
    int elapsedMs_ = 0;
};

}
