#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gateway::protocol {

class ModbusTcpFramer {
public:
    std::vector<std::vector<std::uint8_t>> push_bytes(std::span<const std::uint8_t> bytes);

    std::size_t buffered_size() const;
    bool has_error() const;
    void reset();

private:
    std::vector<std::uint8_t> buffer_;
    bool has_error_{false};
};

} // namespace gateway::protocol
