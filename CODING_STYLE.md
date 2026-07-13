# Edge Gateway Coding Style

本文档定义边缘网关项目的 C++ 编码规范、目录规范、测试规范和工程约定。后续代码实现应遵循本文档。

## 1. 基本原则

- 代码优先追求清晰、可测试、可讲解。
- 不为暂时不存在的需求设计复杂抽象。
- 模块边界清楚，业务逻辑尽量放在可单元测试的类或函数中。
- 网络 IO、协议解析、消息转换、规则处理、MQTT 映射分层实现。
- 错误处理要显式，不依赖隐式失败。
- 新增模块必须配套测试。

## 2. C++ 标准

项目使用 C++20。

允许使用：

- `std::span`
- `std::string_view`
- `std::optional`
- `std::variant`
- `std::chrono`
- `std::unique_ptr`
- `std::shared_ptr`
- structured binding
- `if` initializer

谨慎使用：

- template metaprogramming。
- 复杂 SFINAE。
- coroutine。
- concept。
- macro 技巧。

除非有明确收益，否则不要使用复杂 C++20 特性。

## 3. 命名空间

所有项目代码放在 `gateway` 命名空间下。

子模块可以使用二级命名空间：

```cpp
namespace gateway::protocol {
class ModbusTcpCodec {};
}
```

推荐命名空间：

- `gateway::core`
- `gateway::queue`
- `gateway::protocol`
- `gateway::southbound`
- `gateway::northbound`
- `gateway::rule`
- `gateway::runtime`

测试代码可以直接使用被测类型，避免过度封装。

## 4. 文件命名

文件名使用小写字母和下划线。

示例：

```text
modbus_tcp_codec.hpp
modbus_tcp_codec.cpp
rule_engine.hpp
rule_engine.cpp
spsc_ring_buffer.hpp
```

测试文件使用 `test_` 前缀：

```text
test_modbus_tcp_codec.cpp
test_rule_engine.cpp
```

## 5. 类型命名

类型使用 `PascalCase`：

```cpp
class ModbusTcpCodec;
struct TelemetryMessage;
enum class ErrorCode;
```

函数和变量使用 `snake_case`：

```cpp
auto decode_response(std::span<const std::uint8_t> bytes) -> DecodeResult;

std::string device_id;
std::uint16_t transaction_id;
```

常量使用 `kPascalCase`：

```cpp
constexpr std::size_t kCacheLineSize = 64;
constexpr std::uint16_t kModbusProtocolId = 0;
```

私有成员变量使用尾部下划线：

```cpp
class Worker {
public:
    void start();

private:
    std::atomic_bool running_{false};
};
```

## 6. 头文件规范

头文件使用 `#pragma once`。

包含顺序：

1. 当前模块对应头文件。
2. C++ 标准库。
3. 第三方库。
4. 项目内头文件。

示例：

```cpp
#include "gateway/protocol/modbus_tcp_codec.hpp"

#include <cstdint>
#include <span>
#include <vector>

#include <nlohmann/json.hpp>

#include "gateway/core/error.hpp"
```

头文件中尽量减少依赖，能前置声明就前置声明。

不要在头文件中使用：

```cpp
using namespace std;
```

任何源文件也不要使用全局 `using namespace std;`。

## 7. 函数规范

函数应短小，职责单一。

函数参数约定：

- 输入字符串优先用 `std::string_view`。
- 输入字节视图优先用 `std::span<const std::uint8_t>`。
- 需要保存的字符串使用 `std::string`。
- 不拥有对象时使用引用或指针。
- 表达可空返回值时使用 `std::optional`。

示例：

```cpp
DecodeResult decode_response(std::span<const std::uint8_t> bytes);
std::optional<CommandMessage> parse_command(std::string_view payload);
```

## 8. 类设计

类应该有明确职责。

推荐：

- `ModbusTcpCodec` 只做协议编码和解码。
- `RuleEngine` 只做规则判断。
- `MqttMessageMapper` 只做 topic 和 payload 映射。
- `GatewayApp` 只做模块组装和生命周期管理。

避免：

- 一个类同时负责网络、协议、规则和 JSON。
- 为了未来扩展提前设计多层继承。
- 全局单例。

接口类需要虚析构：

```cpp
class MqttClient {
public:
    virtual ~MqttClient() = default;
    virtual void publish(std::string_view topic, std::string_view payload) = 0;
};
```

## 9. 错误处理

协议解析、规则判断、消息映射等核心逻辑优先返回结构化结果。

示例：

```cpp
struct DecodeResult {
    bool ok{false};
    ErrorCode error{ErrorCode::none};
    ModbusResponse response{};
};
```

也可以使用 `std::optional` 表示简单失败，但如果需要错误原因，应使用结果结构。

异常使用约定：

- 配置加载失败可以抛异常。
- 程序入口必须捕获异常并打印明确错误。
- 协议解析失败不使用异常控制流程。
- 队列满不抛异常。
- 命令执行失败不抛到主循环外，必须转换为 `CommandAck`。

## 10. 日志规范

第一阶段可以使用轻量日志封装或标准输出。

日志内容要包含关键上下文：

- `gateway_id`
- `device_id`
- `command_id`
- error code
- topic
- address

示例：

```text
[warn] queue_full device_id=device-001 dropped=1
[error] modbus_timeout device_id=device-001 transaction_id=12
```

不要在核心库中散落大量 `std::cout`。演示输出集中放在 `ConsoleMqttClient` 或应用层。

## 11. JSON 规范

内部模块不直接传递 JSON 字符串。

允许 JSON 出现在：

- 配置加载。
- MQTT payload 序列化。
- MQTT command payload 解析。
- 单元测试断言。

