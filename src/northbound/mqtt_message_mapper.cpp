#include "gateway/northbound/mqtt_message_mapper.hpp"

#include <utility>

#include <nlohmann/json.hpp>

namespace gateway::northbound {

MqttMessageMapper::MqttMessageMapper(std::string gateway_id) : gateway_id_(std::move(gateway_id)) {
    /* TODO */
}

auto MqttMessageMapper::telemetry_topic(std::string_view device_id) const -> std::string {
    return {};
}

auto MqttMessageMapper::command_topic(std::string_view device_id) const -> std::string {
    return {};
}

auto MqttMessageMapper::command_ack_topic(std::string_view device_id) const -> std::string {
    return {};
}

auto MqttMessageMapper::map_telemetry(const gateway::core::TelemetryMessage& message) const
    -> MqttPublishMessage {
    return {};
}

auto MqttMessageMapper::map_command_ack(const gateway::core::CommandAck& ack) const
    -> MqttPublishMessage {
    return {};
}

auto MqttMessageMapper::parse_command(std::string_view payload) const
    -> std::optional<gateway::core::CommandMessage> {
    return {};
}

} // namespace gateway::northbound
