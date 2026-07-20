#include "io/move_notation.hpp"

#include <iomanip>
#include <sstream>

#include "io/piece_codec.hpp"

namespace kfc::io {
namespace {

constexpr char kFirstFile = 'a';
constexpr char kMoveSeparator = '-';
constexpr char kJumpMarker = 'J';

constexpr int kMsPerSecond = 1000;
constexpr int kSecondsPerMinute = 60;
constexpr char kClockPad = '0';
constexpr int kMinuteWidth = 2;
constexpr int kSecondWidth = 2;
constexpr int kMillisecondWidth = 3;
constexpr char kMinuteSeparator = ':';
constexpr char kSecondSeparator = '.';

// A pawn is named by its square alone, as chess notation has always had it.
bool isNamedBySquareAlone(model::PieceKind kind) {
    return kind == model::PieceKind::kPawn;
}

}  // namespace

// Row zero is the far rank, so ranks count back from the board's height rather
// than being fixed to any particular board size.
std::string squareName(model::Position cell, int boardHeight) {
    std::string name;
    name += static_cast<char>(kFirstFile + cell.col);
    name += std::to_string(boardHeight - cell.row);
    return name;
}

std::string notationOf(const product::MoveRecord& record, int boardHeight) {
    std::string text;
    if (record.action == engine::ActionKind::kJump) {
        text += kJumpMarker;
        text += squareName(record.from, boardHeight);
        return text;
    }

    if (!isNamedBySquareAlone(record.kind)) text += kindLetter(record.kind);
    text += squareName(record.from, boardHeight);
    text += kMoveSeparator;
    text += squareName(record.to, boardHeight);
    return text;
}

std::string clockText(int ms) {
    int seconds = ms / kMsPerSecond;
    std::ostringstream text;
    text << std::setfill(kClockPad) << std::setw(kMinuteWidth)
         << seconds / kSecondsPerMinute << kMinuteSeparator
         << std::setw(kSecondWidth) << seconds % kSecondsPerMinute
         << kSecondSeparator << std::setw(kMillisecondWidth)
         << ms % kMsPerSecond;
    return text.str();
}

}
