#pragma once

#include <map>
#include <string>
#include <vector>

#include "img.hpp"
#include "model/piece.hpp"

namespace kfc::view {

// The animations a piece can be drawn with. This is a display concept: it
// names a folder of sprites, not a rule of the game.
enum class Animation { kIdle, kMove, kJump, kLongRest, kShortRest };

// How an animation plays back, as declared in its config.json.
struct AnimationSpec {
    double framesPerSec;
    bool isLoop;
};

// Every piece sprite and its playback spec, loaded once with the sprites
// already scaled to the cell size, so that drawing a frame never touches the
// disk. Built for a specific cell size; construct a new library if the board
// is resized.
class SpriteLibrary {
public:
    SpriteLibrary(const std::string& piecesRoot, int cellWidth, int cellHeight);

    const std::vector<Img>& frames(model::PieceKind kind, model::Color color,
                                   Animation animation) const;
    const AnimationSpec& spec(model::PieceKind kind, model::Color color,
                              Animation animation) const;

private:
    void loadPiece(const std::string& piecesRoot, model::PieceKind kind,
                   model::Color color, int cellWidth, int cellHeight);

    std::map<std::string, std::vector<Img>> frames_;
    std::map<std::string, AnimationSpec> specs_;
};

}
