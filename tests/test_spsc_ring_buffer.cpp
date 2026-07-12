#include "gateway/queue/spsc_ring_buffer.hpp"
#include <gtest/gtest.h>

namespace {

//测试1：验证 FIFO 行为
TEST(SpscRingBufferTest, MaintainsFifoOrder) {
    gateway::queue::SpscRingBuffer<int, 4> buffer; // 实际容量 3

    EXPECT_TRUE(buffer.push(10));
    EXPECT_TRUE(buffer.push(20));

    int item = 0;
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 10);
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 20);

}

// 测试2： 验证满/空状态
TEST(SpscRingBufferTest, ReportsFullAndEmptyCorrectly) {
    gateway::queue::SpscRingBuffer<int, 4> buffer; // 实际容量 3

    int item = 0;
    EXPECT_FALSE(buffer.pop(item));

    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));
    EXPECT_FALSE(buffer.push(4)); // 第 4 个推不进去，说明满了

    EXPECT_TRUE(buffer.pop(item));
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_FALSE(buffer.pop(item)); // 第 4 次弹不出来，说明空了
}

// 测试3：只测“容量”边界（确认最大存储量是 Capacity - 1）
TEST(SpscRingBufferTest, CapacityEqualsMask) {
    gateway::queue::SpscRingBuffer<int, 8> buffer; // Capacity=8, mask_=7

    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(buffer.push(i));
    }
    // 第 8 个应该失败
    EXPECT_FALSE(buffer.push(999));

    EXPECT_EQ(buffer.capacity(), 7U); // 确认 capacity 返回的是 mask_
}

// 测试4：专门验证环形回绕（tail/head 绕回 0 后，索引映射是否正确）
TEST(SpscRingBufferTest, HandlesWrapAroundCorrectly) {
    gateway::queue::SpscRingBuffer<int, 4> buffer; // 实际容量 3

    // 1. 填满 (存 1, 2, 3)
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));

    // 2. 弹出 2 个 (取 1, 2，此时 head 移动，腾出空间)
    int item = 0;
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 1);
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 2);

    // 3. 再推入 2 个 (此时 tail 会绕回数组开头，覆盖旧位置)
    EXPECT_TRUE(buffer.push(4));
    EXPECT_TRUE(buffer.push(5));

    // 4. 关键验证：弹出的顺序必须是 3, 4, 5 (不能丢 3，也不能把 3 覆盖)
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 3);
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 4);
    EXPECT_TRUE(buffer.pop(item));
    EXPECT_EQ(item, 5);

    // 此时队列应该彻底为空
    EXPECT_FALSE(buffer.pop(item));
}

} // namespace