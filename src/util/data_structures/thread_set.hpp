#pragma once



#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <functional>


using namespace std;


/**
 * A thread safe map
 */
template<typename T>
struct thread_set {
    unordered_set<T> set;
    mutex mtx;

    // Insert an element into the map
    void insert(const T& key, const S& value) {
        lock_guard<mutex> lock(mtx);
        set.insert({key, value});
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

    // Get the value associated with a key
    S& operator[](const T& key) {
        lock_guard<mutex> lock(mtx);
        return map[key];
    }

    
};