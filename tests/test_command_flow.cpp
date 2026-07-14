#include "gateway/northbound/fake_mqtt_client.hpp"
#include "gateway/northbound/mqtt_message_mapper.hpp"
#include "gateway/queue/spsc_ring_buffer.hpp"
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

TEST(CommandFlowTest, WorkerConsumesTelemetryFromSpscQueue) {
    gateway::northbound::FakeMqttClient mqtt_client;
    gateway::runtime::Worker worker(
        gateway::rule::RuleEngine{},
        gateway::northbound::MqttMessageMapper{"gateway-001"},
        mqtt_client);
    gateway::queue::SpscRingBuffer<gateway::core::TelemetryMessage, 4> queue;

    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";
    ASSERT_TRUE(queue.push(message));

    EXPECT_TRUE(worker.process_one_from_queue(queue));

    ASSERT_EQ(mqtt_client.published_messages().size(), 1U);
    EXPECT_EQ(mqtt_client.published_messages()[0].topic, "edge/gateway-001/telemetry/device-001");
}

TEST(CommandFlowTest, WorkerReportsEmptyQueue) {
    gateway::northbound::FakeMqttClient mqtt_client;
    gateway::runtime::Worker worker(
        gateway::rule::RuleEngine{},
        gateway::northbound::MqttMessageMapper{"gateway-001"},
        mqtt_client);
    gateway::queue::SpscRingBuffer<gateway::core::TelemetryMessage, 4> queue;

    EXPECT_FALSE(worker.process_one_from_queue(queue));
    EXPECT_TRUE(mqtt_client.published_messages().empty());
}

TEST(CommandFlowTest, WorkerDropsTelemetryWhenRuleDoesNotMatch) {
    gateway::northbound::FakeMqttClient mqtt_client;
    gateway::runtime::Worker worker(
        gateway::rule::RuleEngine({
            {
                .name = "temperature_limit",
                .point = "temperature",
                .op = gateway::rule::CompareOperator::less_than,
                .value = 10.0,
                .action = gateway::rule::RuleAction::allow,
            },
        }),
        gateway::northbound::MqttMessageMapper{"gateway-001"},
        mqtt_client);

    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";
    message.points.push_back({
        .name = "temperature",
        .address = 0,
        .value = 25.6,
        .quality = gateway::core::PointQuality::good,
    });

    worker.process_once(message);

    EXPECT_TRUE(mqtt_client.published_messages().empty());
}

} // namespace
