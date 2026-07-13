#include "gateway/southbound/device_poller.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

class FakeModbusClient final : public gateway::southbound::ModbusClient {
public:
    gateway::southbound::ReadRegistersResult read_holding_registers(
        const gateway::core::DeviceConfig&,
        std::uint16_t start_address,
        std::uint16_t quantity) override {
        last_start_address = start_address;
        last_quantity = quantity;
        return read_result;
    }

    gateway::southbound::WriteRegisterResult write_single_register(
        const gateway::core::DeviceConfig&,
        std::uint16_t,
        std::uint16_t) override {
        return {};
    }

    gateway::southbound::ReadRegistersResult read_result;
    std::uint16_t last_start_address{0};
    std::uint16_t last_quantity{0};
};

auto make_device() -> gateway::core::DeviceConfig {
    gateway::core::DeviceConfig device;
    device.device_id = "device-001";
    device.unit_id = 1;
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
    return device;
}

TEST(DevicePollerTest, PollsConfiguredAddressRangeAndBuildsTelemetry) {
    FakeModbusClient client;
    client.read_result = {
        .ok = true,
        .error = gateway::core::ErrorCode::none,
        .registers = std::vector<std::uint16_t>{256, 1234},
    };
    gateway::southbound::DevicePoller poller(make_device(), client);

    const auto result = poller.poll_once();

    ASSERT_TRUE(result.ok);
    ASSERT_TRUE(result.message.has_value());
    EXPECT_EQ(client.last_start_address, 0U);
    EXPECT_EQ(client.last_quantity, 2U);
    EXPECT_EQ(result.message->device_id, "device-001");
    ASSERT_EQ(result.message->points.size(), 2U);
    EXPECT_DOUBLE_EQ(result.message->points[0].value, 25.6);
    EXPECT_DOUBLE_EQ(result.message->points[1].value, 12.34);
}

TEST(DevicePollerTest, ReturnsInvalidConfigWhenDeviceHasNoPoints) {
    FakeModbusClient client;
    gateway::core::DeviceConfig device;
    gateway::southbound::DevicePoller poller(device, client);

    const auto result = poller.poll_once();

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, gateway::core::ErrorCode::invalid_config);
    EXPECT_FALSE(result.message.has_value());
}

TEST(DevicePollerTest, PropagatesReadFailure) {
    FakeModbusClient client;
    client.read_result = {
        .ok = false,
        .error = gateway::core::ErrorCode::network_error,
        .registers = {},
    };
    gateway::southbound::DevicePoller poller(make_device(), client);

    const auto result = poller.poll_once();

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, gateway::core::ErrorCode::network_error);
    EXPECT_FALSE(result.message.has_value());
}

} // namespace
