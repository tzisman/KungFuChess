#include "product/move_log.hpp"

namespace kfc::product {
namespace {

const std::vector<MoveRecord> kNoMoves;

}  // namespace

void MoveLog::subscribeTo(bus::EventBus& bus) {
    bus.subscribe<engine::ActionEvent>(
        [this](const engine::ActionEvent& event) { onAction(event); });
}

void MoveLog::onAction(const engine::ActionEvent& event) {
    entries_[event.color].push_back(
        {event.action, event.kind, event.from, event.to, event.atMs});
}

const std::vector<MoveRecord>& MoveLog::movesOf(model::Color color) const {
    auto found = entries_.find(color);
    return found == entries_.end() ? kNoMoves : found->second;
}

}
