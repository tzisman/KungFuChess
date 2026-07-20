#include "protocol/position_codec.hpp"

namespace kfc::protocol {
namespace {

constexpr char kRowKey[] = "row";
constexpr char kColKey[] = "col";

}  // namespace

nlohmann::json encodePosition(model::Position cell) {
    return {{kRowKey, cell.row}, {kColKey, cell.col}};
}

std::optional<model::Position> decodePosition(const nlohmann::json& value) {
    if (!value.is_object()) return std::nullopt;
    if (!value.contains(kRowKey) || !value[kRowKey].is_number_integer())
        return std::nullopt;
    if (!value.contains(kColKey) || !value[kColKey].is_number_integer())
        return std::nullopt;
    return model::Position{value[kRowKey].get<int>(), value[kColKey].get<int>()};
}

}
