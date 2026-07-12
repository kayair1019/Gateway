#include "gateway/northbound/console_mqtt_client.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace gateway::northbound {

void ConsoleMqttClient::publish(std::string_view topic, std::string_view payload) {
    /* TODO: 实现该函数 */
}

void ConsoleMqttClient::subscribe(std::string_view topic, MessageHandler handler) {
    /* TODO: 实现该函数 */
}

} // namespace gateway::northbound
