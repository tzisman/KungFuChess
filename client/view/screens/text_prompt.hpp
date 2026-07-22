#pragma once

#include <string>

#include "img.hpp"
#include "view/screens/screen_layout.hpp"

namespace kfc::view {

// Draws the "enter room name" screen: a prompt label above a field holding
// whatever has been typed so far. Display only — client/input/text_prompt_
// buffer.hpp owns the actual keystroke accumulation.
class TextPromptRenderer {
public:
    TextPromptRenderer(int canvasWidth, int canvasHeight);

    Img render(const std::string& prompt, const std::string& typedSoFar) const;

private:
    void drawField(Img& canvas, const std::string& typedSoFar) const;

    PromptLayout layout_;
};

}
