#pragma once



#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>


using namespace std;


/**
 * A thread safe map
 */
template<typename T, typename S>
struct thread_map {
    unordered_map<T, S> map;
    mutex mtx;

    // Insert an element into the map
    void insert(const T& key, const S& value) {
        lock_guard<mutex> lock(mtx);
        map.insert({key, value});
    }

    // Remove an element from the map
    void erase(const T& key) {
        lock_guard<mutex> lock(mtx);
        map.erase(key);
    }

    // Check if the map contains a key
    bool contains(const T& key) {
        lock_guard<mutex> lock(mtx);
        return map.count(key) > 0;
    }

    // Find an element in the map
    S* find(const T& key) {
        lock_guard<mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    S* end() {
        lock_guard<mutex> lock(mtx);
        return map.end();
    }


    // Get the value associated with a key
    S& operator[](const T& key) {
        lock_guard<mutex> lock(mtx);
        return map[key];
    }
};