#pragma once

#include <mutex>
#include <shared_mutex>


using namespace std;


/**
 * A wrapper around an object that pairs it with a mutex. Functions can be passed in
 * to act on the object with the assurance of being thread safe with 'run_with_lock'.
 * 
 * Using 'run_with_read_lock' should be done with more care, as it assumes the function
 * does not mutate the object at all.
 * 
 * Use 'unsafe()' when you know the operation doesn't need synchronization (e.g. per-client
 * TCP socket operations that use a dedicated socket handle, not the shared server socket).
 */
template<typename T>
struct TSafe {

    shared_mutex mutex;
    T object;

    TSafe() = default;
    TSafe(T&& obj) : object(forward<T>(obj)) {}
    TSafe(const T& obj) : object(obj) {}

    /**
     * Bypass the lock. Use only when you're certain no other thread can interfere
     * (e.g. a per-client socket handle that only one thread touches).
     *
     *   sock.unsafe().receive(client_socket, buffer, size);
     */
    T& unsafe() { return object; }
    const T& unsafe() const { return object; }

    /**
     * Exclusive (write) lock. Pass a lambda that mutates the object.
     *
     *   
     ```
     safe.run_with_lock([](auto& obj) { obj.push_back(42); });
     ```
     */
    template<typename Func>
    decltype(auto) run_with_lock(Func&& func) {
        lock_guard<shared_mutex> lock(mutex);
        return func(object);
    }

    /**
     * Shared (read) lock. Multiple readers can run concurrently. Pass a lambda that
     * just reads from the object, and DOES NOT mutate it in any way.
     *
     ```
     auto size = safe.run_with_read_lock([](auto& obj) { return obj.size(); });
     ```
     */
    template<typename Func>
    decltype(auto) run_with_read_lock(Func&& func) {
        shared_lock<shared_mutex> lock(mutex);
        return func(object);
    }
};
