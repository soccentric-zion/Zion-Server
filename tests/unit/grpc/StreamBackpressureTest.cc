// tests/unit/grpc/StreamBackpressureTest.cc — verify BoundedQueue semantics.
#include "grpc_server/StreamBackpressure.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using zion::grpc::BoundedQueue;

TEST(BoundedQueueTest, BasicEnqueueDequeue) {
    BoundedQueue<int> q{4};
    EXPECT_TRUE(q.enqueue(1));
    EXPECT_TRUE(q.enqueue(2));
    EXPECT_EQ(*q.dequeue(), 1);
    EXPECT_EQ(*q.dequeue(), 2);
}

TEST(BoundedQueueTest, CloseUnblocksDequeue) {
    BoundedQueue<int> q{4};
    std::thread t{[&] {
        auto v = q.dequeue();
        EXPECT_FALSE(v.has_value());
    }};
    q.close();
    t.join();
}

TEST(BoundedQueueTest, EnqueueAfterCloseFails) {
    BoundedQueue<int> q{4};
    q.close();
    EXPECT_FALSE(q.enqueue(42));
}

TEST(BoundedQueueTest, BackpressureBlocks) {
    BoundedQueue<int> q{2};
    EXPECT_TRUE(q.enqueue(1));
    EXPECT_TRUE(q.enqueue(2));

    std::atomic<bool> enqueued{false};
    std::thread producer{[&] {
        q.enqueue(3);  // Will block until consumer reads.
        enqueued = true;
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    EXPECT_FALSE(enqueued);

    q.dequeue();  // Release one slot.
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    EXPECT_TRUE(enqueued);

    producer.join();
    q.close();
}
