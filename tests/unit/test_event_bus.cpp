#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "bus/event_bus.hpp"

using kfc::bus::EventBus;

namespace {

struct Ping {
    int value;
};

struct Pong {
    std::string text;
};

}  // namespace

TEST_CASE("a subscriber receives the event that was published") {
    EventBus bus;
    std::vector<int> received;
    bus.subscribe<Ping>([&](const Ping& e) { received.push_back(e.value); });

    bus.publish(Ping{42});

    REQUIRE(received.size() == 1);
    CHECK(received[0] == 42);
}

TEST_CASE("every subscriber to a type hears the same event") {
    EventBus bus;
    int first = 0;
    int second = 0;
    bus.subscribe<Ping>([&](const Ping& e) { first = e.value; });
    bus.subscribe<Ping>([&](const Ping& e) { second = e.value; });

    bus.publish(Ping{7});

    CHECK(first == 7);
    CHECK(second == 7);
}

TEST_CASE("publishing with no subscribers does nothing") {
    EventBus bus;

    bus.publish(Ping{1});  // must not throw or crash

    CHECK(true);
}

TEST_CASE("a subscriber hears only the event type it asked for") {
    EventBus bus;
    int pings = 0;
    bus.subscribe<Ping>([&](const Ping&) { ++pings; });

    bus.publish(Pong{"ignored"});

    CHECK(pings == 0);
}

TEST_CASE("different event types travel on their own channels") {
    EventBus bus;
    int pingValue = 0;
    std::string pongText;
    bus.subscribe<Ping>([&](const Ping& e) { pingValue = e.value; });
    bus.subscribe<Pong>([&](const Pong& e) { pongText = e.text; });

    bus.publish(Ping{5});
    bus.publish(Pong{"hello"});

    CHECK(pingValue == 5);
    CHECK(pongText == "hello");
}
