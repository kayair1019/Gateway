# Edge Gateway

一个基于 C++20、CMake 和 standalone Asio 的轻量级边缘网关项目。项目模拟工业现场设备到云端平台之间的数据采集、规则过滤、上报和命令下发流程。

当前项目不是生产级网关平台，而是聚焦边缘网关最核心的工程链路：

```text
Modbus TCP Device / Simulator
  -> Southbound ModbusTcpClient
  -> DevicePoller
  -> TelemetryMessage
  -> SPSC Queue
  -> Worker Thread
  -> RuleEngine
  -> MQTT-style Console Publish
```

北到南命令链路：

```text
stdin JSON command payload
  -> MqttMessageMapper::parse_command
  -> CommandDispatcher
  -> ModbusTcpClient
  -> CommandAck
  -> MQTT-style Console Publish
```

## Features

- C++20 工程，使用 CMake 构建。
- 使用 standalone Asio 实现 TCP client/server。
- 支持 Modbus TCP：
  - `0x03` Read Holding Registers
  - `0x06` Write Single Register
- 内置 Modbus TCP 设备模拟器，便于 WSL 本地演示。
- 使用统一内部消息模型：
  - `TelemetryMessage`
  - `CommandMessage`
  - `CommandAck`
- 使用 SPSC ring buffer 解耦南向轮询和 worker 处理。
- 使用规则引擎做遥测过滤。
- 使用 Console/Fake MQTT Client 抽象北向发布，后续可替换为真实 MQTT client。
- 支持 stdin 输入 JSON command，模拟云端 MQTT command 下发。
- 实现 Modbus TCP 半包/粘包拆帧器，并用单元测试覆盖。
- 使用 GoogleTest 覆盖核心模块。

## Current Scope

当前 MVP 支持：

- 单个 Modbus TCP 设备。
- 常驻轮询第一个配置设备。
- 每次读取配置点位覆盖的 holding register 地址范围。
- 将寄存器值按 `scale` 映射为 `TelemetryMessage`。
- 将 telemetry 转换为 MQTT 风格 topic 和 JSON payload。
- 从 stdin 输入 command JSON，执行读/写寄存器命令，并发布 command ack。

当前暂不支持：

- 真实 MQTT broker 连接。
- 多设备并发轮询。
- 断线重连。
- socket 超时控制。
- TLS/认证/设备管理/数据库持久化。
- Web 管理页面。

这些能力属于后续扩展方向。

## Architecture

```text
+-----------------------------+
|        edge_gateway         |
+-------------+---------------+
              |
              v
+-------------+---------------+
|       Gateway Runtime       |
| poll loop / worker thread   |
+------+----------------------+
       |
       | read holding registers
       v
+------+----------------------+
|      Southbound Layer       |
| ModbusTcpClient/DevicePoller|
+------+----------------------+
       |
       | complete Modbus frame
       v
+------+----------------------+
|       Protocol Layer        |
| codec / framer              |
+------+----------------------+
       |
       | TelemetryMessage
       v
+------+----------------------+
|        SPSC Queue           |
+------+----------------------+
       |
       v
+------+----------------------+
|         Worker              |
| rule / mqtt mapping         |
+------+----------------------+
       |
       | topic + JSON payload
       v
+------+----------------------+
|      ConsoleMqttClient      |
+-----------------------------+
```

本地演示时，另一个进程运行模拟设备：

```text
modbus_simulator
  -> listens on 127.0.0.1:1502
  -> handles 0x03 and 0x06
```

## Directory Layout

```text
include/gateway/
  core/          config, error, message model
  northbound/    mqtt abstraction and message mapper
  protocol/      Modbus TCP codec and stream framer
  queue/         SPSC ring buffer
  rule/          rule engine
  runtime/       GatewayApp, Worker, CommandDispatcher
  simulator/     local Modbus TCP device simulator
  southbound/    ModbusTcpClient, DevicePoller, point mapper

src/
  gateway_main.cpp
  simulator_main.cpp
  ...

tests/
  GoogleTest unit tests

configs/
  gateway.json
  simulator.json
```

## Dependencies

CMake 使用 `FetchContent` 获取：

- standalone Asio
- nlohmann/json
- GoogleTest

需要本机具备：

- CMake 3.24+
- C++20 编译器，例如 GCC 13+
- 可访问 GitHub 的网络环境，首次配置时用于下载依赖

## Build

