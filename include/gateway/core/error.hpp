#pragma once

#include <string_view>

namespace gateway::core {

enum class ErrorCode {
    none,
    invalid_config,
    network_error,
    protocol_error,
    device_timeout,
    queue_full,
    mqtt_publish_failed,
    command_not_supported,
    device_not_found
};

auto to_string(ErrorCode error) -> std::string_view;

} // namespace gateway::core
