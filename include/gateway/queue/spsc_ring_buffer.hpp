#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace gateway::queue {

constexpr std::size_t kCacheLineSize = 64;

template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static_assert(Capacity >= 2, "Capacity must be at least 2");

public:
    SpscRingBuffer() : buffer_(Capacity) {}

    auto push(const T& item) -> bool {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = (tail + 1) & mask_;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[tail & mask_] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    auto pop(T& item) -> bool {
        const auto head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return false;
        }

        item = buffer_[head & mask_];
        head_.store((head + 1) & mask_, std::memory_order_release);
        return true;
    }

    auto size() const -> std::size_t {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_relaxed);
        return (tail - head) & mask_;
    }

    auto capacity() const -> std::size_t {
        return mask_;
    }

private:
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};

    static constexpr std::size_t mask_ = Capacity - 1;
    std::vector<T> buffer_;
};

} // namespace gateway::queue
