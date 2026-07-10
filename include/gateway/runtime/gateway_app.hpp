#pragma once

#include <asio/io_context.hpp>

namespace gateway::runtime {

class GatewayApp {
public:
    auto run() -> int;

private:
    asio::io_context io_context_;
};

} // namespace gateway::runtime
