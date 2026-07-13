#include "gateway/core/config.hpp"
#include "gateway/simulator/modbus_device_simulator.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

std::string parse_config_path(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--config") {
            return argv[i + 1];
        }
    }

    return "configs/simulator.json";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto config = gateway::core::load_simulator_config(parse_config_path(argc, argv));
        gateway::simulator::ModbusDeviceSimulator simulator(config);
        return simulator.run();
    } catch (const std::exception& ex) {
        std::cerr << "modbus_simulator failed: " << ex.what() << '\n';
        return 1;
    }
}
