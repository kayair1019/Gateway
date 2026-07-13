#include "gateway/southbound/modbus_point_mapper.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

TEST(ModbusPointMapperTest, MapsRegistersToTelemetryPointsWithScale) {
    gateway::core::DeviceConfig device;
    device.device_id = "device-001";
    device.protocol = "modbus_tcp";
    device.points = {
        {
            .name = "temperature",
            .address = 0,
            .type = "uint16",
            .scale = 0.1,
        },
        {
            .name = "pressure",
            .address = 1,
            .type = "uint16",
            .scale = 0.01,
        },
    };

    const std::vector<std::uint16_t> registers{256, 1234};
    const auto timestamp =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};

    const auto message =
        gateway::southbound::map_registers_to_telemetry(device, 0, registers, timestamp);

    EXPECT_EQ(message.device_id, "device-001");
    EXPECT_EQ(message.protocol, "modbus_tcp");
    EXPECT_EQ(message.timestamp, timestamp);
    ASSERT_EQ(message.points.size(), 2U);
    EXPECT_EQ(message.points[0].name, "temperature");
    EXPECT_DOUBLE_EQ(message.points[0].value, 25.6);
    EXPECT_EQ(message.points[1].name, "pressure");
    EXPECT_DOUBLE_EQ(message.points[1].value, 12.34);
}

TEST(ModbusPointMapperTest, SkipsPointsOutsideReadRange) {
    gateway::core::DeviceConfig device;
    device.device_id = "device-001";
    device.points = {
        {
            .name = "temperature",
            .address = 0,
            .type = "uint16",
            .scale = 0.1,
        },
        {
            .name = "pressure",
            .address = 10,
            .type = "uint16",
            .scale = 0.01,
        },
    };

    const std::vector<std::uint16_t> registers{256};
    const auto message = gateway::southbound::map_registers_to_telemetry(
        device,
        0,
        registers,
        std::chrono::system_clock::now());

    ASSERT_EQ(message.points.size(), 1U);
    EXPECT_EQ(message.points[0].name, "temperature");
}

} // namespace
