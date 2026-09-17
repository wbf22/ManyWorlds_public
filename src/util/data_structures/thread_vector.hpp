#include <vector>
#include <mutex>

/**
 * A thread-safe vector that can be accessed by multiple threads. Uses std::vector
 * and std::mutex to provide thread safety.
 */
template <typename T>
class thread_vector {
public:
    thread_vector() = default;

    thread_vector(int size) {
        vec_.resize(size);
    }


    void push_back(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        vec_.push_back(value);
    }

    T get(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        return vec_.at(index);
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return vec_.size();
    }

    // Override the array access operator for non-const access
    T& operator[](size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        return vec_[index];
    }

    // Override the array access operator for const access
    const T& operator[](size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return vec_[index];
    }


    // Move assignment operator
    thread_vector& operator=(thread_vector&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock_this(mutex_, std::defer_lock);
            std::lock_guard<std::mutex> lock_other(other.mutex_, std::defer_lock);
            std::lock(lock_this, lock_other);
            vec_ = std::move(other.vec_);
        }
        return *this;
    }


    // Disable copy constructor and copy assignment operator
    thread_vector(const thread_vector&) = delete;
    thread_vector& operator=(const thread_vector&) = delete;

private:
    std::vector<T> vec_;
    mutable std::mutex mutex_;
};