JSON 字段名使用 `snake_case`：

```json
{
  "device_id": "device-001",
  "timestamp_ms": 1720000000000
}
```

解析 JSON 时要校验必填字段，不能默认吞掉明显错误。

## 12. 时间规范

内部时间使用 `std::chrono`。

推荐：

```cpp
std::chrono::system_clock::time_point timestamp;
std::chrono::milliseconds poll_interval;
```

JSON 输出时间使用 Unix timestamp milliseconds：

```json
{
  "timestamp_ms": 1720000000000
}
```

不要在核心逻辑中直接依赖系统当前时间。需要测试时间相关逻辑时，优先传入时间或封装 clock。

## 13. 并发规范

第一阶段线程模型固定为：

- 一个 Asio IO 线程。
- 一个 worker 线程。
- 一个 SPSC telemetry queue。

SPSC 队列使用规则：

- 只能有一个生产者。
- 只能有一个消费者。
- `push` 失败表示队列满。
- `pop` 失败表示队列空。

禁止在多个线程中同时向同一个 SPSC 队列写入。

共享状态优先避免。如果必须共享：

- 使用 `std::atomic` 管理简单状态。
- 使用 `std::mutex` 保护复杂状态。
- 明确所有权和生命周期。

## 14. Asio 使用规范

使用 standalone Asio：

```cpp
#define ASIO_STANDALONE
#include <asio.hpp>
```

建议在 CMake 中统一定义 `ASIO_STANDALONE`，避免每个源文件重复定义。

Asio 相关约定：

- 网络回调中不要执行耗时规则处理。
- 定时轮询使用 `asio::steady_timer`。
- 连接、读写、超时处理集中在 southbound 模块。
- 回调中捕获对象生命周期要清晰，必要时使用 `std::enable_shared_from_this`。
- 关闭流程要取消 timer、关闭 socket、停止 worker。

## 15. Modbus 编码规范

字节处理必须显式处理大小端。

Modbus 使用 big-endian：

```cpp
auto hi = static_cast<std::uint8_t>((value >> 8) & 0xff);
auto lo = static_cast<std::uint8_t>(value & 0xff);
```

不要依赖结构体内存布局直接转换为字节：

```cpp
// 禁止
reinterpret_cast<std::uint8_t*>(&header)
```

解析前必须检查长度。

所有 function code、exception code、MBAP 字段应定义命名常量。

## 16. MQTT 抽象规范

第一阶段不直接依赖真实 MQTT 库。

必须通过接口发布和订阅：

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

实现约定：

- `ConsoleMqttClient` 用于演示。
- `FakeMqttClient` 用于测试。
- 真实 MQTT 客户端作为后续扩展，不影响现有测试。

## 17. 测试规范

测试框架使用 GoogleTest。

每个核心模块都要有测试：

- queue
- protocol
- message
- rule
- mqtt mapper
- command flow

测试命名使用行为描述：

```cpp
TEST(ModbusTcpCodecTest, EncodesReadHoldingRegistersRequest)
TEST(RuleEngineTest, DropsMessageWhenThresholdRuleDoesNotMatch)
```

测试应尽量纯逻辑，不依赖真实网络。

需要网络行为时：

- 优先使用 fake。
- 集成测试可以使用本地 simulator。
- 避免依赖外部服务。

## 18. CMake 规范

推荐目标：

```text
edge_gateway_core
edge_gateway
modbus_simulator
edge_gateway_tests
```

要求：

- 设置 C++20。
- 开启常见编译警告。
- 核心逻辑放入 `edge_gateway_core`。
- 可执行程序只做参数解析和模块组装。
- 测试链接 `edge_gateway_core`。

推荐编译选项：

```cmake
target_compile_features(edge_gateway_core PUBLIC cxx_std_20)
target_compile_options(edge_gateway_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
```

不要把所有代码直接编进 `main.cpp`。

## 19. 格式化规范

建议使用 `clang-format`，风格接近 LLVM，但保持 4 空格缩进。

基本约定：

- 缩进 4 空格。
- 不使用 tab。
- 行宽建议 100 到 120。
- 花括号跟随控制语句同行。
- namespace 结束处加注释。

示例：

```cpp
namespace gateway::rule {

class RuleEngine {
public:
    auto evaluate(const TelemetryMessage& message) const -> RuleResult;
};

} // namespace gateway::rule
```

## 20. 注释规范

优先让代码本身清楚。

应该注释：

- 协议字段含义。
- 非显然的并发内存序。
- 错误处理策略。
- 与 Modbus 规范相关的地址或长度约定。

不应该注释：

- 显而易见的 getter/setter。
- 重复函数名含义。
- 已经能从类型看出的内容。

示例：

```cpp
// The ring buffer keeps one slot empty to distinguish full from empty.
auto capacity() const -> std::size_t;
```

## 21. Git 与提交约定

提交应小而清晰。

推荐提交信息：

```text
Add Modbus TCP request encoding
Add telemetry message JSON mapping
Add SPSC ring buffer tests
```

不要把无关重构、格式化和功能改动混在一个提交中。

## 22. 代码审查自检清单

提交前检查：

- 是否符合 `ARCHITECTURE.md` 的模块边界。
- 是否新增或更新了对应测试。
- 是否能通过 CMake 构建。
- 是否能通过 GoogleTest。
- 是否没有引入不必要的第三方依赖。
- 是否没有把 JSON 字符串作为内部通用消息。
- 是否没有违反 SPSC 单生产者单消费者约束。
- 是否没有在协议解析中做未检查长度的字节访问。
- 是否没有把演示输出散落到核心库。

