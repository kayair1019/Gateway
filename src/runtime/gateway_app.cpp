#include "gateway/runtime/gateway_app.hpp"

#include <iostream>
#include <thread>
#include <utility>

#include "gateway/core/error.hpp"
#include "gateway/northbound/console_mqtt_client.hpp"
#include "gateway/northbound/mqtt_message_mapper.hpp"
#include "gateway/queue/spsc_ring_buffer.hpp"
#include "gateway/rule/rule_engine.hpp"
#include "gateway/runtime/command_dispatcher.hpp"
#include "gateway/runtime/worker.hpp"
#include "gateway/southbound/device_poller.hpp"
#include "gateway/southbound/modbus_tcp_client.hpp"

namespace gateway::runtime {
namespace {

constexpr std::size_t kTelemetryQueueCapacity = 1024;

} // namespace

GatewayApp::GatewayApp(gateway::core::GatewayConfig config) : config_(std::move(config)) {}

int GatewayApp::run() {
    if (config_.devices.empty()) {
        std::cerr << "edge_gateway failed: no device configured\n";
        return 1;
    }

    gateway::northbound::ConsoleMqttClient mqtt_client;
    gateway::northbound::MqttMessageMapper mapper(config_.gateway_id);
    gateway::rule::RuleEngine rule_engine(config_.rules);
    Worker worker(std::move(rule_engine), mapper, mqtt_client);

    gateway::southbound::ModbusTcpClient modbus_client(io_context_);
    gateway::southbound::DevicePoller poller(config_.devices.front(), modbus_client);

    const auto result = poller.poll_once();
    if (!result.ok || !result.message.has_value()) {
        std::cerr << "edge_gateway poll failed: " << gateway::core::to_string(result.error) << '\n';
        return 1;
    }

    gateway::queue::SpscRingBuffer<gateway::core::TelemetryMessage, kTelemetryQueueCapacity>
        telemetry_queue;
    if (!telemetry_queue.push(*result.message)) {
        std::cerr << "edge_gateway queue failed: "
                  << gateway::core::to_string(gateway::core::ErrorCode::queue_full) << '\n';
        return 1;
    }

    std::thread worker_thread([&worker, &telemetry_queue] {
        worker.process_one_from_queue(telemetry_queue);
    });
    worker_thread.join();

    CommandDispatcher command_dispatcher(config_, modbus_client);
    gateway::core::CommandMessage command;
    command.command_id = "demo-cmd-001";
    command.device_id = config_.devices.front().device_id;
    command.type = gateway::core::CommandType::write_register;
    command.address = 1;
    command.value = 4321;

    const auto ack = command_dispatcher.dispatch(command);
    const auto ack_message = mapper.map_command_ack(ack);
    mqtt_client.publish(ack_message.topic, ack_message.payload);

    return 0;
}

} // namespace gateway::runtime
