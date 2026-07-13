#pragma once

#include <asio/io_context.hpp>

#include "gateway/core/config.hpp"

namespace gateway::runtime {

class GatewayApp {
public:
    explicit GatewayApp(gateway::core::GatewayConfig config);

    int run();

private:
    gateway::core::GatewayConfig config_;
    asio::io_context io_context_;
};

} // namespace gateway::runtime
