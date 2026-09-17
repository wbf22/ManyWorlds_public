#pragma once

#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include "util/StellarCoordinate.hpp"


using namespace std;



template<typename T>
struct SpaceGrid {
    unordered_map<StellarCoordinate, T, StellarCoordinateHash> data;
    StellarCoordinate max;
    StellarCoordinate min;
    bool initialized = false;

    // Returns -1 if a < b, 0 if a == b, 1 if a > b
    static int compare(const StellarCoordinate& a, const StellarCoordinate& b) {
        if (a.quadrant_x != b.quadrant_x) return a.quadrant_x < b.quadrant_x ? -1 : 1;
        if (a.quadrant_y != b.quadrant_y) return a.quadrant_y < b.quadrant_y ? -1 : 1;
        if (a.quadrant_z != b.quadrant_z) return a.quadrant_z < b.quadrant_z ? -1 : 1;
        if (a.light_year_x != b.light_year_x) return a.light_year_x < b.light_year_x ? -1 : 1;
        if (a.light_year_y != b.light_year_y) return a.light_year_y < b.light_year_y ? -1 : 1;
        if (a.light_year_z != b.light_year_z) return a.light_year_z < b.light_year_z ? -1 : 1;
        if (a.km_x != b.km_x) return a.km_x < b.km_x ? -1 : 1;
        if (a.km_y != b.km_y) return a.km_y < b.km_y ? -1 : 1;
        if (a.km_z != b.km_z) return a.km_z < b.km_z ? -1 : 1;
        return 0;
    }

    T& operator[](StellarCoordinate i) {
        if (!initialized) {
            max = i;
            min = i;
            initialized = true;
        } else {
            if (compare(i, max) > 0) max = i;
            if (compare(i, min) < 0) min = i;
        }
        return data[i];
    }

    const T& operator[](StellarCoordinate i) const {
        static const T default_value{};
        auto it = data.find(i);
        if (it != data.end()) return it->second;
        return default_value;
    }

    bool in_bounds(StellarCoordinate i) const {
        if (!initialized) return false;
        return compare(i, min) >= 0 && compare(i, max) <= 0;
    }

    bool has(StellarCoordinate i) const {
        return data.find(i) != data.end();
    }

    void clear() {
        data.clear();
        initialized = false;
    }

    size_t size() const {
        return data.size();
    }

    typename unordered_map<StellarCoordinate, T, StellarCoordinateHash>::iterator begin() { return data.begin(); }
    typename unordered_map<StellarCoordinate, T, StellarCoordinateHash>::iterator end() { return data.end(); }
    typename unordered_map<StellarCoordinate, T, StellarCoordinateHash>::const_iterator begin() const { return data.begin(); }
    typename unordered_map<StellarCoordinate, T, StellarCoordinateHash>::const_iterator end() const { return data.end(); }


    vector<T> values() const {
        vector<T> vals;
        for (const auto& pair : this->data) {
            if (pair.second != nullptr)
                vals.push_back(pair.second);
        }
        sort(vals.begin(), vals.end(), [](const T& a, const T& b) {
            return a.get() < b.get();
        });
        return vals;
    }
};


