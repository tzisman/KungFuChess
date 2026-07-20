#pragma once

#include <istream>
#include <string>

namespace kfc::io {

// The contents of one state's config.json, as written: physics for the game
// clock and graphics for the animation. This is raw data only; mapping it onto
// realtime or view types happens where the game is assembled.
struct StateConfig {
    double speedSquaresPerSec;
    std::string nextState;
    double framesPerSec;
    bool isLoop;
};

StateConfig parseStateConfig(std::istream& in);

}
