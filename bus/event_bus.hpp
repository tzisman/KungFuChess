#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kfc::bus {

// A generic publish/subscribe bus. Publishers announce events by value and
// subscribers ask for exactly the event types they care about; neither side
// knows the other. The bus is deliberately ignorant of any concrete event
// type, so it depends on no game layer and any layer may depend on it.
class EventBus {
public:
    template <typename Event>
    void subscribe(std::function<void(const Event&)> handler) {
        channels_[std::type_index(typeid(Event))].push_back(
            [handler = std::move(handler)](const void* event) {
                handler(*static_cast<const Event*>(event));
            });
    }

    template <typename Event>
    void publish(const Event& event) const {
        auto found = channels_.find(std::type_index(typeid(Event)));
        if (found == channels_.end()) return;
        for (const ErasedHandler& handler : found->second) handler(&event);
    }

private:
    using ErasedHandler = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<ErasedHandler>> channels_;
};

}
