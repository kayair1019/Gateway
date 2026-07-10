#pragma once

#include <functional>
#include <string_view>

namespace gateway::northbound {

class MqttClient {
public:
    using MessageHandler =
        std::function<void(std::string_view topic, std::string_view payload)>;

    virtual ~MqttClient() = default;

    virtual void publish(std::string_view topic, std::string_view payload) = 0;
    virtual void subscribe(std::string_view topic, MessageHandler handler) = 0;
};

} // namespace gateway::northbound
