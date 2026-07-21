#pragma once

#include <string>

namespace kfc::input {

inline constexpr int kBackspaceKey = 8;
inline constexpr int kEnterKey = 13;
inline constexpr int kMinPrintableAscii = 0x20;
inline constexpr int kMaxPrintableAscii = 0x7E;

// Accumulates typed characters into a short line of text (a room name),
// driven by raw key codes as returned by view::Window::waitKey. Pure: knows
// nothing of OpenCV, or where the result ends up drawn.
class TextPromptBuffer {
public:
    // Feeds one key code. Returns true once Enter was pressed (text() then
    // holds the finished value); false for every ordinary keystroke,
    // backspace, or unrecognised key.
    bool handleKey(int key);

    const std::string& text() const { return text_; }
    void clear() { text_.clear(); }

private:
    std::string text_;
};

}
