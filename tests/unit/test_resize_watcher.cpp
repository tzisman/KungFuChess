#include <doctest/doctest.h>

#include "view/resize_watcher.hpp"

using kfc::view::ResizeWatcher;
using kfc::view::WindowSize;

TEST_CASE("a size that never changes is never reported") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    CHECK_FALSE(watcher.poll(WindowSize{800, 600}, 1000).has_value());
}

TEST_CASE("a changed size is not reported before it has held long enough") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    CHECK_FALSE(watcher.poll(WindowSize{900, 700}, 100).has_value());
    CHECK_FALSE(watcher.poll(WindowSize{900, 700}, 100).has_value());
}

TEST_CASE("a changed size is reported once it has held long enough") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    watcher.poll(WindowSize{900, 700}, 100);
    watcher.poll(WindowSize{900, 700}, 100);
    std::optional<WindowSize> settled = watcher.poll(WindowSize{900, 700}, 200);

    REQUIRE(settled.has_value());
    CHECK(settled->width == 900);
    CHECK(settled->height == 700);
}

TEST_CASE("a settled size is reported only once") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    watcher.poll(WindowSize{900, 700}, 100);
    watcher.poll(WindowSize{900, 700}, 100);
    watcher.poll(WindowSize{900, 700}, 200);

    CHECK_FALSE(watcher.poll(WindowSize{900, 700}, 1000).has_value());
}

TEST_CASE("a still-changing size restarts the settle timer") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    watcher.poll(WindowSize{900, 700}, 100);
    watcher.poll(WindowSize{950, 720}, 100);
    CHECK_FALSE(watcher.poll(WindowSize{950, 720}, 200).has_value());

    std::optional<WindowSize> settled = watcher.poll(WindowSize{950, 720}, 100);

    REQUIRE(settled.has_value());
    CHECK(settled->width == 950);
    CHECK(settled->height == 720);
}

TEST_CASE("reset adopts a size as already settled, without reporting it") {
    ResizeWatcher watcher{WindowSize{800, 600}};

    watcher.reset(WindowSize{900, 700});

    CHECK_FALSE(watcher.poll(WindowSize{900, 700}, 1000).has_value());
}
