#pragma once

#include "gateway/core/config.hpp"
#include "gateway/core/message.hpp"
#include "gateway/southbound/modbus_tcp_client.hpp"

namespace gateway::runtime {

class CommandDispatcher {
public:
    CommandDispatcher(
        const gateway::core::GatewayConfig& config,
        gateway::southbound::ModbusClient& modbus_client);

    gateway::core::CommandAck dispatch(const gateway::core::CommandMessage& command);

private:
    const gateway::core::DeviceConfig* find_device(const std::string& device_id) const;
    gateway::core::CommandAck make_failed_ack(
        const gateway::core::CommandMessage& command,
        gateway::core::ErrorCode error,
        std::string message) const;
    gateway::core::CommandAck make_success_ack(
        const gateway::core::CommandMessage& command,
        std::string message) const;

    const gateway::core::GatewayConfig& config_;
    gateway::southbound::ModbusClient& modbus_client_;
};

} // namespace gateway::runtime
