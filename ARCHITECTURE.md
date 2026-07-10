# Edge Gateway Architecture

本文档描述边缘网关求职项目的目标、范围、模块划分和数据流设计。后续编码以本文档为架构基线，避免项目在实现过程中膨胀成大而全的平台。

## 1. 项目定位

本项目是一个轻量级边缘网关，用于模拟工业现场设备和云端平台之间的数据采集、协议转换、规则过滤、上报和命令下发。

项目重点不是覆盖所有工业协议或云平台能力，而是完整、清晰、可测试地实现一条边缘网关核心链路：

- 南向采集：从设备侧读取原始协议数据。
- 协议解析：将 Modbus TCP 字节流解析为结构化数据。
- 消息统一：将不同南向协议转换为统一内部消息模型。
- 队列解耦：通过 SPSC 队列连接 IO 线程和工作线程。
- 规则处理：对遥测数据做轻量过滤。
- 北向上报：转换为 MQTT 风格的 topic 和 JSON payload。
- 云端命令：接收北向命令并转换为南向 Modbus TCP 操作。
- 命令回执：将命令执行结果返回到北向。

## 2. 技术选型

| 项目 | 选择 |
| --- | --- |
| 语言标准 | C++20 |
| 构建系统 | CMake |
| 网络库 | standalone Asio |
| JSON | nlohmann/json |
| 单元测试 | GoogleTest |
| 南向协议 | Modbus TCP |
| 北向协议 | MQTT 抽象接口，第一阶段使用 Console/Fake 实现 |
| 运行环境 | WSL/Linux |

### 2.1 选择 C++20 的原因

项目可以使用 C++17 完成，但协议解析模块经常需要处理连续字节视图。C++20 的 `std::span` 能表达“不拥有内存的连续视图”，适合作为 codec 的输入参数。

因此项目采用 C++20，主要收益是：

- 使用 `std::span<const std::uint8_t>` 表达只读字节视图。
- 减少不必要的 `std::vector` 拷贝。
- 让协议解析接口更清晰。

本项目不使用复杂的 C++20 特性，例如 coroutine、concept、module。保持代码易读、易面试讲解。

## 3. 非目标

以下能力不是第一阶段目标：

- 多协议完整插件系统。
- 复杂规则 DSL 或脚本引擎。
- Web 管理页面。
- 数据库持久化。
- 设备影子。
- TLS、证书、认证鉴权。
- OTA。
- 集群、高可用、多租户。
- 完整生产级 MQTT 客户端实现。

这些能力可以在文档中作为后续扩展方向，但不进入 MVP。

## 4. 总体架构

```text
+-----------------------------+
|        Application          |
|  config / startup / wiring  |
+-------------+---------------+
              |
+-------------v---------------+
|       Gateway Runtime       |
| asio io_context / threads   |
+------+----------------------+
       |
       | southbound telemetry
       v
+------+----------------------+
|      Southbound Adapter     |
| tcp session / polling       |
+------+----------------------+
       |
       | raw bytes
       v
+------+----------------------+
|       Modbus TCP Codec      |
| encode request / decode rsp |
+------+----------------------+
       |
       | structured values
       v
+------+----------------------+
|       Message Model         |
| Telemetry / Command / Ack   |
+------+----------------------+
       |
       | TelemetryMessage
       v
+------+----------------------+
|       SPSC Queue            |
| IO thread -> worker thread  |
+------+----------------------+
       |
       v
+------+----------------------+
|       Worker Thread         |
| rule engine / mqtt mapping  |
+------+----------------------+
       |
       | topic + JSON payload
       v
+------+----------------------+
|       MqttClient            |
| Console / Fake first        |
+-----------------------------+
```

北向命令链路与遥测链路方向相反：

```text
MqttClient
    |
    | command topic + JSON payload
    v
Command Parser
    |
    | CommandMessage
    v
Command Dispatcher
    |
    | route by device_id
    v
Modbus TCP Codec
    |
    | write/read request bytes
    v
Southbound Adapter
    |
    | response bytes
    v
CommandAck
    |
    | ack topic + JSON payload
    v
MqttClient
```

## 5. 推荐目录结构

