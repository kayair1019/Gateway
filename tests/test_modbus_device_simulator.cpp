#include "gateway/southbound/modbus_device_simulator.hpp"

#include "gateway/protocol/modbus_tcp_codec.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

auto make_config() -> gateway::core::SimulatorConfig {
    gateway::core::SimulatorConfig config;
    config.unit_id = 1;
    config.holding_registers = {
        {0, 256},
        {1, 1234},
    };
    return config;
}

TEST(ModbusDeviceSimulatorTest, HandlesReadHoldingRegistersRequest) {
    gateway::southbound::ModbusDeviceSimulator simulator(make_config());
    const gateway::protocol::ModbusTcpCodec codec;
    const auto request = codec.encode_read_holding_registers(11, 1, 0, 2);

    const auto response = simulator.handle_request(request);
    const auto decoded = codec.decode_read_holding_registers_response(response, 11, 1);

    ASSERT_TRUE(decoded.ok);
    ASSERT_EQ(decoded.response.registers.size(), 2U);
    EXPECT_EQ(decoded.response.registers[0], 256U);
    EXPECT_EQ(decoded.response.registers[1], 1234U);
}

TEST(ModbusDeviceSimulatorTest, HandlesWriteSingleRegisterRequest) {
    gateway::southbound::ModbusDeviceSimulator simulator(make_config());
    const gateway::protocol::ModbusTcpCodec codec;
    const auto write_request = codec.encode_write_single_register(12, 1, 1, 42);

    const auto write_response = simulator.handle_request(write_request);

    EXPECT_EQ(write_response, write_request);

    const auto read_request = codec.encode_read_holding_registers(13, 1, 1, 1);
    const auto read_response = simulator.handle_request(read_request);
    const auto decoded = codec.decode_read_holding_registers_response(read_response, 13, 1);

    ASSERT_TRUE(decoded.ok);
    ASSERT_EQ(decoded.response.registers.size(), 1U);
    EXPECT_EQ(decoded.response.registers[0], 42U);
}

TEST(ModbusDeviceSimulatorTest, ReturnsExceptionForMissingRegister) {
    gateway::southbound::ModbusDeviceSimulator simulator(make_config());
    const gateway::protocol::ModbusTcpCodec codec;
    const auto request = codec.encode_read_holding_registers(14, 1, 9, 1);

    const auto response = simulator.handle_request(request);

    const std::vector<std::uint8_t> expected{
        0x00, 0x0e, 0x00, 0x00, 0x00, 0x03,
        0x01, 0x83, 0x02,
    };
    EXPECT_EQ(response, expected);
}

} // namespace
