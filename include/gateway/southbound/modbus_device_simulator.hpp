#pragma once

namespace gateway::southbound {

class ModbusDeviceSimulator {
public:
    auto run() -> int;
};

} // namespace gateway::southbound
