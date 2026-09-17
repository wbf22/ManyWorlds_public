#pragma once

#include <unordered_map>
#include <cstdint>
#include <limits>

using namespace std;

/**
 * An infinite expanding 2d array. Elements are set an accessed with
 * my_array[x][y]. Uses a map internally so only set positions are stored.
 */
template<typename T>
struct span_array {

    unordered_map<int64_t, T> values;

    int32_t min_x = numeric_limits<int32_t>::max();
    int32_t max_x = numeric_limits<int32_t>::min();
    int32_t min_y = numeric_limits<int32_t>::max();
    int32_t max_y = numeric_limits<int32_t>::min();

    static inline int64_t key(int32_t x, int32_t y) {
        return (static_cast<int64_t>(x) << 32) | static_cast<int64_t>(static_cast<uint32_t>(y));
    }

    void track(int32_t x, int32_t y) {
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }

    void set(int32_t x, int32_t y, const T& val) {
        track(x, y);
        values[key(x, y)] = val;
    }

    void set(int32_t x, int32_t y, T&& val) {
        track(x, y);
        values[key(x, y)] = move(val);
    }

    T& get(int32_t x, int32_t y) {
        track(x, y);
        return values[key(x, y)];
    }

    const T& get(int32_t x, int32_t y) const {
        return values.at(key(x, y));
    }

    bool contains(int32_t x, int32_t y) const {
        return values.find(key(x, y)) != values.end();
    }

    bool erase(int32_t x, int32_t y) {
        return values.erase(key(x, y)) > 0;
    }

    int64_t size() const {
        return static_cast<int64_t>(values.size());
    }

    bool empty() const {
        return values.empty();
    }

    void clear() {
        values.clear();
        min_x = numeric_limits<int32_t>::max();
        max_x = numeric_limits<int32_t>::min();
        min_y = numeric_limits<int32_t>::max();
        max_y = numeric_limits<int32_t>::min();
    }

    // --- operator[] chain: arr[x][y] = value ---
    // Inner proxy holds a reference back to the parent and the chosen x.
    struct row_proxy {
        span_array<T>& parent;
        int32_t x;

        T& operator[](int32_t y) {
            parent.track(x, y);
            return parent.values[span_array<T>::key(x, y)];
        }
    };

    row_proxy operator[](int32_t x) {
        return row_proxy{ *this, x };
    }

    // Const version — read-only, no tracking needed.
    struct const_row_proxy {
        const span_array<T>& parent;
        int32_t x;

        const T& operator[](int32_t y) const {
            return parent.values.at(span_array<T>::key(x, y));
        }
    };

    const_row_proxy operator[](int32_t x) const {
        return const_row_proxy{ *this, x };
    }
};
