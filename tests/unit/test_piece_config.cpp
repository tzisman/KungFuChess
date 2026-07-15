#include <doctest/doctest.h>

#include <sstream>

#include "io/parse_error.hpp"
#include "io/piece_config.hpp"

using kfc::io::ParseError;
using kfc::io::parseStateConfig;
using kfc::io::StateConfig;

TEST_CASE("a state config carries its physics and graphics") {
    std::istringstream in{R"({
        "physics": {"speed_m_per_sec": 1.5, "next_state_when_finished": "long_rest"},
        "graphics": {"frames_per_sec": 12, "is_loop": true}
    })"};

    StateConfig config = parseStateConfig(in);

    CHECK(config.speedSquaresPerSec == doctest::Approx(1.5));
    CHECK(config.nextState == "long_rest");
    CHECK(config.framesPerSec == doctest::Approx(12));
    CHECK(config.isLoop);
}

TEST_CASE("malformed JSON is rejected") {
    std::istringstream in{"{ this is not json"};

    CHECK_THROWS_AS(parseStateConfig(in), ParseError);
}

TEST_CASE("a config missing a field is rejected") {
    std::istringstream in{R"({
        "physics": {"speed_m_per_sec": 1.5},
        "graphics": {"frames_per_sec": 12, "is_loop": true}
    })"};

    CHECK_THROWS_AS(parseStateConfig(in), ParseError);
}

TEST_CASE("a config missing a whole section is rejected") {
    std::istringstream in{R"({"graphics": {"frames_per_sec": 6, "is_loop": true}})"};

    CHECK_THROWS_AS(parseStateConfig(in), ParseError);
}
