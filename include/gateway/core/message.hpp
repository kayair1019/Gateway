#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "gateway/core/error.hpp"

namespace gateway::core {

enum class PointQuality {
    good,
    bad
};

enum class CommandType {
    read_register,
    write_register
};

enum class CommandStatus {
    success,
    failed
};

struct PointValue {
    std::string name;
    std::uint16_t address{0};
    double value{0.0};
    PointQuality quality{PointQuality::good};
};

struct TelemetryMessage {
    std::string device_id;
    std::string protocol{"modbus_tcp"};
    std::chrono::system_clock::time_point timestamp{};
    std::vector<PointValue> points;
};

struct CommandMessage {
    std::string command_id;
    std::string device_id;
    CommandType type{CommandType::read_register};
    std::uint16_t address{0};
    std::uint16_t value{0};
    std::uint16_t count{1};
};

struct CommandAck {
    std::string command_id;
    std::string device_id;
    CommandStatus status{CommandStatus::success};
    ErrorCode error{ErrorCode::none};
    std::string message;
    std::chrono::system_clock::time_point timestamp{};
};

auto point_quality_to_string(PointQuality quality) -> std::string_view;
auto command_status_to_string(CommandStatus status) -> std::string_view;
auto command_type_to_string(CommandType type) -> std::string_view;
auto command_type_from_string(std::string_view value) -> std::optional<CommandType>;

auto to_json(const TelemetryMessage& message) -> nlohmann::json;
auto to_json(const CommandAck& ack) -> nlohmann::json;
auto parse_command(const nlohmann::json& payload) -> std::optional<CommandMessage>;
auto to_unix_millis(std::chrono::system_clock::time_point timestamp) -> std::int64_t;

} // namespace gateway::core
