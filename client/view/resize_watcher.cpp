#include "view/resize_watcher.hpp"

namespace kfc::view {

constexpr int kSettleMs = 250;

ResizeWatcher::ResizeWatcher(WindowSize initial)
    : pending_(initial), settled_(initial), msPendingHeld_(0) {}

std::optional<WindowSize> ResizeWatcher::poll(WindowSize current,
                                              int elapsedMs) {
    if (!(current == pending_)) {
        pending_ = current;
        msPendingHeld_ = 0;
        return std::nullopt;
    }
    if (pending_ == settled_) return std::nullopt;

    msPendingHeld_ += elapsedMs;
    if (msPendingHeld_ < kSettleMs) return std::nullopt;

    settled_ = pending_;
    return settled_;
}

void ResizeWatcher::reset(WindowSize size) {
    pending_ = size;
    settled_ = size;
    msPendingHeld_ = 0;
}

}
