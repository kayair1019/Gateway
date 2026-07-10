#include "gateway/protocol/modbus_tcp_codec.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(ModbusTcpCodecTest, EncodesReadHoldingRegistersRequest) {
    const gateway::protocol::ModbusTcpCodec codec;

    const auto bytes = codec.encode_read_holding_registers(1, 2, 0, 2);

    const std::vector<std::uint8_t> expected{
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06,
        0x02, 0x03, 0x00, 0x00, 0x00, 0x02,
    };
    EXPECT_EQ(bytes, expected);
}

TEST(ModbusTcpCodecTest, EncodesWriteSingleRegisterRequest) {
    const gateway::protocol::ModbusTcpCodec codec;

    const auto bytes = codec.encode_write_single_register(7, 1, 3, 123);

    const std::vector<std::uint8_t> expected{
        0x00, 0x07, 0x00, 0x00, 0x00, 0x06,
        0x01, 0x06, 0x00, 0x03, 0x00, 0x7b,
    };
    EXPECT_EQ(bytes, expected);
}

TEST(ModbusTcpCodecTest, DecodesReadHoldingRegistersResponse) {
    const gateway::protocol::ModbusTcpCodec codec;
    const std::vector<std::uint8_t> bytes{
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07,
        0x02, 0x03, 0x04, 0x00, 0x0a, 0x01, 0x00,
    };

    const auto result = codec.decode_read_holding_registers_response(bytes, 1, 2);

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.response.registers.size(), 2U);
    EXPECT_EQ(result.response.registers[0], 10U);
    EXPECT_EQ(result.response.registers[1], 256U);
}

} // namespace
