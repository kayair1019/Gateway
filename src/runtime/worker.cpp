#include "gateway/runtime/worker.hpp"

#include <utility>

namespace gateway::runtime {

Worker::Worker(
    gateway::rule::RuleEngine rule_engine,
    gateway::northbound::MqttMessageMapper mapper,
    gateway::northbound::MqttClient& mqtt_client)
    : rule_engine_(std::move(rule_engine)), mapper_(std::move(mapper)), mqtt_client_(mqtt_client) {}

void Worker::process_once(const gateway::core::TelemetryMessage& message) {
    const auto result = rule_engine_.evaluate(message);
    if (!result.allowed) {
        return;
    }

    const auto publish_message = mapper_.map_telemetry(message);
    mqtt_client_.publish(publish_message.topic, publish_message.payload);
}

} // namespace gateway::runtime
