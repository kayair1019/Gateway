#include "gateway/runtime/command_dispatcher.hpp"

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
        last_read_address = start_address;
        last_read_quantity = quantity;
        return read_result;
    }

    gateway::southbound::WriteRegisterResult write_single_register(
        const gateway::core::DeviceConfig&,
        std::uint16_t address,
        std::uint16_t value) override {
        last_write_address = address;
        last_write_value = value;
        return write_result;
    }

    gateway::southbound::ReadRegistersResult read_result{
        .ok = true,
        .error = gateway::core::ErrorCode::none,
        .registers = std::vector<std::uint16_t>{42},
    };
    gateway::southbound::WriteRegisterResult write_result{
        .ok = true,
        .error = gateway::core::ErrorCode::none,
    };
    std::uint16_t last_read_address{0};
    std::uint16_t last_read_quantity{0};
    std::uint16_t last_write_address{0};
    std::uint16_t last_write_value{0};
};

gateway::core::GatewayConfig make_config() {
    gateway::core::GatewayConfig config;
    gateway::core::DeviceConfig device;
    device.device_id = "device-001";
    device.unit_id = 1;
    config.devices.push_back(device);
    return config;
}

TEST(CommandDispatcherTest, DispatchesWriteRegisterCommand) {
    auto config = make_config();
    FakeModbusClient client;
    gateway::runtime::CommandDispatcher dispatcher(config, client);
    gateway::core::CommandMessage command;
    command.command_id = "cmd-001";
    command.device_id = "device-001";
    command.type = gateway::core::CommandType::write_register;
    command.address = 1;
    command.value = 123;

    const auto ack = dispatcher.dispatch(command);

    EXPECT_EQ(client.last_write_address, 1U);
    EXPECT_EQ(client.last_write_value, 123U);
    EXPECT_EQ(ack.command_id, "cmd-001");
    EXPECT_EQ(ack.status, gateway::core::CommandStatus::success);
    EXPECT_EQ(ack.message, "register written");
}

TEST(CommandDispatcherTest, DispatchesReadRegisterCommand) {
    auto config = make_config();
    FakeModbusClient client;
    gateway::runtime::CommandDispatcher dispatcher(config, client);
    gateway::core::CommandMessage command;
    command.command_id = "cmd-002";
    command.device_id = "device-001";
    command.type = gateway::core::CommandType::read_register;
    command.address = 2;
    command.count = 3;

    const auto ack = dispatcher.dispatch(command);

    EXPECT_EQ(client.last_read_address, 2U);
    EXPECT_EQ(client.last_read_quantity, 3U);
    EXPECT_EQ(ack.status, gateway::core::CommandStatus::success);
    EXPECT_EQ(ack.message, "register read");
}

TEST(CommandDispatcherTest, ReturnsDeviceNotFoundAck) {
    auto config = make_config();
    FakeModbusClient client;
    gateway::runtime::CommandDispatcher dispatcher(config, client);
    gateway::core::CommandMessage command;
    command.command_id = "cmd-003";
    command.device_id = "missing-device";
    command.type = gateway::core::CommandType::write_register;

    const auto ack = dispatcher.dispatch(command);

    EXPECT_EQ(ack.status, gateway::core::CommandStatus::failed);
    EXPECT_EQ(ack.error, gateway::core::ErrorCode::device_not_found);
}

TEST(CommandDispatcherTest, ReturnsWriteFailureAck) {
    auto config = make_config();
    FakeModbusClient client;
    client.write_result = {
        .ok = false,
        .error = gateway::core::ErrorCode::network_error,
    };
    gateway::runtime::CommandDispatcher dispatcher(config, client);
    gateway::core::CommandMessage command;
    command.command_id = "cmd-004";
    command.device_id = "device-001";
    command.type = gateway::core::CommandType::write_register;

    const auto ack = dispatcher.dispatch(command);

    EXPECT_EQ(ack.status, gateway::core::CommandStatus::failed);
    EXPECT_EQ(ack.error, gateway::core::ErrorCode::network_error);
    EXPECT_EQ(ack.message, "write register failed");
}

} // namespace
