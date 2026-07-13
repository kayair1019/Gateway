#include "gateway/southbound/modbus_tcp_client.hpp"

#include <array>
#include <span>
#include <string>

#include <asio.hpp>

#include "gateway/protocol/modbus_tcp_codec.hpp"

namespace gateway::southbound {
namespace {

constexpr std::size_t kMbapHeaderSize = 7;
constexpr std::size_t kWriteSingleRegisterResponseSize = 12;
constexpr std::uint16_t kModbusProtocolId = 0;
constexpr std::uint8_t kWriteSingleRegister = 0x06;

std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8) |
                                      static_cast<std::uint16_t>(bytes[offset + 1]));
}

ReadRegistersResult make_read_error(gateway::core::ErrorCode error) {
    return {
        .ok = false,
        .error = error,
        .registers = {},
    };
}

WriteRegisterResult make_write_error(gateway::core::ErrorCode error) {
    return {
        .ok = false,
        .error = error,
    };
}

} // namespace

ModbusTcpClient::ModbusTcpClient(asio::io_context& io_context) : io_context_(io_context) {}

ReadRegistersResult ModbusTcpClient::read_holding_registers(
    const gateway::core::DeviceConfig& device,
    std::uint16_t start_address,
    std::uint16_t quantity) {
    const auto transaction_id = next_transaction_id();
    const gateway::protocol::ModbusTcpCodec codec;
    const auto request = codec.encode_read_holding_registers(
        transaction_id,
        device.unit_id,
        start_address,
        quantity);

    try {
        const auto response = send_request(device, request);
        const auto decoded =
            codec.decode_read_holding_registers_response(response, transaction_id, device.unit_id);

        if (!decoded.ok) {
            return make_read_error(decoded.error);
        }

        return {
            .ok = true,
            .error = gateway::core::ErrorCode::none,
            .registers = decoded.response.registers,
        };
    } catch (const asio::system_error&) {
        return make_read_error(gateway::core::ErrorCode::network_error);
    }
}

WriteRegisterResult ModbusTcpClient::write_single_register(
    const gateway::core::DeviceConfig& device,
    std::uint16_t address,
    std::uint16_t value) {
    const auto transaction_id = next_transaction_id();
    const gateway::protocol::ModbusTcpCodec codec;
    const auto request = codec.encode_write_single_register(
        transaction_id,
        device.unit_id,
        address,
        value);

    try {
        const auto response = send_request(device, request);
        if (response.size() != kWriteSingleRegisterResponseSize) {
            return make_write_error(gateway::core::ErrorCode::protocol_error);
        }

        if (read_u16(response, 0) != transaction_id || read_u16(response, 2) != kModbusProtocolId ||
            read_u16(response, 4) != 6 || response[6] != device.unit_id ||
            response[7] != kWriteSingleRegister || read_u16(response, 8) != address ||
            read_u16(response, 10) != value) {
            return make_write_error(gateway::core::ErrorCode::protocol_error);
        }

        return {
            .ok = true,
            .error = gateway::core::ErrorCode::none,
        };
    } catch (const asio::system_error&) {
        return make_write_error(gateway::core::ErrorCode::network_error);
    }
}

std::uint16_t ModbusTcpClient::next_transaction_id() {
    ++transaction_id_;
    if (transaction_id_ == 0) {
        ++transaction_id_;
    }
    return transaction_id_;
}

std::vector<std::uint8_t> ModbusTcpClient::send_request(
    const gateway::core::DeviceConfig& device,
    const std::vector<std::uint8_t>& request) {
    asio::ip::tcp::resolver resolver(io_context_);
    asio::ip::tcp::socket socket(io_context_);

    const auto endpoints = resolver.resolve(device.host, std::to_string(device.port));
    asio::connect(socket, endpoints);
    asio::write(socket, asio::buffer(request));

    std::array<std::uint8_t, kMbapHeaderSize> header{};
    asio::read(socket, asio::buffer(header));

    const auto length = read_u16(header, 4);
    std::vector<std::uint8_t> response(header.begin(), header.end());
    response.resize(kMbapHeaderSize + length - 1);
    asio::read(socket, asio::buffer(response.data() + kMbapHeaderSize, length - 1));

    return response;
}

} // namespace gateway::southbound
