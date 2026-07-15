#include "gateway/protocol/modbus_tcp_framer.hpp"

#include <algorithm>

namespace gateway::protocol {
namespace {

constexpr std::size_t kMbapHeaderSize = 7;
constexpr std::uint16_t kMinMbapLength = 2;
constexpr std::uint16_t kMaxMbapLength = 254;

std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8) |
                                      static_cast<std::uint16_t>(bytes[offset + 1]));
}

} // namespace

std::vector<std::vector<std::uint8_t>> ModbusTcpFramer::push_bytes(
    std::span<const std::uint8_t> bytes) {
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());

    std::vector<std::vector<std::uint8_t>> frames;
    while (buffer_.size() >= kMbapHeaderSize) {
        const auto length = read_u16(buffer_, 4);
        if (length < kMinMbapLength || length > kMaxMbapLength) {
            has_error_ = true;
            buffer_.clear();
            break;
        }

        const auto frame_size = kMbapHeaderSize + length - 1;
        if (buffer_.size() < frame_size) {
            break;
        }

        frames.emplace_back(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size));
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size));
    }

    return frames;
}

std::size_t ModbusTcpFramer::buffered_size() const {
    return buffer_.size();
}

bool ModbusTcpFramer::has_error() const {
    return has_error_;
}

void ModbusTcpFramer::reset() {
    buffer_.clear();
    has_error_ = false;
}

} // namespace gateway::protocol
