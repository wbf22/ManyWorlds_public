#pragma once

#include <any>
#include <unordered_map>
#include <mutex>

using namespace std;


template<typename K, typename T>
class concurrent_map {
private:
    unordered_map<K, T> map;
public:
    recursive_mutex mute;


    void push(K tag, T data) {
        lock_guard<recursive_mutex> lock(mute);
        map[tag] = data;
    }

    T pop(K tag) {
        lock_guard<recursive_mutex> lock(mute);
        auto it = map.find(tag);
        if (it == map.end()) return nullptr;
        auto data = it->second;
        map.erase(it);
        return data;
    }

    int size() {
        lock_guard<recursive_mutex> lock(mute);
        return map.size();
    }

    T& operator[](K tag) {
        lock_guard<recursive_mutex> lock(mute);
        return map[tag];
    }

    T& operator[](K&& tag)
    { 
        lock_guard<recursive_mutex> lock(mute);
        return map[std::move(tag)]; 
    }

    bool erase(K tag) {
        lock_guard<recursive_mutex> lock(mute);
        return map.erase(tag) > 0;
    }

    bool contains(K tag) {
        lock_guard<recursive_mutex> lock(mute);
        return map.find(tag) != map.end();
    }






};