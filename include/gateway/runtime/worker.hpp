#pragma once

#include "gateway/core/message.hpp"
#include "gateway/northbound/mqtt_client.hpp"
#include "gateway/northbound/mqtt_message_mapper.hpp"
#include "gateway/rule/rule_engine.hpp"

namespace gateway::runtime {

class Worker {
public:
    Worker(
        gateway::rule::RuleEngine rule_engine,
        gateway::northbound::MqttMessageMapper mapper,
        gateway::northbound::MqttClient& mqtt_client);

    void process_once(const gateway::core::TelemetryMessage& message);

    template <typename TelemetryQueue>
    bool process_one_from_queue(TelemetryQueue& queue) {
        gateway::core::TelemetryMessage message;
        if (!queue.pop(message)) {
            return false;
        }

        process_once(message);
        return true;
    }

private:
    gateway::rule::RuleEngine rule_engine_;
    gateway::northbound::MqttMessageMapper mapper_;
    gateway::northbound::MqttClient& mqtt_client_;
};

} // namespace gateway::runtime