```text
.
├── CMakeLists.txt
├── ARCHITECTURE.md
├── CODING_STYLE.md
├── configs/
│   ├── gateway.json
│   └── simulator.json
├── include/
│   └── gateway/
│       ├── core/
│       │   ├── config.hpp
│       │   ├── error.hpp
│       │   ├── logger.hpp
│       │   └── message.hpp
│       ├── northbound/
│       │   ├── console_mqtt_client.hpp
│       │   ├── fake_mqtt_client.hpp
│       │   ├── mqtt_client.hpp
│       │   └── mqtt_message_mapper.hpp
│       ├── protocol/
│       │   ├── modbus_tcp_codec.hpp
│       │   └── protocol_codec.hpp
│       ├── queue/
│       │   └── spsc_ring_buffer.hpp
│       ├── rule/
│       │   ├── rule.hpp
│       │   └── rule_engine.hpp
│       ├── runtime/
│       │   ├── gateway_app.hpp
│       │   └── worker.hpp
│       └── southbound/
│           ├── device_session.hpp
│           ├── modbus_device_simulator.hpp
│           └── modbus_tcp_client.hpp
├── src/
│   ├── core/
│   ├── northbound/
│   ├── protocol/
│   ├── rule/
│   ├── runtime/
│   ├── southbound/
│   ├── gateway_main.cpp
│   └── simulator_main.cpp
└── tests/
    ├── test_command_flow.cpp
    ├── test_message_model.cpp
    ├── test_modbus_tcp_codec.cpp
    ├── test_mqtt_message_mapper.cpp
    ├── test_rule_engine.cpp
    └── test_spsc_ring_buffer.cpp
```

现有的 `include/ring_buffer.hpp` 后续应迁移到：

```text
include/gateway/queue/spsc_ring_buffer.hpp
```

## 6. 核心模块

### 6.1 Application

职责：

- 加载配置。
- 初始化日志。
- 创建 `asio::io_context`。
- 组装 southbound、rule、queue、northbound 等模块。
- 启动和停止网关。

主程序应尽量薄：

```cpp
int main(int argc, char** argv) {
    auto config = gateway::load_config(argc, argv);
    gateway::GatewayApp app(config);
    return app.run();
}
```

### 6.2 Gateway Runtime

职责：

- 管理 IO 线程和 worker 线程。
- 管理模块生命周期。
- 处理优雅退出。
- 定义遥测队列和命令处理入口。

第一阶段线程模型：

- 一个 Asio IO 线程。
- 一个 worker 线程。
- 一个 SPSC telemetry queue。

SPSC 队列要求单生产者、单消费者。第一阶段所有遥测消息都由 IO 线程生产，worker 线程消费。

### 6.3 Southbound Adapter

职责：

- 维护设备 TCP 连接。
- 定时轮询设备寄存器。
- 发送 Modbus TCP 请求。
- 接收响应字节流。
- 处理超时和连接错误。

南向适配器不负责业务规则，不直接生成 MQTT payload。

### 6.4 Protocol Codec

职责：

- 编码 Modbus TCP 请求。
- 解码 Modbus TCP 响应。
- 校验 MBAP header、transaction id、function code、payload 长度。
- 将协议错误转换成统一错误码。

第一阶段支持：

- `0x03` Read Holding Registers。
- `0x06` Write Single Register。

可选第二阶段支持：

- `0x10` Write Multiple Registers。

### 6.5 Message Model

内部模块之间传递强类型消息，不直接传递 JSON 字符串。

核心消息类型：

- `TelemetryMessage`：南向采集得到的遥测数据。
- `PointValue`：单个点位值。
- `CommandMessage`：北向下发的命令。
- `CommandAck`：命令执行结果。

JSON 只在模块边界使用：

- telemetry 发布到 MQTT 前序列化为 JSON。
- command 从 MQTT 收到后从 JSON 解析。
- command ack 发布前序列化为 JSON。

### 6.6 SPSC Queue

职责：

- 解耦 IO 线程和 worker 线程。
- 提供无锁的 `push` / `pop`。
- 队列满时返回失败，由调用方决定丢弃、计数或记录日志。

第一阶段策略：

