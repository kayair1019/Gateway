cmake_minimum_required(VERSION 3.24)

project(edge_gateway LANGUAGES CXX)

option(EGW_BUILD_TESTS "Build edge gateway tests" ON)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(asio nlohmann_json)

add_library(asio_standalone INTERFACE)
target_include_directories(asio_standalone INTERFACE
    ${asio_SOURCE_DIR}/asio/include
)
target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE)

add_library(edge_gateway_core
    src/core/config.cpp
    src/core/error.cpp
    src/core/message.cpp
    src/northbound/console_mqtt_client.cpp
    src/northbound/mqtt_message_mapper.cpp
    src/protocol/modbus_tcp_codec.cpp
    src/rule/rule_engine.cpp
    src/runtime/command_dispatcher.cpp
    src/runtime/gateway_app.cpp
    src/runtime/worker.cpp
    src/southbound/device_poller.cpp
    src/southbound/modbus_point_mapper.cpp
    src/southbound/modbus_tcp_client.cpp
    src/simulator/modbus_device_simulator.cpp
)

target_include_directories(edge_gateway_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(edge_gateway_core PUBLIC
    asio_standalone
    nlohmann_json::nlohmann_json
)

target_compile_features(edge_gateway_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(edge_gateway_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_executable(edge_gateway
    src/gateway_main.cpp
)

target_link_libraries(edge_gateway PRIVATE edge_gateway_core)

add_executable(modbus_simulator
    src/simulator_main.cpp
)

target_link_libraries(modbus_simulator PRIVATE edge_gateway_core)

if(EGW_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(edge_gateway_tests
        tests/test_command_flow.cpp
        tests/test_command_dispatcher.cpp
        tests/test_config.cpp
        tests/test_device_poller.cpp
        tests/test_message_model.cpp
        tests/test_modbus_device_simulator.cpp
        tests/test_modbus_point_mapper.cpp
        tests/test_modbus_tcp_codec.cpp
        tests/test_mqtt_message_mapper.cpp
        tests/test_rule_engine.cpp
        tests/test_spsc_ring_buffer.cpp
    )

    target_link_libraries(edge_gateway_tests PRIVATE
        edge_gateway_core
        GTest::gtest_main
    )

    target_compile_definitions(edge_gateway_tests PRIVATE
        EGW_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    include(GoogleTest)
    gtest_discover_tests(edge_gateway_tests)
endif()
