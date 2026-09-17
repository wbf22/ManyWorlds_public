#pragma once

#include <vector>
#include <mutex>

using namespace std;


template<typename T>
class concurrent_vector {
private:
    vector<T> vec;
public:
    recursive_mutex mute;


    void push_back(T data) {
        lock_guard<recursive_mutex> lock(mute);
        vec.push_back(data);
    }


    int size() {
        lock_guard<recursive_mutex> lock(mute);
        return vec.size();
    }

    T operator[](int index) {
        lock_guard<recursive_mutex> lock(mute);
        return vec[index];
    }


    void erase(T data) {
        lock_guard<recursive_mutex> lock(mute);
        vec.erase(std::remove(vec.begin(), vec.end(), data), vec.end());
    }

    bool empty() {
        lock_guard<recursive_mutex> lock(mute);
        return vec.empty();
    }


    /**
     * UNSAFE! You need to lock the mutex before using this or doing a foreach on this kind of vector
     */
    auto begin() {
        return vec.begin();
    }

    /**
     * UNSAFE! You need to lock the mutex before using this or doing a foreach on this kind of vector
     */
    auto end() {
        return vec.end();
    }
};