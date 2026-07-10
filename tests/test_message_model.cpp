#include "gateway/core/message.hpp"

#include <chrono>

#include <gtest/gtest.h>

namespace {

TEST(MessageModelTest, SerializesTelemetryToJson) {
    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";
    message.timestamp = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    message.points.push_back({
        .name = "temperature",
        .address = 0,
        .value = 25.6,
        .quality = gateway::core::PointQuality::good,
    });

    const auto json = gateway::core::to_json(message);

    EXPECT_EQ(json["device_id"], "device-001");
    EXPECT_EQ(json["protocol"], "modbus_tcp");
    EXPECT_EQ(json["timestamp_ms"], 1000);
    EXPECT_EQ(json["points"][0]["name"], "temperature");
}

TEST(MessageModelTest, ParsesWriteRegisterCommand) {
    const nlohmann::json payload{
        {"command_id", "cmd-001"},
        {"device_id", "device-001"},
        {"type", "write_register"},
        {"address", 0},
        {"value", 123},
    };

    const auto command = gateway::core::parse_command(payload);

    ASSERT_TRUE(command.has_value());
    EXPECT_EQ(command->command_id, "cmd-001");
    EXPECT_EQ(command->type, gateway::core::CommandType::write_register);
    EXPECT_EQ(command->value, 123U);
}

} // namespace
