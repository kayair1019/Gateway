#include "gateway/core/config.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace gateway::core {
namespace {

auto load_json_file(const std::string& path) -> nlohmann::json {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open config file: " + path);
    }

    nlohmann::json json;
    input >> json;
    return json;
}

auto to_u16(const nlohmann::json& json, const char* field) -> std::uint16_t {
    const auto value = json.at(field).get<int>();
    if (value < 0 || value > 65535) {
        throw std::runtime_error(std::string("config field out of uint16 range: ") + field);
    }
    return static_cast<std::uint16_t>(value);
}

auto to_u8(const nlohmann::json& json, const char* field) -> std::uint8_t {
    const auto value = json.at(field).get<int>();
    if (value < 0 || value > 255) {
        throw std::runtime_error(std::string("config field out of uint8 range: ") + field);
    }
    return static_cast<std::uint8_t>(value);
}

} // namespace

auto load_gateway_config(const std::string& path) -> GatewayConfig {
    const auto json = load_json_file(path);

    GatewayConfig config;
    config.gateway_id = json.value("gateway_id", config.gateway_id);

    const auto& southbound = json.at("southbound");
    for (const auto& device_json : southbound.at("devices")) {
        DeviceConfig device;
        device.device_id = device_json.at("device_id").get<std::string>();
        device.protocol = device_json.value("protocol", device.protocol);
        device.host = device_json.value("host", device.host);
        device.port = to_u16(device_json, "port");
        device.unit_id = to_u8(device_json, "unit_id");
        device.poll_interval =
            std::chrono::milliseconds(device_json.value("poll_interval_ms", 1000));

        for (const auto& point_json : device_json.at("points")) {
            PointConfig point;
            point.name = point_json.at("name").get<std::string>();
            point.address = to_u16(point_json, "address");
            point.type = point_json.value("type", point.type);
            point.scale = point_json.value("scale", point.scale);
            device.points.push_back(std::move(point));
        }

        config.devices.push_back(std::move(device));
    }

    if (json.contains("northbound") && json["northbound"].contains("mqtt")) {
        const auto& mqtt_json = json["northbound"]["mqtt"];
        config.mqtt.host = mqtt_json.value("host", config.mqtt.host);
        config.mqtt.port = to_u16(mqtt_json, "port");
        config.mqtt.client_id = mqtt_json.value("client_id", config.mqtt.client_id);
    }

    if (json.contains("queue")) {
        config.queue.telemetry_capacity = json["queue"].value(
            "telemetry_capacity",
            config.queue.telemetry_capacity);
    }

    return config;
}

auto load_simulator_config(const std::string& path) -> SimulatorConfig {
    const auto json = load_json_file(path);

    SimulatorConfig config;
    config.host = json.value("host", config.host);
    config.port = to_u16(json, "port");
    config.unit_id = to_u8(json, "unit_id");

    for (const auto& item : json.at("holding_registers").items()) {
        const auto address = static_cast<std::uint16_t>(std::stoul(item.key()));
        const auto value = item.value().get<int>();
        if (value < 0 || value > 65535) {
            throw std::runtime_error("holding register value out of uint16 range");
        }
        config.holding_registers[address] = static_cast<std::uint16_t>(value);
    }

    return config;
}

} // namespace gateway::core
