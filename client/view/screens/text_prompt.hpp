#pragma once

#include <string>

#include "img.hpp"

namespace kfc::view {

// Draws the "enter room name" screen: a prompt label above a box holding
// whatever has been typed so far. Display only — client/input/text_prompt_
// buffer.hpp owns the actual keystroke accumulation.
class TextPromptRenderer {
public:
    TextPromptRenderer(int canvasWidth, int canvasHeight);

    Img render(const std::string& prompt, const std::string& typedSoFar) const;

private:
    int canvasWidth_;
    int canvasHeight_;
};

}
