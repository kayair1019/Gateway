#include "gateway/southbound/modbus_point_mapper.hpp"

namespace gateway::southbound {

gateway::core::TelemetryMessage map_registers_to_telemetry(
    const gateway::core::DeviceConfig& device,
    std::uint16_t start_address,
    const std::vector<std::uint16_t>& registers,
    std::chrono::system_clock::time_point timestamp) {
    gateway::core::TelemetryMessage message;
    message.device_id = device.device_id;
    message.protocol = device.protocol;
    message.timestamp = timestamp;

    for (const auto& point : device.points) {
        if (point.address < start_address) {
            continue;
        }

        const auto offset = static_cast<std::size_t>(point.address - start_address);
        if (offset >= registers.size()) {
            continue;
        }

        message.points.push_back({
            .name = point.name,
            .address = point.address,
            .value = static_cast<double>(registers[offset]) * point.scale,
            .quality = gateway::core::PointQuality::good,
        });
    }

    return message;
}

} // namespace gateway::southbound
