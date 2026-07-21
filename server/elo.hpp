#pragma once

namespace kfc::server {

inline constexpr int kEloKFactor = 32;

struct EloUpdate {
    int winnerRating;
    int loserRating;
};

EloUpdate computeElo(int winnerRatingBefore, int loserRatingBefore, int kFactor = kEloKFactor);

}  // namespace kfc::server
