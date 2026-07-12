#include "gateway/protocol/modbus_tcp_codec.hpp"

namespace gateway::protocol {
namespace {

constexpr std::uint16_t kProtocolId = 0;
constexpr std::uint8_t kReadHoldingRegisters = 0x03;
constexpr std::uint8_t kWriteSingleRegister = 0x06;
constexpr std::size_t kMbapHeaderSize = 7;
constexpr std::size_t kReadResponseMinSize = 9;

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    /* TODO: 实现该函数 */
}

auto read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) -> std::uint16_t {
    return {};
}

auto make_error(gateway::core::ErrorCode error) -> DecodeResult {
    return {};
}

} // namespace

auto ModbusTcpCodec::encode_read_holding_registers(
    std::uint16_t transaction_id,
    std::uint8_t unit_id,
    std::uint16_t start_address,
    std::uint16_t quantity) const -> std::vector<std::uint8_t> {
    return {};
}

auto ModbusTcpCodec::encode_write_single_register(
    std::uint16_t transaction_id,
    std::uint8_t unit_id,
    std::uint16_t address,
    std::uint16_t value) const -> std::vector<std::uint8_t> {
    return {};
}

auto ModbusTcpCodec::decode_read_holding_registers_response(
    std::span<const std::uint8_t> bytes,
    std::uint16_t expected_transaction_id,
    std::uint8_t expected_unit_id) const -> DecodeResult {
    return {};
}

} // namespace gateway::protocol
