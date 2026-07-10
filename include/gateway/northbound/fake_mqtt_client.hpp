#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gateway/northbound/mqtt_client.hpp"

namespace gateway::northbound {

class FakeMqttClient final : public MqttClient {
public:
    struct PublishedMessage {
        std::string topic;
        std::string payload;
    };

    void publish(std::string_view topic, std::string_view payload) override {
        published_messages_.push_back({std::string(topic), std::string(payload)});
    }

    void subscribe(std::string_view topic, MessageHandler handler) override {
        handlers_.emplace(std::string(topic), std::move(handler));
    }

    void inject_message(std::string_view topic, std::string_view payload) {
        const auto iter = handlers_.find(std::string(topic));
        if (iter != handlers_.end()) {
            iter->second(topic, payload);
        }
    }

    auto published_messages() const -> const std::vector<PublishedMessage>& {
        return published_messages_;
    }

private:
    std::vector<PublishedMessage> published_messages_;
    std::unordered_map<std::string, MessageHandler> handlers_;
};

} // namespace gateway::northbound
