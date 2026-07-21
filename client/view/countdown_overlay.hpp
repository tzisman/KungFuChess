#pragma once

#include <string>

namespace kfc::view {

// Formats the disconnect-forfeit countdown broadcast by the server
// (protocol::CountdownTick) into the text drawn over the board. Pure string
// formatting: it knows nothing of sockets, timers, or how it gets drawn.
std::string countdownText(int secondsLeft);

}
