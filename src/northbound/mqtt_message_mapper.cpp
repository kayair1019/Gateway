#include "gateway/northbound/mqtt_message_mapper.hpp"

#include <utility>

#include <nlohmann/json.hpp>

namespace gateway::northbound {

MqttMessageMapper::MqttMessageMapper(std::string gateway_id) : gateway_id_(std::move(gateway_id)) {}

auto MqttMessageMapper::telemetry_topic(std::string_view device_id) const -> std::string {
    return "edge/" + gateway_id_ + "/telemetry/" + std::string(device_id);
}

auto MqttMessageMapper::command_topic(std::string_view device_id) const -> std::string {
    return "edge/" + gateway_id_ + "/command/" + std::string(device_id);
}

auto MqttMessageMapper::command_ack_topic(std::string_view device_id) const -> std::string {
    return "edge/" + gateway_id_ + "/command_ack/" + std::string(device_id);
}

auto MqttMessageMapper::map_telemetry(const gateway::core::TelemetryMessage& message) const
    -> MqttPublishMessage {
    return {
        .topic = telemetry_topic(message.device_id),
        .payload = gateway::core::to_json(message).dump(),
    };
}

auto MqttMessageMapper::map_command_ack(const gateway::core::CommandAck& ack) const
    -> MqttPublishMessage {
    return {
        .topic = command_ack_topic(ack.device_id),
        .payload = gateway::core::to_json(ack).dump(),
    };
}

auto MqttMessageMapper::parse_command(std::string_view payload) const
    -> std::optional<gateway::core::CommandMessage> {
    try {
        return gateway::core::parse_command(nlohmann::json::parse(payload.begin(), payload.end()));
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

} // namespace gateway::northbound
