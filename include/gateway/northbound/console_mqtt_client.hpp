#pragma once

#include <string>
#include <unordered_map>

#include "gateway/northbound/mqtt_client.hpp"

namespace gateway::northbound {

class ConsoleMqttClient final : public MqttClient {
public:
    void publish(std::string_view topic, std::string_view payload) override;
    void subscribe(std::string_view topic, MessageHandler handler) override;

private:
    std::unordered_map<std::string, MessageHandler> handlers_;
};

} // namespace gateway::northbound
