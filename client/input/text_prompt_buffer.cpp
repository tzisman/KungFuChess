#include "input/text_prompt_buffer.hpp"

namespace kfc::input {

bool TextPromptBuffer::handleKey(int key) {
    if (key == kEnterKey) return true;
    if (key == kBackspaceKey) {
        if (!text_.empty()) text_.pop_back();
        return false;
    }
    if (key >= kMinPrintableAscii && key <= kMaxPrintableAscii) {
        text_.push_back(static_cast<char>(key));
    }
    return false;
}

}
