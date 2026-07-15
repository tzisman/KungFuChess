#include "io/piece_config.hpp"

#include <nlohmann/json.hpp>

#include "io/parse_error.hpp"

namespace kfc::io {
namespace {

constexpr char kBadConfigJson[] = "BAD_CONFIG_JSON";
constexpr char kMissingConfigField[] = "MISSING_CONFIG_FIELD";

}  // namespace

StateConfig parseStateConfig(std::istream& in) {
    nlohmann::json root = nlohmann::json::parse(in, nullptr, false);
    if (root.is_discarded()) {
        throw ParseError{kBadConfigJson};
    }

    try {
        const nlohmann::json& physics = root.at("physics");
        const nlohmann::json& graphics = root.at("graphics");
        return StateConfig{
            physics.at("speed_m_per_sec").get<double>(),
            physics.at("next_state_when_finished").get<std::string>(),
            graphics.at("frames_per_sec").get<double>(),
            graphics.at("is_loop").get<bool>(),
        };
    } catch (const nlohmann::json::exception&) {
        throw ParseError{kMissingConfigField};
    }
}

}
