#include "gateway/core/config.hpp"

#include <string>

#include <gtest/gtest.h>

namespace {

auto source_path(const char* relative_path) -> std::string {
    return std::string(EGW_SOURCE_DIR) + "/" + relative_path;
}

TEST(ConfigTest, LoadsGatewayConfig) {
    const auto config = gateway::core::load_gateway_config(source_path("configs/gateway.json"));

    EXPECT_EQ(config.gateway_id, "gateway-001");
    ASSERT_EQ(config.devices.size(), 1U);
    EXPECT_EQ(config.devices[0].device_id, "device-001");
    EXPECT_EQ(config.devices[0].port, 1502U);
    EXPECT_EQ(config.devices[0].unit_id, 1U);
    ASSERT_EQ(config.devices[0].points.size(), 2U);
    EXPECT_EQ(config.devices[0].points[0].name, "temperature");
    EXPECT_EQ(config.mqtt.client_id, "edge-gateway-001");
    EXPECT_EQ(config.queue.telemetry_capacity, 1024U);
}

TEST(ConfigTest, LoadsSimulatorConfig) {
    const auto config = gateway::core::load_simulator_config(source_path("configs/simulator.json"));

    EXPECT_EQ(config.host, "127.0.0.1");
    EXPECT_EQ(config.port, 1502U);
    EXPECT_EQ(config.unit_id, 1U);
    EXPECT_EQ(config.holding_registers.at(0), 256U);
    EXPECT_EQ(config.holding_registers.at(1), 1234U);
}

} // namespace
