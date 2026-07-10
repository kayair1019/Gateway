#include "gateway/queue/spsc_ring_buffer.hpp"

#include <gtest/gtest.h>

namespace {

TEST(SpscRingBufferTest, PushesAndPopsInOrder) {
    gateway::queue::SpscRingBuffer<int, 4> queue;

    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));

    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 2);
}

TEST(SpscRingBufferTest, ReportsFullAndEmpty) {
    gateway::queue::SpscRingBuffer<int, 4> queue;

    EXPECT_EQ(queue.capacity(), 3U);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_TRUE(queue.push(3));
    EXPECT_FALSE(queue.push(4));

    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_TRUE(queue.pop(value));
    EXPECT_TRUE(queue.pop(value));
    EXPECT_FALSE(queue.pop(value));
}

} // namespace
