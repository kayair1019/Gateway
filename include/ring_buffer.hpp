#pragma once
#include <atomic>
#include <vector>
#include <cstddef>
#include <stdexcept>

constexpr size_t CACHE_LINE_SIZE = 64;

template <typename T, size_t Capacity>
class SPSCRingBuffer {
    // 编译期断言：容量必须是 2 的整数幂
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    // 容量至少为 2，否则无法通过留空区分空/满
    static_assert(Capacity >= 2, "Capacity must be at least 2");

public:
    SPSCRingBuffer()
        : mask_(Capacity - 1),
          buffer_(Capacity)
    {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    // 生产者调用
    bool push(const T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) & mask_;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // 满
        }

        buffer_[tail & mask_] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // 消费者调用
    bool pop(T& item) {
        size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return false; // 空
        }

        item = buffer_[head & mask_];
        head_.store((head + 1) & mask_, std::memory_order_release);
        return true;
    }

    // 当前元素个数（调试用）
    size_t size() const {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);
        return (tail - head) & mask_;
    }

    // 最大可存储元素个数 = Capacity - 1（始终留一空位区分空和满）
    size_t capacity() const { return mask_; }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_;
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_;

    const size_t mask_;         // =Capacity - 1
    std::vector<T> buffer_;     // 大小 = Capacity
};