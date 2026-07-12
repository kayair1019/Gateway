#include "gateway/core/message.hpp"

#include <string>

namespace gateway::core {

auto point_quality_to_string(PointQuality quality) -> std::string_view {
    return {};
}

auto command_status_to_string(CommandStatus status) -> std::string_view {
    return {};
}

auto command_type_to_string(CommandType type) -> std::string_view {
    return {};
}

auto command_type_from_string(std::string_view value) -> std::optional<CommandType> {
    return {};
}

auto to_unix_millis(std::chrono::system_clock::time_point timestamp) -> std::int64_t {
    return {};
}

auto to_json(const TelemetryMessage& message) -> nlohmann::json {
    return {};
}

auto to_json(const CommandAck& ack) -> nlohmann::json {
    return {};
}

auto parse_command(const nlohmann::json& payload) -> std::optional<CommandMessage> {
    return {};
}

} // namespace gateway::core
