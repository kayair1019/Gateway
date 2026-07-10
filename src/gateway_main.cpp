#include "gateway/runtime/gateway_app.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        gateway::runtime::GatewayApp app;
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "edge_gateway failed: " << ex.what() << '\n';
        return 1;
    }
}
