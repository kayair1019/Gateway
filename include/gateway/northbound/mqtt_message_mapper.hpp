#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "gateway/core/message.hpp"

namespace gateway::northbound {

struct MqttPublishMessage {
    std::string topic;
    std::string payload;
};

class MqttMessageMapper {
public:
    explicit MqttMessageMapper(std::string gateway_id);

    std::string telemetry_topic(std::string_view device_id) const;
    std::string command_topic(std::string_view device_id) const;
    std::string command_ack_topic(std::string_view device_id) const;

    MqttPublishMessage map_telemetry(const gateway::core::TelemetryMessage& message) const;
    MqttPublishMessage map_command_ack(const gateway::core::CommandAck& ack) const;
    std::optional<gateway::core::CommandMessage> parse_command(std::string_view payload) const;

private:
    std::string gateway_id_;
};

} // namespace gateway::northbound
