#pragma once

#include <optional>

#include "gateway/core/config.hpp"
#include "gateway/core/error.hpp"
#include "gateway/core/message.hpp"
#include "gateway/southbound/modbus_tcp_client.hpp"

namespace gateway::southbound {

struct PollResult {
    bool ok{false};
    gateway::core::ErrorCode error{gateway::core::ErrorCode::none};
    std::optional<gateway::core::TelemetryMessage> message;
};

class DevicePoller {
public:
    DevicePoller(gateway::core::DeviceConfig device, ModbusClient& client);

    PollResult poll_once();

private:
    gateway::core::DeviceConfig device_;
    ModbusClient& client_;
};

} // namespace gateway::southbound
