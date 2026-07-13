#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "gateway/core/error.hpp"

namespace gateway::protocol {

struct ModbusReadResponse {
    std::uint16_t transaction_id{0};
    std::uint8_t unit_id{0};
    std::vector<std::uint16_t> registers;
};

struct DecodeResult {
    bool ok{false};
    gateway::core::ErrorCode error{gateway::core::ErrorCode::none};
    ModbusReadResponse response;
};

class ModbusTcpCodec {
public:
    std::vector<std::uint8_t> encode_read_holding_registers(
        std::uint16_t transaction_id,
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity) const;

    std::vector<std::uint8_t> encode_write_single_register(
        std::uint16_t transaction_id,
        std::uint8_t unit_id,
        std::uint16_t address,
        std::uint16_t value) const;

    DecodeResult decode_read_holding_registers_response(
        std::span<const std::uint8_t> bytes,
        std::uint16_t expected_transaction_id,
        std::uint8_t expected_unit_id) const;
};

} // namespace gateway::protocol
