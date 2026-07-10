#include "gateway/northbound/mqtt_message_mapper.hpp"

#include <chrono>

#include <gtest/gtest.h>

namespace {

TEST(MqttMessageMapperTest, MapsTelemetryToTopicAndPayload) {
    const gateway::northbound::MqttMessageMapper mapper("gateway-001");

    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";
    message.timestamp = std::chrono::system_clock::time_point{std::chrono::milliseconds{1}};

    const auto mapped = mapper.map_telemetry(message);

    EXPECT_EQ(mapped.topic, "edge/gateway-001/telemetry/device-001");
    EXPECT_NE(mapped.payload.find("\"device_id\":\"device-001\""), std::string::npos);
}

TEST(MqttMessageMapperTest, ParsesCommandPayload) {
    const gateway::northbound::MqttMessageMapper mapper("gateway-001");

    const auto command = mapper.parse_command(
        R"({"command_id":"cmd-001","device_id":"device-001","type":"read_register","address":0,"count":2})");

    ASSERT_TRUE(command.has_value());
    EXPECT_EQ(command->count, 2U);
}

} // namespace
