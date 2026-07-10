#include "gateway/protocol/modbus_tcp_codec.hpp"

namespace gateway::protocol {
namespace {

constexpr std::uint16_t kProtocolId = 0;
constexpr std::uint8_t kReadHoldingRegisters = 0x03;
constexpr std::uint8_t kWriteSingleRegister = 0x06;
constexpr std::size_t kMbapHeaderSize = 7;
constexpr std::size_t kReadResponseMinSize = 9;

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
}

auto read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) -> std::uint16_t {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8) |
                                      static_cast<std::uint16_t>(bytes[offset + 1]));
}

auto make_error(gateway::core::ErrorCode error) -> DecodeResult {
    DecodeResult result;
    result.ok = false;
    result.error = error;
    return result;
}

} // namespace

auto ModbusTcpCodec::encode_read_holding_registers(
    std::uint16_t transaction_id,
    std::uint8_t unit_id,
    std::uint16_t start_address,
    std::uint16_t quantity) const -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(12);

    append_u16(bytes, transaction_id);
    append_u16(bytes, kProtocolId);
    append_u16(bytes, 6);
    bytes.push_back(unit_id);
    bytes.push_back(kReadHoldingRegisters);
    append_u16(bytes, start_address);
    append_u16(bytes, quantity);

    return bytes;
}

auto ModbusTcpCodec::encode_write_single_register(
    std::uint16_t transaction_id,
    std::uint8_t unit_id,
    std::uint16_t address,
    std::uint16_t value) const -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(12);

    append_u16(bytes, transaction_id);
    append_u16(bytes, kProtocolId);
    append_u16(bytes, 6);
    bytes.push_back(unit_id);
    bytes.push_back(kWriteSingleRegister);
    append_u16(bytes, address);
    append_u16(bytes, value);

    return bytes;
}

auto ModbusTcpCodec::decode_read_holding_registers_response(
    std::span<const std::uint8_t> bytes,
    std::uint16_t expected_transaction_id,
    std::uint8_t expected_unit_id) const -> DecodeResult {
    if (bytes.size() < kReadResponseMinSize) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    const auto transaction_id = read_u16(bytes, 0);
    const auto protocol_id = read_u16(bytes, 2);
    const auto length = read_u16(bytes, 4);
    const auto unit_id = bytes[6];
    const auto function_code = bytes[7];

    if (transaction_id != expected_transaction_id || unit_id != expected_unit_id) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    if (protocol_id != kProtocolId || bytes.size() != kMbapHeaderSize + length - 1) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    if ((function_code & 0x80U) != 0U) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    if (function_code != kReadHoldingRegisters) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    const auto byte_count = bytes[8];
    if (byte_count % 2 != 0 || bytes.size() != kReadResponseMinSize + byte_count) {
        return make_error(gateway::core::ErrorCode::protocol_error);
    }

    DecodeResult result;
    result.ok = true;
    result.error = gateway::core::ErrorCode::none;
    result.response.transaction_id = transaction_id;
    result.response.unit_id = unit_id;

    for (std::size_t offset = 9; offset < bytes.size(); offset += 2) {
        result.response.registers.push_back(read_u16(bytes, offset));
    }

    return result;
}

} // namespace gateway::protocol
