#include "gateway/runtime/gateway_app.hpp"

#include <iostream>

namespace gateway::runtime {

auto GatewayApp::run() -> int {
    std::cout << "edge_gateway skeleton started\n";
    io_context_.poll();
    return 0;
}

} // namespace gateway::runtime
