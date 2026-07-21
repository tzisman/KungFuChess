#include <doctest/doctest.h>

#include "server/elo.hpp"

using kfc::server::computeElo;
using kfc::server::kEloKFactor;

TEST_CASE("equal ratings move symmetrically by half the K-factor") {
    const auto update = computeElo(1000, 1000);
    CHECK(update.winnerRating == 1000 + kEloKFactor / 2);
    CHECK(update.loserRating == 1000 - kEloKFactor / 2);
}

TEST_CASE("a big rating gap moves the weaker winner more than the stronger one would gain") {
    const auto expectedUpset = computeElo(1000, 1400);
    const auto favoriteWin = computeElo(1400, 1000);
    CHECK(expectedUpset.winnerRating - 1000 > favoriteWin.winnerRating - 1400);
}

TEST_CASE("the update is zero-sum: the winner's gain equals the loser's loss") {
    const auto update = computeElo(1123, 987);
    const int winnerGain = update.winnerRating - 1123;
    const int loserLoss = 987 - update.loserRating;
    CHECK(winnerGain == loserLoss);
}

TEST_CASE("a K-factor of zero is a no-op") {
    const auto update = computeElo(1500, 1600, 0);
    CHECK(update.winnerRating == 1500);
    CHECK(update.loserRating == 1600);
}