```bash
cmake -S . -B build -DEGW_BUILD_TESTS=ON
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

当前测试覆盖：

- SPSC ring buffer
- Modbus TCP codec
- Modbus TCP framer 半包/粘包
- config loader
- message model
- rule engine
- MQTT message mapper
- device poller
- command dispatcher
- Modbus device simulator
- worker command flow

## Run Demo

打开第一个终端，启动 Modbus TCP 设备模拟器：

```bash
build/modbus_simulator --config configs/simulator.json
```

默认监听：

```text
127.0.0.1:1502
```

打开第二个终端，启动边缘网关：

```bash
build/edge_gateway --config configs/gateway.json
```

示例输出：

```text
edge_gateway started gateway_id=gateway-001
enter command JSON on stdin to simulate MQTT command payload
[mqtt publish] topic=edge/gateway-001/telemetry/device-001 payload={"device_id":"device-001","points":[{"address":0,"name":"temperature","quality":"good","value":25.6},{"address":1,"name":"pressure","quality":"good","value":12.34}],"protocol":"modbus_tcp","timestamp_ms":...}
```

`configs/gateway.json` 中配置的轮询周期为：

```json
"poll_interval_ms": 1000
```

所以 gateway 会大约每秒发布一次 telemetry。

按 `Ctrl+C` 可以优雅退出：

```text
^Cedge_gateway stopping
edge_gateway stopped
```

## Command Demo

`edge_gateway` 运行时可以在 stdin 输入一行 JSON，用来模拟云端 MQTT command payload。

写单个保持寄存器：

```json
{"command_id":"cmd-001","device_id":"device-001","type":"write_register","address":1,"value":4321}
```

成功时会输出 command ack：

```text
[mqtt publish] topic=edge/gateway-001/command_ack/device-001 payload={"command_id":"cmd-001","device_id":"device-001","message":"register written","status":"success","timestamp_ms":...}
```

下一次 telemetry 中，`pressure` 会因为 address `1` 的寄存器被写为 `4321` 而变为：

```text
4321 * 0.01 = 43.21
```

读保持寄存器：

```json
{"command_id":"cmd-002","device_id":"device-001","type":"read_register","address":0,"count":2}
```

当前 read command 的 ack 只表示读取成功，不携带读取值。读取值仍通过 telemetry 链路展示。

## Configuration

Gateway 配置示例：

```json
{
  "gateway_id": "gateway-001",
  "southbound": {
    "devices": [
      {
        "device_id": "device-001",
        "protocol": "modbus_tcp",
        "host": "127.0.0.1",
        "port": 1502,
        "unit_id": 1,
        "poll_interval_ms": 1000,
        "points": [
          {
            "name": "temperature",
            "address": 0,
            "type": "uint16",
            "scale": 0.1
          },
          {
            "name": "pressure",
            "address": 1,
            "type": "uint16",
            "scale": 0.01
          }
        ]
      }
    ]
  },
  "rules": [
    {
      "name": "temperature_limit",
      "point": "temperature",
      "operator": "<",
      "value": 80.0,
      "action": "allow"
    }
  ]
}
```

Modbus 地址约定：

- 配置中的 `address` 使用 Modbus PDU 偏移地址。
- 传统文档中的 `40001` 对应本项目配置里的 `address = 0`。

## Modbus TCP Framing

TCP 是字节流，不保证一次 read 对应一个完整协议帧。项目中提供 `ModbusTcpFramer` 用于处理：

- 半包
- 粘包
- 多个完整帧一次到达
- 完整帧后跟随下一个半包
- 异常 MBAP length

当前同步 `ModbusTcpClient` 和 simulator 使用 `asio::read(header)` + `asio::read(body)` 精确读取当前帧，已经能覆盖当前同步场景。`ModbusTcpFramer` 为后续改造为 `async_read_some` 的流式会话模型提供基础。

## Resume Highlights

这个项目适合在简历中概括为：

```text
基于 C++20/Asio 实现轻量级边缘网关，支持 Modbus TCP 采集、规则过滤、SPSC 队列解耦、MQTT 风格上报和命令回执；实现本地 Modbus 设备模拟器、TCP 半包/粘包拆帧器，并使用 GoogleTest 覆盖核心模块。
```

可以展开的技术点：

- C++20 / CMake 工程组织
- Asio TCP client/server
- Modbus TCP MBAP Header 编解码
- TCP 半包/粘包处理
- SPSC ring buffer
- Worker thread
- JSON message model
- 规则引擎
- GoogleTest 单元测试
- WSL 本地端到端演示

## Roadmap

后续可以继续完善：

- 接入真实 MQTT broker，例如 Mosquitto/Paho。
- 将 `ModbusTcpClient` 改造为异步连接和 `async_read_some + ModbusTcpFramer`。
- 增加 socket 超时、断线重连和失败统计。
- 支持多个设备轮询。
- 支持更多 Modbus function code。
- 增加集成测试脚本。
- 增加 clang-format、clang-tidy、sanitizer 或 CI。

