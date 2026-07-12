#include "gateway/southbound/modbus_device_simulator.hpp"

#include <array>
#include <iostream>

#include <asio.hpp>

namespace gateway::southbound {
namespace {

constexpr std::uint16_t kProtocolId = 0;
constexpr std::uint8_t kReadHoldingRegisters = 0x03;
constexpr std::uint8_t kWriteSingleRegister = 0x06;
constexpr std::uint8_t kIllegalFunction = 0x01;
constexpr std::uint8_t kIllegalDataAddress = 0x02;
constexpr std::size_t kMbapHeaderSize = 7;
constexpr std::size_t kRequestSize = 12;

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
}

auto read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) -> std::uint16_t {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8) |
                                      static_cast<std::uint16_t>(bytes[offset + 1]));
}

auto make_exception_response(
    std::uint16_t transaction_id,
    std::uint8_t unit_id,
    std::uint8_t function_code,
    std::uint8_t exception_code) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> response;
    response.reserve(9);
    append_u16(response, transaction_id);
    append_u16(response, kProtocolId);
    append_u16(response, 3);
    response.push_back(unit_id);
    response.push_back(static_cast<std::uint8_t>(function_code | 0x80U));
    response.push_back(exception_code);
    return response;
}

} // namespace

ModbusDeviceSimulator::ModbusDeviceSimulator(gateway::core::SimulatorConfig config)
    : config_(std::move(config)), holding_registers_(config_.holding_registers) {}

auto ModbusDeviceSimulator::run() -> int {
    asio::io_context io_context;
    const auto address = asio::ip::make_address(config_.host);
    asio::ip::tcp::acceptor acceptor(
        io_context,
        asio::ip::tcp::endpoint(address, config_.port));

    std::cout << "modbus_simulator listening on " << config_.host << ':' << config_.port << '\n';

    for (;;) {
        asio::ip::tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "modbus_simulator accepted connection\n";

        asio::error_code error;
        for (;;) {
            std::array<std::uint8_t, kMbapHeaderSize> header{};
            asio::read(socket, asio::buffer(header), error);
            if (error) {
                break;
            }

            const auto length = read_u16(header, 4);
            if (length == 0) {
                break;
            }

            std::vector<std::uint8_t> request(header.begin(), header.end());
            request.resize(kMbapHeaderSize + length - 1);
            asio::read(socket, asio::buffer(request.data() + kMbapHeaderSize, length - 1), error);
            if (error) {
                break;
            }

            const auto response = handle_request(request);
            if (!response.empty()) {
                asio::write(socket, asio::buffer(response), error);
                if (error) {
                    break;
                }
            }
        }
    }
}

auto ModbusDeviceSimulator::handle_request(std::span<const std::uint8_t> request)
    -> std::vector<std::uint8_t> {
    if (request.size() < kRequestSize) {
        return {};
    }

    const auto transaction_id = read_u16(request, 0);
    const auto protocol_id = read_u16(request, 2);
    const auto length = read_u16(request, 4);
    const auto unit_id = request[6];
    const auto function_code = request[7];

    if (protocol_id != kProtocolId || length != 6 || unit_id != config_.unit_id) {
        return {};
    }

    const auto address = read_u16(request, 8);
    const auto value_or_quantity = read_u16(request, 10);

    if (function_code == kReadHoldingRegisters) {
        const auto quantity = value_or_quantity;
        std::vector<std::uint8_t> response;
        response.reserve(9 + quantity * 2);

        for (std::uint16_t offset = 0; offset < quantity; ++offset) {
            if (!holding_registers_.contains(static_cast<std::uint16_t>(address + offset))) {
                return make_exception_response(
                    transaction_id,
                    unit_id,
                    function_code,
                    kIllegalDataAddress);
            }
        }

        append_u16(response, transaction_id);
        append_u16(response, kProtocolId);
        append_u16(response, static_cast<std::uint16_t>(3 + quantity * 2));
        response.push_back(unit_id);
        response.push_back(function_code);
        response.push_back(static_cast<std::uint8_t>(quantity * 2));

        for (std::uint16_t offset = 0; offset < quantity; ++offset) {
            append_u16(response, holding_registers_.at(static_cast<std::uint16_t>(address + offset)));
        }

        return response;
    }

    if (function_code == kWriteSingleRegister) {
        holding_registers_[address] = value_or_quantity;
        return std::vector<std::uint8_t>(request.begin(), request.end());
    }

    return make_exception_response(transaction_id, unit_id, function_code, kIllegalFunction);
}

} // namespace gateway::southbound
