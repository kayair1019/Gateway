#include "gateway/northbound/fake_mqtt_client.hpp"
#include "gateway/northbound/mqtt_message_mapper.hpp"
#include "gateway/rule/rule_engine.hpp"
#include "gateway/runtime/worker.hpp"

#include <gtest/gtest.h>

namespace {

TEST(CommandFlowTest, WorkerPublishesAllowedTelemetry) {
    gateway::northbound::FakeMqttClient mqtt_client;
    gateway::runtime::Worker worker(
        gateway::rule::RuleEngine{},
        gateway::northbound::MqttMessageMapper{"gateway-001"},
        mqtt_client);

    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";

    worker.process_once(message);

    ASSERT_EQ(mqtt_client.published_messages().size(), 1U);
    EXPECT_EQ(mqtt_client.published_messages()[0].topic, "edge/gateway-001/telemetry/device-001");
}

} // namespace
