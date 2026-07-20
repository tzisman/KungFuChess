#pragma once

#include <map>
#include <optional>
#include <vector>

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

// One single-cell hop of a route: the cell it lands on and how long it takes.
struct Step {
    model::Position cell;
    int durationMs;
};

// Decomposes a move from `from` to `to` into the hops a piece travels. A sliding
// move becomes one hop per cell; a leap (e.g. a knight) stays a single hop that
// keeps its full travel time.
std::vector<Step> buildRoute(model::Position from, model::Position to,
                             int squareTravelMs);

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


// A piece travelling a route one hop at a time. It occupies its current cell for
// the whole of each hop, so a piece caught mid-slide is found on the cell it is
// leaving, not the one it is heading toward.
class Motion {
public:
    Motion(model::PieceId pieceId, model::Position origin,
           std::vector<Step> steps);

    model::PieceId pieceId() const { return pieceId_; }
    model::Position currentCell() const { return currentCell_; }
    model::Position nextCell() const { return steps_[index_].cell; }
    model::Position destination() const { return steps_.back().cell; }
    int durationMs() const { return steps_[index_].durationMs; }
    int elapsedMs() const { return elapsedMs_; }
    double hopProgress() const;

    void advance(int deltaMs);
    bool hopArrived() const { return elapsedMs_ >= durationMs(); }

    // Completes the current hop: the piece now stands on the hop's cell. Returns
    // false when that was the last hop (the route is done), true when another hop
    // remains, carrying any overshoot into it.
    bool advanceToNextHop();

private:
    model::PieceId pieceId_;
    model::Position currentCell_;
    std::vector<Step> steps_;
    std::size_t index_ = 0;
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
