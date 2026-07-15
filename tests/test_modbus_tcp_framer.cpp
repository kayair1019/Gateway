#include "gateway/protocol/modbus_tcp_framer.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

const std::vector<std::uint8_t> kReadRequest{
    0x00, 0x01, 0x00, 0x00, 0x00, 0x06,
    0x01, 0x03, 0x00, 0x00, 0x00, 0x02,
};

const std::vector<std::uint8_t> kWriteRequest{
    0x00, 0x02, 0x00, 0x00, 0x00, 0x06,
    0x01, 0x06, 0x00, 0x01, 0x10, 0xe1,
};

TEST(ModbusTcpFramerTest, KeepsIncompleteHeaderBuffered) {
    gateway::protocol::ModbusTcpFramer framer;
    const std::vector<std::uint8_t> bytes{kReadRequest.begin(), kReadRequest.begin() + 3};

    const auto frames = framer.push_bytes(bytes);

    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(framer.buffered_size(), 3U);
    EXPECT_FALSE(framer.has_error());
}

TEST(ModbusTcpFramerTest, KeepsIncompleteBodyBuffered) {
    gateway::protocol::ModbusTcpFramer framer;
    const std::vector<std::uint8_t> bytes{kReadRequest.begin(), kReadRequest.begin() + 9};

    const auto frames = framer.push_bytes(bytes);

    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(framer.buffered_size(), 9U);
    EXPECT_FALSE(framer.has_error());
}

TEST(ModbusTcpFramerTest, EmitsFrameAfterHalfPacketArrives) {
    gateway::protocol::ModbusTcpFramer framer;
    const std::vector<std::uint8_t> first{kReadRequest.begin(), kReadRequest.begin() + 5};
    const std::vector<std::uint8_t> second{kReadRequest.begin() + 5, kReadRequest.end()};

    EXPECT_TRUE(framer.push_bytes(first).empty());
    const auto frames = framer.push_bytes(second);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0], kReadRequest);
    EXPECT_EQ(framer.buffered_size(), 0U);
}

TEST(ModbusTcpFramerTest, EmitsMultipleFramesFromStickyPacket) {
    gateway::protocol::ModbusTcpFramer framer;
    std::vector<std::uint8_t> bytes = kReadRequest;
    bytes.insert(bytes.end(), kWriteRequest.begin(), kWriteRequest.end());

    const auto frames = framer.push_bytes(bytes);

    ASSERT_EQ(frames.size(), 2U);
    EXPECT_EQ(frames[0], kReadRequest);
    EXPECT_EQ(frames[1], kWriteRequest);
    EXPECT_EQ(framer.buffered_size(), 0U);
}

TEST(ModbusTcpFramerTest, EmitsCompleteFrameAndKeepsPartialNextFrame) {
    gateway::protocol::ModbusTcpFramer framer;
    std::vector<std::uint8_t> bytes = kReadRequest;
    bytes.insert(bytes.end(), kWriteRequest.begin(), kWriteRequest.begin() + 4);

    const auto frames = framer.push_bytes(bytes);

    ASSERT_EQ(frames.size(), 1U);
    EXPECT_EQ(frames[0], kReadRequest);
    EXPECT_EQ(framer.buffered_size(), 4U);
}

TEST(ModbusTcpFramerTest, RejectsInvalidMbapLength) {
    gateway::protocol::ModbusTcpFramer framer;
    std::vector<std::uint8_t> bytes = kReadRequest;
    bytes[4] = 0xff;
    bytes[5] = 0xff;

    const auto frames = framer.push_bytes(bytes);

    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(framer.buffered_size(), 0U);
    EXPECT_TRUE(framer.has_error());
}

} // namespace
