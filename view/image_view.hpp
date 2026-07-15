#pragma once

#include <string>

namespace kfc::view {

// Graphical view. Displays board/piece images through the Img backend and
// holds no game rules. Its only outward dependency is the image tool
// (images/img.hpp), never the engine, model, or rules layers.
class ImageView {
public:
    explicit ImageView(std::string boardImagePath);

    void show();

private:
    std::string boardImagePath_;
};

}
