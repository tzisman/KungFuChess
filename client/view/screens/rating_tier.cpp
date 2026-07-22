#include "view/screens/rating_tier.hpp"

#include <array>

namespace kfc::view {
namespace {

struct Belt {
    int lowestRating;
    const char* name;
};

// Read from the top down: the first belt the rating reaches. Listed once, in
// one order, so a band cannot be widened at one end without narrowing its
// neighbour at the other.
constexpr std::array<Belt, 7> kBelts{{{2000, "Master"},
                                      {1800, "Black Belt"},
                                      {1600, "Brown Belt"},
                                      {1400, "Blue Belt"},
                                      {1200, "Green Belt"},
                                      {1000, "Yellow Belt"},
                                      {0, "White Belt"}}};

}  // namespace

std::string beltFor(int rating) {
    for (const Belt& belt : kBelts) {
        if (rating >= belt.lowestRating) return belt.name;
    }
    return kBelts.back().name;
}

}
