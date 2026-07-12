#pragma once

#include <atomic>
#include <cstddef>
#include <array>
#include <utility>  // 新增 std::move

namespace gateway::queue {

constexpr std::size_t kCacheLineSize = 64;

template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static_assert(Capacity >= 2, "Capacity must be at least 2");

public:
    SpscRingBuffer() = default;

    // ================ 方案 B：按值传参 + 移动语义 ================
    // 优势：
    // 1. 接口极简清晰，一看便知“数据会被转移进队列”。
    // 2. 传左值：拷贝1次（构造形参）+ 移动1次（存入buffer）
    // 3. 传右值：移动2次（构造形参 + 存入buffer），远优于深拷贝。
    auto push(T item) -> bool {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = (tail + 1) & mask_;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;  // 队列已满
        }

        // 关键：将形参的资源“偷”进 buffer，避免拷贝
        buffer_[tail & mask_] = std::move(item);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // ================ pop 也升级为移动语义 ================
    // 从槽位中“偷”出数据，让槽位变成空壳（有效但未指定状态）
    auto pop(T& item) -> bool {
        const auto head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return false;  // 队列为空
        }

        // 关键：用移动赋值替代拷贝赋值，把资源转移给调用者
        item = std::move(buffer_[head & mask_]);
        head_.store((head + 1) & mask_, std::memory_order_release);
        return true;
    }

    auto size() const -> std::size_t {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_relaxed);
        return (tail - head) & mask_;
    }

    auto capacity() const -> std::size_t {
        return mask_;  // 由于 1 个槽位用于区分空/满，实际可存元素数 = Capacity - 1
    }

private:
    // 防止伪共享（False Sharing）
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};

    static constexpr std::size_t mask_ = Capacity - 1;
    std::array<T, Capacity> buffer_;
};

} // namespace gateway::queue