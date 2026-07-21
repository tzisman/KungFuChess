#include <doctest/doctest.h>

#include "input/text_prompt_buffer.hpp"

using kfc::input::kBackspaceKey;
using kfc::input::kEnterKey;
using kfc::input::TextPromptBuffer;

TEST_CASE("typing printable characters accumulates them in order") {
    TextPromptBuffer buffer;

    CHECK_FALSE(buffer.handleKey('l'));
    CHECK_FALSE(buffer.handleKey('o'));
    CHECK_FALSE(buffer.handleKey('u'));
    CHECK_FALSE(buffer.handleKey('n'));
    CHECK_FALSE(buffer.handleKey('g'));
    CHECK_FALSE(buffer.handleKey('e'));

    CHECK(buffer.text() == "lounge");
}

TEST_CASE("backspace removes the last character") {
    TextPromptBuffer buffer;
    buffer.handleKey('h');
    buffer.handleKey('i');

    CHECK_FALSE(buffer.handleKey(kBackspaceKey));

    CHECK(buffer.text() == "h");
}

TEST_CASE("backspace on empty text is a no-op") {
    TextPromptBuffer buffer;

    CHECK_FALSE(buffer.handleKey(kBackspaceKey));

    CHECK(buffer.text().empty());
}

TEST_CASE("enter reports done without altering the accumulated text") {
    TextPromptBuffer buffer;
    buffer.handleKey('h');
    buffer.handleKey('i');

    CHECK(buffer.handleKey(kEnterKey));

    CHECK(buffer.text() == "hi");
}

TEST_CASE("a key outside the printable ASCII range is ignored") {
    TextPromptBuffer buffer;

    CHECK_FALSE(buffer.handleKey(1));
    CHECK_FALSE(buffer.handleKey(200));

    CHECK(buffer.text().empty());
}

TEST_CASE("clear empties the buffer") {
    TextPromptBuffer buffer;
    buffer.handleKey('x');

    buffer.clear();

    CHECK(buffer.text().empty());
}
