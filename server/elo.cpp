#include "server/elo.hpp"

#include <cmath>

namespace kfc::server {

namespace {

double expectedScore(int ratingBefore, int opponentRatingBefore) {
    return 1.0 / (1.0 + std::pow(10.0, (opponentRatingBefore - ratingBefore) / 400.0));
}

}  // namespace

EloUpdate computeElo(int winnerRatingBefore, int loserRatingBefore, int kFactor) {
    const double expectedWinner = expectedScore(winnerRatingBefore, loserRatingBefore);
    const int delta = static_cast<int>(std::lround(kFactor * (1.0 - expectedWinner)));
    return {winnerRatingBefore + delta, loserRatingBefore - delta};
}

}  // namespace kfc::server
