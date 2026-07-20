#pragma once

#include "engine/game_engine.hpp"
#include "input/command_sink.hpp"
#include "model/position.hpp"

namespace kfc::input {

// Adapts the engine to the CommandSink seam for offline play. The engine stays
// unaware of the seam, so Business Logic keeps no dependency on input.
class EngineCommandSink : public CommandSink {
public:
    explicit EngineCommandSink(engine::GameEngine& engine) : engine_(engine) {}

    void requestMove(model::Position from, model::Position to) override {
        engine_.requestMove(from, to);
    }
    void requestJump(model::Position cell) override {
        engine_.requestJump(cell);
    }

private:
    engine::GameEngine& engine_;
};

}
