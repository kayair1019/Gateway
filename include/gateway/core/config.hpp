#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gateway::core {

struct PointConfig {
    std::string name;
    std::uint16_t address{0};
    std::string type{"uint16"};
    double scale{1.0};
};

struct DeviceConfig {
    std::string device_id;
    std::string protocol{"modbus_tcp"};
    std::string host{"127.0.0.1"};
    std::uint16_t port{1502};
    std::uint8_t unit_id{1};
    std::chrono::milliseconds poll_interval{1000};
    std::vector<PointConfig> points;
};

struct MqttConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{1883};
    std::string client_id{"edge-gateway-001"};
};

struct QueueConfig {
    std::size_t telemetry_capacity{1024};
};

struct GatewayConfig {
    std::string gateway_id{"gateway-001"};
    std::vector<DeviceConfig> devices;
    MqttConfig mqtt;
    QueueConfig queue;
};

struct SimulatorConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{1502};
    std::uint8_t unit_id{1};
    std::unordered_map<std::uint16_t, std::uint16_t> holding_registers;
};

GatewayConfig load_gateway_config(const std::string& path);
SimulatorConfig load_simulator_config(const std::string& path);

} // namespace gateway::core
