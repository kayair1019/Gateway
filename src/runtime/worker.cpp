#include "gateway/runtime/worker.hpp"

#include <utility>

namespace gateway::runtime {

Worker::Worker(
    gateway::rule::RuleEngine rule_engine,
    gateway::northbound::MqttMessageMapper mapper,
    gateway::northbound::MqttClient& mqtt_client)
    : rule_engine_(std::move(rule_engine)), mapper_(std::move(mapper)), mqtt_client_(mqtt_client) {
    /* TODO */
}

void Worker::process_once(const gateway::core::TelemetryMessage& message) {
    /* TODO: 实现该函数 */
}

} // namespace gateway::runtime
