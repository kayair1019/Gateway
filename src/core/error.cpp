#include "gateway/core/error.hpp"

namespace gateway::core {

std::string_view to_string(ErrorCode error) {
    switch (error) {
    case ErrorCode::none:
        return "none";
    case ErrorCode::invalid_config:
        return "invalid_config";
    case ErrorCode::network_error:
        return "network_error";
    case ErrorCode::protocol_error:
        return "protocol_error";
    case ErrorCode::device_timeout:
        return "device_timeout";
    case ErrorCode::queue_full:
        return "queue_full";
    case ErrorCode::mqtt_publish_failed:
        return "mqtt_publish_failed";
    case ErrorCode::command_not_supported:
        return "command_not_supported";
    case ErrorCode::device_not_found:
        return "device_not_found";
    }

    return "unknown";
}

} // namespace gateway::core
