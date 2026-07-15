#include "gateway/runtime/gateway_app.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <sys/select.h>
#include <unistd.h>

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
constexpr auto kWorkerIdleSleep = std::chrono::milliseconds(10);
constexpr auto kStopCheckInterval = std::chrono::milliseconds(50);

volatile std::sig_atomic_t g_stop_requested = 0;

void handle_stop_signal(int) {
    g_stop_requested = 1;
}

void sleep_until_next_poll_or_stop(std::chrono::milliseconds interval) {
    auto remaining = interval;
    while (remaining.count() > 0 && g_stop_requested == 0) {
        const auto step = remaining < kStopCheckInterval ? remaining : kStopCheckInterval;
        std::this_thread::sleep_for(step);
        remaining -= step;
    }
}

std::optional<std::string> try_read_stdin_line() {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    timeval timeout{};
    const auto ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready <= 0 || !FD_ISSET(STDIN_FILENO, &read_fds)) {
        return std::nullopt;
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::nullopt;
    }

    if (line.empty()) {
        return std::nullopt;
    }

    return line;
}

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
    CommandDispatcher command_dispatcher(config_, modbus_client);
    gateway::queue::SpscRingBuffer<gateway::core::TelemetryMessage, kTelemetryQueueCapacity>
        telemetry_queue;

    g_stop_requested = 0;
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);

    std::atomic_bool running{true};
    std::thread worker_thread([&running, &worker, &telemetry_queue] {
        while (running.load(std::memory_order_acquire) || telemetry_queue.size() > 0) {
            if (!worker.process_one_from_queue(telemetry_queue)) {
                std::this_thread::sleep_for(kWorkerIdleSleep);
            }
        }
    });

    std::cout << "edge_gateway started gateway_id=" << config_.gateway_id << '\n';
    std::cout << "enter command JSON on stdin to simulate MQTT command payload\n";
    while (g_stop_requested == 0) {
        while (const auto line = try_read_stdin_line()) {
            const auto command = mapper.parse_command(*line);
            if (!command.has_value()) {
                std::cerr << "edge_gateway command parse failed\n";
                continue;
            }

            const auto ack = command_dispatcher.dispatch(*command);
            const auto ack_message = mapper.map_command_ack(ack);
            mqtt_client.publish(ack_message.topic, ack_message.payload);
        }

        const auto result = poller.poll_once();
        if (!result.ok || !result.message.has_value()) {
            std::cerr << "edge_gateway poll failed: " << gateway::core::to_string(result.error)
                      << '\n';
        } else if (!telemetry_queue.push(*result.message)) {
            std::cerr << "edge_gateway queue failed: "
                      << gateway::core::to_string(gateway::core::ErrorCode::queue_full) << '\n';
        }

        sleep_until_next_poll_or_stop(config_.devices.front().poll_interval);
    }

    std::cout << "edge_gateway stopping\n";
    running.store(false, std::memory_order_release);
    worker_thread.join();
    std::cout << "edge_gateway stopped\n";

    return 0;
}

} // namespace gateway::runtime
