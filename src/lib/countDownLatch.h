#pragma once

#include <mutex>
#include <condition_variable>
#include "noncopyable.h"

class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count) : count_(count) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return count_ == 0; });
    }

    void countDown() {
        std::lock_guard<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0) {
            condition_.notify_all();
        }
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    int count_;
};
