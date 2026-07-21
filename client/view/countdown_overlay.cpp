#include "view/countdown_overlay.hpp"

namespace kfc::view {
namespace {
constexpr char kCountdownLabel[] = "COUNTDOWN: ";
}

std::string countdownText(int secondsLeft) {
    return std::string(kCountdownLabel) + std::to_string(secondsLeft);
}

}
