#include "gateway/northbound/mqtt_message_mapper.hpp"

#include <utility>

#include <nlohmann/json.hpp>

namespace gateway::northbound {

MqttMessageMapper::MqttMessageMapper(std::string gateway_id) : gateway_id_(std::move(gateway_id)) {}

std::string MqttMessageMapper::telemetry_topic(std::string_view device_id) const {
    return "edge/" + gateway_id_ + "/telemetry/" + std::string(device_id);
}

std::string MqttMessageMapper::command_topic(std::string_view device_id) const {
    return "edge/" + gateway_id_ + "/command/" + std::string(device_id);
}

std::string MqttMessageMapper::command_ack_topic(std::string_view device_id) const {
    return "edge/" + gateway_id_ + "/command_ack/" + std::string(device_id);
}

MqttPublishMessage MqttMessageMapper::map_telemetry(const gateway::core::TelemetryMessage& message) const {
    return {
        .topic = telemetry_topic(message.device_id),
        .payload = gateway::core::to_json(message).dump(),
    };
}

MqttPublishMessage MqttMessageMapper::map_command_ack(const gateway::core::CommandAck& ack) const {
    return {
        .topic = command_ack_topic(ack.device_id),
        .payload = gateway::core::to_json(ack).dump(),
    };
}

std::optional<gateway::core::CommandMessage> MqttMessageMapper::parse_command(std::string_view payload) const {
    try {
        return gateway::core::parse_command(nlohmann::json::parse(payload.begin(), payload.end()));
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

} // namespace gateway::northbound
