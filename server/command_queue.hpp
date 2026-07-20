#pragma once

#include <mutex>
#include <variant>
#include <vector>

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::server {

// One player's move request, already tagged with the colour of the session
// that sent it. ServerApp — the only place that knows the connection-to-colour
// mapping — resolves that colour once, when the intent arrives; GameSession
// only has to compare it against the board.
struct MoveCommand {
    model::Color color;
    model::Position from;
    model::Position to;
};

struct JumpCommand {
    model::Color color;
    model::Position cell;
};

using PlayerCommand = std::variant<MoveCommand, JumpCommand>;

// Marshals player commands from the network thread, which decodes them as they
// arrive, to the single game thread, which is the only thing allowed to touch
// the engine. A mutex-guarded vector is enough: pushes are rare compared to
// ticks.
class CommandQueue {
public:
    void push(PlayerCommand command);

    // Hands over every command queued since the last call, leaving none behind.
    std::vector<PlayerCommand> drain();

private:
    std::mutex mutex_;
    std::vector<PlayerCommand> pending_;
};

}
