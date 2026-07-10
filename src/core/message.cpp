#include "gateway/core/message.hpp"

#include <string>

namespace gateway::core {

auto point_quality_to_string(PointQuality quality) -> std::string_view {
    switch (quality) {
    case PointQuality::good:
        return "good";
    case PointQuality::bad:
        return "bad";
    }

    return "bad";
}

auto command_status_to_string(CommandStatus status) -> std::string_view {
    switch (status) {
    case CommandStatus::success:
        return "success";
    case CommandStatus::failed:
        return "failed";
    }

    return "failed";
}

auto command_type_to_string(CommandType type) -> std::string_view {
    switch (type) {
    case CommandType::read_register:
        return "read_register";
    case CommandType::write_register:
        return "write_register";
    }

    return "read_register";
}

auto command_type_from_string(std::string_view value) -> std::optional<CommandType> {
    if (value == "read_register") {
        return CommandType::read_register;
    }

    if (value == "write_register") {
        return CommandType::write_register;
    }

    return std::nullopt;
}

auto to_unix_millis(std::chrono::system_clock::time_point timestamp) -> std::int64_t {
    const auto duration = timestamp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

auto to_json(const TelemetryMessage& message) -> nlohmann::json {
    nlohmann::json points = nlohmann::json::array();
    for (const auto& point : message.points) {
        points.push_back({
            {"name", point.name},
            {"address", point.address},
            {"value", point.value},
            {"quality", std::string(point_quality_to_string(point.quality))},
        });
    }

    return {
        {"device_id", message.device_id},
        {"protocol", message.protocol},
        {"timestamp_ms", to_unix_millis(message.timestamp)},
        {"points", points},
    };
}

auto to_json(const CommandAck& ack) -> nlohmann::json {
    nlohmann::json payload{
        {"command_id", ack.command_id},
        {"device_id", ack.device_id},
        {"status", std::string(command_status_to_string(ack.status))},
        {"message", ack.message},
        {"timestamp_ms", to_unix_millis(ack.timestamp)},
    };

    if (ack.status == CommandStatus::failed) {
        payload["error_code"] = std::string(to_string(ack.error));
    }

    return payload;
}

auto parse_command(const nlohmann::json& payload) -> std::optional<CommandMessage> {
    if (!payload.contains("command_id") || !payload.contains("device_id") ||
        !payload.contains("type") || !payload.contains("address")) {
        return std::nullopt;
    }

    const auto type = command_type_from_string(payload.value("type", ""));
    if (!type.has_value()) {
        return std::nullopt;
    }

    CommandMessage command;
    command.command_id = payload.value("command_id", "");
    command.device_id = payload.value("device_id", "");
    command.type = *type;
    command.address = payload.value("address", 0);

    if (command.type == CommandType::write_register) {
        if (!payload.contains("value")) {
            return std::nullopt;
        }
        command.value = payload.value("value", 0);
    }

    if (command.type == CommandType::read_register) {
        command.count = payload.value("count", 1);
    }

    return command;
}

} // namespace gateway::core
