#include "gateway/runtime/gateway_app.hpp"

#include <iostream>
#include <utility>

#include "gateway/core/error.hpp"
#include "gateway/northbound/console_mqtt_client.hpp"
#include "gateway/northbound/mqtt_message_mapper.hpp"
#include "gateway/rule/rule_engine.hpp"
#include "gateway/runtime/worker.hpp"
#include "gateway/southbound/device_poller.hpp"
#include "gateway/southbound/modbus_tcp_client.hpp"

namespace gateway::runtime {

GatewayApp::GatewayApp(gateway::core::GatewayConfig config) : config_(std::move(config)) {}

int GatewayApp::run() {
    if (config_.devices.empty()) {
        std::cerr << "edge_gateway failed: no device configured\n";
        return 1;
    }

    gateway::northbound::ConsoleMqttClient mqtt_client;
    gateway::northbound::MqttMessageMapper mapper(config_.gateway_id);
    gateway::rule::RuleEngine rule_engine;
    Worker worker(std::move(rule_engine), std::move(mapper), mqtt_client);

    gateway::southbound::ModbusTcpClient modbus_client(io_context_);
    gateway::southbound::DevicePoller poller(config_.devices.front(), modbus_client);

    const auto result = poller.poll_once();
    if (!result.ok || !result.message.has_value()) {
        std::cerr << "edge_gateway poll failed: " << gateway::core::to_string(result.error) << '\n';
        return 1;
    }

    worker.process_once(*result.message);
    return 0;
}

} // namespace gateway::runtime
