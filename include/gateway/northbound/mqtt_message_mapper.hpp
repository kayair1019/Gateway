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

    auto telemetry_topic(std::string_view device_id) const -> std::string;
    auto command_topic(std::string_view device_id) const -> std::string;
    auto command_ack_topic(std::string_view device_id) const -> std::string;

    auto map_telemetry(const gateway::core::TelemetryMessage& message) const
        -> MqttPublishMessage;
    auto map_command_ack(const gateway::core::CommandAck& ack) const -> MqttPublishMessage;
    auto parse_command(std::string_view payload) const
        -> std::optional<gateway::core::CommandMessage>;

private:
    std::string gateway_id_;
};

} // namespace gateway::northbound
