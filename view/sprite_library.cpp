#include "view/sprite_library.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "io/piece_codec.hpp"
#include "io/piece_config.hpp"

namespace kfc::view {
namespace {

constexpr std::array<std::pair<Animation, const char*>, 5> kAnimationDirs{{
    {Animation::kIdle, "idle"},
    {Animation::kMove, "move"},
    {Animation::kJump, "jump"},
    {Animation::kLongRest, "long_rest"},
    {Animation::kShortRest, "short_rest"},
}};

const char* dirOf(Animation animation) {
    for (const auto& entry : kAnimationDirs)
        if (entry.first == animation) return entry.second;
    return "idle";
}

// The asset folders are named after the same letters the text encoding uses,
// so the letters come from the codec instead of being spelled out again here.
std::string pieceCode(model::PieceKind kind, model::Color color) {
    std::string code;
    code += io::kindLetter(kind);
    code += static_cast<char>(
        std::toupper(static_cast<unsigned char>(io::colorLetter(color))));
    return code;
}

std::string keyOf(model::PieceKind kind, model::Color color,
                  Animation animation) {
    return pieceCode(kind, color) + '/' + dirOf(animation);
}

// Sprites are named 1.png, 2.png, ... so they are ordered numerically rather
// than lexically, which would put 10.png before 2.png.
std::vector<std::filesystem::path> spriteFilesIn(
    const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::is_directory(dir)) return files;

    for (const auto& entry : std::filesystem::directory_iterator(dir))
        if (entry.path().extension() == ".png") files.push_back(entry.path());

    std::sort(files.begin(), files.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b) {
                  return std::stoi(a.stem().string()) <
                         std::stoi(b.stem().string());
              });
    return files;
}

}  // namespace

SpriteLibrary::SpriteLibrary(const std::string& piecesRoot, int cellWidth,
                             int cellHeight) {
    for (model::PieceKind kind : model::kAllPieceKinds)
        for (model::Color color : model::kAllColors)
            loadPiece(piecesRoot, kind, color, cellWidth, cellHeight);
}

void SpriteLibrary::loadPiece(const std::string& piecesRoot,
                              model::PieceKind kind, model::Color color,
                              int cellWidth, int cellHeight) {
    for (const auto& [animation, dirName] : kAnimationDirs) {
        std::filesystem::path stateDir = std::filesystem::path(piecesRoot) /
                                         pieceCode(kind, color) / "states" /
                                         dirName;

        std::vector<Img> frames;
        for (const std::filesystem::path& file :
             spriteFilesIn(stateDir / "sprites")) {
            Img sprite;
            sprite.read(file.string(), {cellWidth, cellHeight});
            frames.push_back(std::move(sprite));
        }
        frames_.emplace(keyOf(kind, color, animation), std::move(frames));

        std::ifstream configFile{stateDir / "config.json"};
        if (!configFile) {
            throw std::runtime_error("Missing config.json in " +
                                     stateDir.string());
        }
        io::StateConfig config = io::parseStateConfig(configFile);
        specs_.emplace(keyOf(kind, color, animation),
                       AnimationSpec{config.framesPerSec, config.isLoop});
    }
}

const std::vector<Img>& SpriteLibrary::frames(model::PieceKind kind,
                                              model::Color color,
                                              Animation animation) const {
    auto it = frames_.find(keyOf(kind, color, animation));
    if (it == frames_.end())
        throw std::runtime_error("No sprites for " + keyOf(kind, color, animation));
    return it->second;
}

const AnimationSpec& SpriteLibrary::spec(model::PieceKind kind,
                                         model::Color color,
                                         Animation animation) const {
    auto it = specs_.find(keyOf(kind, color, animation));
    if (it == specs_.end())
        throw std::runtime_error("No animation spec for " +
                                 keyOf(kind, color, animation));
    return it->second;
}

}
