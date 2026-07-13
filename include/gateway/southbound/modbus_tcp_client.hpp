#pragma once

#include <cstdint>
#include <vector>

#include <asio/io_context.hpp>

#include "gateway/core/config.hpp"
#include "gateway/core/error.hpp"

namespace gateway::southbound {

struct ReadRegistersResult {
    bool ok{false};
    gateway::core::ErrorCode error{gateway::core::ErrorCode::none};
    std::vector<std::uint16_t> registers;
};

struct WriteRegisterResult {
    bool ok{false};
    gateway::core::ErrorCode error{gateway::core::ErrorCode::none};
};

class ModbusClient {
public:
    virtual ~ModbusClient() = default;

    virtual ReadRegistersResult read_holding_registers(
        const gateway::core::DeviceConfig& device,
        std::uint16_t start_address,
        std::uint16_t quantity) = 0;

    virtual WriteRegisterResult write_single_register(
        const gateway::core::DeviceConfig& device,
        std::uint16_t address,
        std::uint16_t value) = 0;
};

class ModbusTcpClient : public ModbusClient {
public:
    explicit ModbusTcpClient(asio::io_context& io_context);

    ReadRegistersResult read_holding_registers(
        const gateway::core::DeviceConfig& device,
        std::uint16_t start_address,
        std::uint16_t quantity) override;

    WriteRegisterResult write_single_register(
        const gateway::core::DeviceConfig& device,
        std::uint16_t address,
        std::uint16_t value) override;

private:
    std::uint16_t next_transaction_id();
    std::vector<std::uint8_t> send_request(
        const gateway::core::DeviceConfig& device,
        const std::vector<std::uint8_t>& request);

    asio::io_context& io_context_;
    std::uint16_t transaction_id_{0};
};

} // namespace gateway::southbound
