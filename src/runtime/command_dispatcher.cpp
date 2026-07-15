#include "gateway/runtime/command_dispatcher.hpp"

#include <chrono>
#include <string>
#include <utility>

namespace gateway::runtime {

CommandDispatcher::CommandDispatcher(
    const gateway::core::GatewayConfig& config,
    gateway::southbound::ModbusClient& modbus_client)
    : config_(config), modbus_client_(modbus_client) {}

gateway::core::CommandAck CommandDispatcher::dispatch(
    const gateway::core::CommandMessage& command) {
    const auto* device = find_device(command.device_id);
    if (device == nullptr) {
        return make_failed_ack(
            command,
            gateway::core::ErrorCode::device_not_found,
            "device not found");
    }

    if (command.type == gateway::core::CommandType::write_register) {
        const auto result =
            modbus_client_.write_single_register(*device, command.address, command.value);
        if (!result.ok) {
            return make_failed_ack(command, result.error, "write register failed");
        }

        return make_success_ack(command, "register written");
    }

    if (command.type == gateway::core::CommandType::read_register) {
        const auto result =
            modbus_client_.read_holding_registers(*device, command.address, command.count);
        if (!result.ok) {
            return make_failed_ack(command, result.error, "read register failed");
        }

        return make_success_ack(command, "register read");
    }

    return make_failed_ack(
        command,
        gateway::core::ErrorCode::command_not_supported,
        "command not supported");
}

const gateway::core::DeviceConfig* CommandDispatcher::find_device(
    const std::string& device_id) const {
    for (const auto& device : config_.devices) {
        if (device.device_id == device_id) {
            return &device;
        }
    }

    return nullptr;
}

gateway::core::CommandAck CommandDispatcher::make_failed_ack(
    const gateway::core::CommandMessage& command,
    gateway::core::ErrorCode error,
    std::string message) const {
    return {
        .command_id = command.command_id,
        .device_id = command.device_id,
        .status = gateway::core::CommandStatus::failed,
        .error = error,
        .message = std::move(message),
        .timestamp = std::chrono::system_clock::now(),
    };
}

gateway::core::CommandAck CommandDispatcher::make_success_ack(
    const gateway::core::CommandMessage& command,
    std::string message) const {
    return {
        .command_id = command.command_id,
        .device_id = command.device_id,
        .status = gateway::core::CommandStatus::success,
        .error = gateway::core::ErrorCode::none,
        .message = std::move(message),
        .timestamp = std::chrono::system_clock::now(),
    };
}

} // namespace gateway::runtime
