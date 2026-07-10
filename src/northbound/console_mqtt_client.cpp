#include "gateway/northbound/console_mqtt_client.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace gateway::northbound {

void ConsoleMqttClient::publish(std::string_view topic, std::string_view payload) {
    std::cout << "[mqtt publish] topic=" << topic << " payload=" << payload << '\n';
}

void ConsoleMqttClient::subscribe(std::string_view topic, MessageHandler handler) {
    handlers_.emplace(std::string(topic), std::move(handler));
    std::cout << "[mqtt subscribe] topic=" << topic << '\n';
}

} // namespace gateway::northbound
