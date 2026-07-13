#include "gateway/southbound/device_poller.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "gateway/southbound/modbus_point_mapper.hpp"

namespace gateway::southbound {

DevicePoller::DevicePoller(gateway::core::DeviceConfig device, ModbusClient& client)
    : device_(std::move(device)), client_(client) {}

PollResult DevicePoller::poll_once() {
    if (device_.points.empty()) {
        return {
            .ok = false,
            .error = gateway::core::ErrorCode::invalid_config,
            .message = std::nullopt,
        };
    }

    const auto minmax = std::minmax_element(
        device_.points.begin(),
        device_.points.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.address < rhs.address;
        });

    const auto start_address = minmax.first->address;
    const auto end_address = minmax.second->address;
    const auto quantity = static_cast<std::uint16_t>(end_address - start_address + 1);

    const auto read_result = client_.read_holding_registers(device_, start_address, quantity);
    if (!read_result.ok) {
        return {
            .ok = false,
            .error = read_result.error,
            .message = std::nullopt,
        };
    }

    return {
        .ok = true,
        .error = gateway::core::ErrorCode::none,
        .message = map_registers_to_telemetry(
            device_,
            start_address,
            read_result.registers,
            std::chrono::system_clock::now()),
    };
}

} // namespace gateway::southbound
