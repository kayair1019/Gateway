#include "gateway/core/config.hpp"
#include "gateway/runtime/gateway_app.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

std::string parse_config_path(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--config") {
            return argv[i + 1];
        }
    }

    return "configs/gateway.json";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto config = gateway::core::load_gateway_config(parse_config_path(argc, argv));
        gateway::runtime::GatewayApp app(config);
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "edge_gateway failed: " << ex.what() << '\n';
        return 1;
    }
}