- 遥测队列满时丢弃当前遥测消息。
- 记录 `queue_full` 日志。
- 增加丢弃计数，便于后续观测。

### 6.7 Rule Engine

职责：

- 对 `TelemetryMessage` 执行轻量规则过滤。
- 输出保留或丢弃结果。
- 后续可扩展为字段筛选或死区过滤。

第一阶段只实现阈值规则：

```text
point operator value action
```

示例：

```json
{
  "name": "temperature_limit",
  "point": "temperature",
  "operator": "<",
  "value": 80.0,
  "action": "allow"
}
```

默认策略建议：

- 如果没有配置规则，则允许上报。
- 如果配置了规则但消息不匹配任何规则，则丢弃。

这个策略便于展示规则确实生效，同时避免无规则时网关无法工作。

### 6.8 Northbound Adapter

第一阶段定义 MQTT 抽象接口，不直接绑定真实 MQTT 库：

```cpp
class MqttClient {
public:
    using MessageHandler =
        std::function<void(std::string_view topic, std::string_view payload)>;

    virtual ~MqttClient() = default;

    virtual void publish(std::string_view topic, std::string_view payload) = 0;
    virtual void subscribe(std::string_view topic, MessageHandler handler) = 0;
};
```

第一阶段实现：

- `ConsoleMqttClient`：将 publish 内容打印到标准输出，便于 WSL 演示。
- `FakeMqttClient`：测试使用，记录发布消息并可注入订阅消息。

后续扩展：

- `PahoMqttClient` 或 `MosquittoMqttClient`。
- 连接真实本地 Mosquitto broker。

### 6.9 MQTT Message Mapper

职责：

- 将 `TelemetryMessage` 转为 MQTT topic 和 JSON payload。
- 将 command JSON 解析为 `CommandMessage`。
- 将 `CommandAck` 转为 MQTT topic 和 JSON payload。

推荐 topic：

```text
edge/{gateway_id}/telemetry/{device_id}
edge/{gateway_id}/command/{device_id}
edge/{gateway_id}/command_ack/{device_id}
```

## 7. 南到北数据流

```text
Modbus TCP Device / Simulator
        |
        | raw bytes over TCP
        v
Southbound Adapter
        |
        | Modbus TCP frame
        v
ModbusTcpCodec
        |
        | decoded register values
        v
TelemetryMessage
        |
        v
SPSC Queue
        |
        v
Worker Thread
        |
        v
RuleEngine
        |
        | allow / drop
        v
MqttMessageMapper
        |
        | topic + JSON payload
        v
MqttClient
```

遥测 JSON 示例：

```json
{
  "device_id": "device-001",
  "protocol": "modbus_tcp",
  "timestamp_ms": 1720000000000,
  "points": [
    {
      "name": "temperature",
      "address": 0,
      "value": 25.6,
      "quality": "good"
    }
  ]
}
```

## 8. 北到南数据流

```text
MqttClient
        |
        | command topic + JSON payload
        v
Command Parser
        |
        | CommandMessage
        v
Command Dispatcher
        |
        | route by device_id
        v
ModbusTcpCodec
        |
        | Modbus TCP request bytes
        v
Southbound Adapter
        |
        | device response bytes
        v
CommandAck
        |
        | ack topic + JSON payload
        v
MqttClient
```

第一阶段支持两类命令：

- `read_register`：读取保持寄存器，对应 function code `0x03`。
- `write_register`：写单个保持寄存器，对应 function code `0x06`。

写命令示例：

```json
{
  "command_id": "cmd-001",
  "device_id": "device-001",
  "type": "write_register",
  "address": 0,
  "value": 123
}
```

读命令示例：

```json
{
  "command_id": "cmd-002",
  "device_id": "device-001",
  "type": "read_register",
  "address": 0,
  "count": 2
}
```

命令成功回执：

```json
{
  "command_id": "cmd-001",
  "device_id": "device-001",
  "status": "success",
  "message": "register written",
  "timestamp_ms": 1720000001000
}
```

命令失败回执：

```json
{
  "command_id": "cmd-001",
  "device_id": "device-001",
  "status": "failed",
  "error_code": "device_timeout",
  "message": "modbus response timeout",
  "timestamp_ms": 1720000001000
}
```

