#pragma once
// grpc_server/StreamBackpressure.h — bounded MPSC queue for gRPC stream handlers.
// If the queue is full, enqueue() blocks until space is available (backpressure).
// The DB writer drains the queue from a separate thread.

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace zion::grpc {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {}

    // Enqueue an item.  Blocks if the queue is at capacity.
    // Returns false if the queue has been closed.
    bool enqueue(T item) {
        std::unique_lock<std::mutex> lock{mutex_};
        not_full_.wait(lock, [this] {
            return closed_ || queue_.size() < capacity_;
        });
        if (closed_) return false;
        queue_.push_back(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Dequeue an item.  Blocks until an item is available or the queue is closed.
    std::optional<T> dequeue() {
        std::unique_lock<std::mutex> lock{mutex_};
        not_empty_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop_front();
        not_full_.notify_one();
        return item;
    }

    // Close the queue.  All blocked dequeue() calls will return nullopt.
    void close() {
        std::lock_guard<std::mutex> lock{mutex_};
        closed_ = true;
        not_full_.notify_all();
        not_empty_.notify_all();
    }

    [[nodiscard]] bool is_closed() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return closed_;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return queue_.size();
    }

private:
    mutable std::mutex      mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T>           queue_;
    std::size_t             capacity_;
    bool                    closed_ = false;
};

}  // namespace zion::grpc
