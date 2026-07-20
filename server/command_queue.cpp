#include "server/command_queue.hpp"

#include <utility>

namespace kfc::server {

void CommandQueue::push(PlayerCommand command) {
    std::lock_guard<std::mutex> lock{mutex_};
    pending_.push_back(std::move(command));
}

std::vector<PlayerCommand> CommandQueue::drain() {
    std::lock_guard<std::mutex> lock{mutex_};
    std::vector<PlayerCommand> drained = std::move(pending_);
    pending_.clear();
    return drained;
}

}
