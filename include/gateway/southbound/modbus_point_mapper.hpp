#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "gateway/core/config.hpp"
#include "gateway/core/message.hpp"

namespace gateway::southbound {

gateway::core::TelemetryMessage map_registers_to_telemetry(
    const gateway::core::DeviceConfig& device,
    std::uint16_t start_address,
    const std::vector<std::uint16_t>& registers,
    std::chrono::system_clock::time_point timestamp);

} // namespace gateway::southbound
