#pragma once



#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>


using namespace std;


/**
 * A thread safe queue
 */
template<typename T>
struct ThreadQueue {

    // Add to the queue
    void enqueue(T element) {
        lock_guard<mutex> lock(mutex_); // releases lock when out of scope
        queue_.push(element);
    }

    // Retrieve and remove from the queue
    optional<T> dequeue() {
        unique_lock<mutex> lock(mutex_);
        if (!this->queue_.empty()) {
            T element = queue_.front();
            queue_.pop();
            return element;
        }
        else {
            return {};
        }
    }

    mutex mutex_;
    condition_variable condition_;
    queue<T> queue_;
};