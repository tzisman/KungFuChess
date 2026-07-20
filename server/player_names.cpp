#include "server/player_names.hpp"

#include <utility>

namespace kfc::server {

void PlayerNames::set(model::Color color, std::string name) {
    std::lock_guard<std::mutex> lock{mutex_};
    names_[color] = std::move(name);
}

std::optional<std::string> PlayerNames::get(model::Color color) const {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = names_.find(color);
    if (it == names_.end()) return std::nullopt;
    return it->second;
}

}
