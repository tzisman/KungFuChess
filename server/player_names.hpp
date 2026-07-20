#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "model/piece.hpp"

namespace kfc::server {

// The display name chosen by whoever holds each colour, recorded once at join
// and read every tick when the server broadcasts state. Written from the
// network thread (ServerApp, when a session joins) and read from the game
// thread (GameSession, when it builds a snapshot); the mutex is what makes
// that safe.
class PlayerNames {
public:
    void set(model::Color color, std::string name);

    // The name registered for that colour, or nullopt if nobody has joined as
    // it yet.
    std::optional<std::string> get(model::Color color) const;

private:
    mutable std::mutex mutex_;
    std::map<model::Color, std::string> names_;
};

}
