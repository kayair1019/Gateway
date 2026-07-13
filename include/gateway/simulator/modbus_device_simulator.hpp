#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "gateway/core/config.hpp"

namespace gateway::simulator {

class ModbusDeviceSimulator {
public:
    explicit ModbusDeviceSimulator(gateway::core::SimulatorConfig config);

    int run();
    std::vector<std::uint8_t> handle_request(std::span<const std::uint8_t> request);

private:
    gateway::core::SimulatorConfig config_;
    std::unordered_map<std::uint16_t, std::uint16_t> holding_registers_;
};

} // namespace gateway::simulator