## 9. Modbus TCP 范围

Modbus TCP frame：

```text
MBAP Header:
  Transaction ID: 2 bytes
  Protocol ID:    2 bytes
  Length:         2 bytes
  Unit ID:        1 byte

PDU:
  Function Code:  1 byte
  Data:           N bytes
```

寄存器地址约定：

- 配置文件中的 `address` 使用 Modbus PDU 偏移地址。
- 例如传统文档中的 `40001` 在配置中写为 `0`。

Codec 必须校验：

- MBAP 长度。
- Protocol ID 是否为 `0`。
- transaction id 是否匹配请求。
- unit id 是否匹配设备配置。
- function code 是否符合预期。
- exception response。
- payload 长度是否足够。

## 10. 配置文件

示例：

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
  "northbound": {
    "mqtt": {
      "host": "127.0.0.1",
      "port": 1883,
      "client_id": "edge-gateway-001"
    }
  },
  "rules": [
    {
      "name": "temperature_limit",
      "point": "temperature",
      "operator": "<",
      "value": 80.0,
      "action": "allow"
    }
  ],
  "queue": {
    "telemetry_capacity": 1024
  }
}
```

`northbound.mqtt` 第一阶段只保留配置字段，不要求真实连接 broker。

## 11. 错误模型

统一错误码建议：

```cpp
enum class ErrorCode {
    none,
    invalid_config,
    network_error,
    protocol_error,
    device_timeout,
    queue_full,
    mqtt_publish_failed,
    command_not_supported,
    device_not_found
};
```

错误处理原则：

- codec 返回结构化错误，不抛出异常表示协议失败。
- 配置加载失败可以抛异常或返回错误，但必须在应用入口转换为明确日志。
- 队列满不崩溃，记录日志并丢弃当前遥测。
- 命令失败必须生成 `CommandAck`。

## 12. 可执行程序

第一阶段建议提供两个可执行程序：

```text
edge_gateway
modbus_simulator
```

示例运行方式：

```bash
./modbus_simulator --config configs/simulator.json
./edge_gateway --config configs/gateway.json
```

模拟器默认监听：

```text
127.0.0.1:1502
```

不使用标准 `502` 端口，避免 WSL/Linux 下普通用户绑定特权端口的问题。

## 13. 测试策略

每个核心模块必须有单元测试。测试优先覆盖纯逻辑模块，网络模块通过 fake 或 simulator 做集成测试。

推荐测试文件：

```text
tests/test_spsc_ring_buffer.cpp
tests/test_modbus_tcp_codec.cpp
tests/test_message_model.cpp
tests/test_rule_engine.cpp
tests/test_mqtt_message_mapper.cpp
tests/test_command_flow.cpp
```

测试重点：

- SPSC 队列顺序、空队列、满队列、容量。
- Modbus TCP 编码和解码。
- 协议错误和异常响应。
- 遥测消息 JSON 序列化。
- 命令 JSON 解析。
- 规则引擎 allow/drop。
- MQTT topic 和 payload 映射。
- 北向命令到 Modbus 请求再到 command ack 的闭环。

## 14. MVP 验收标准

第一阶段完成时，项目应满足：

- 可以通过 CMake 配置、构建、运行测试。
- `modbus_simulator` 可以在 WSL 本地启动。
- `edge_gateway` 可以定时读取模拟设备寄存器。
- 遥测数据转换为 `TelemetryMessage`。
- 遥测消息通过 SPSC 队列进入 worker 线程。
- 规则引擎可以过滤遥测消息。
- `ConsoleMqttClient` 可以打印 MQTT topic 和 JSON payload。
- 可以注入一条 `write_register` 命令。
- 网关可以执行命令并生成 `CommandAck`。
- 每个核心模块都有 GoogleTest 单元测试。

## 15. 后续扩展方向

MVP 完成后可以考虑：

- 接入真实 Mosquitto broker。
- 增加 Paho 或 Mosquitto MQTT 客户端实现。
- 支持 Modbus TCP `0x10` 写多个寄存器。
- 支持多个设备轮询。
- 增加死区过滤规则。
- 增加简单运行指标，例如上报数量、丢弃数量、命令成功率。
- 增加结构化日志。

