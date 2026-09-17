#pragma once

#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <vector>

using namespace std;

struct Index3 {
    int64_t x, y, z;

    bool operator==(const Index3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct Index3Hash {
    size_t operator()(Index3 const& i) const noexcept {
        uint64_t h = (static_cast<uint64_t>(i.x) & 0xFFFFFFFF) |
                     ((static_cast<uint64_t>(i.y) & 0xFFFFFFFF) << 32);
        h ^= (static_cast<uint64_t>(i.z) & 0xFFFFFFFF) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return static_cast<size_t>(h);
    }
};

template<typename T>
struct Grid {
    unordered_map<Index3, T, Index3Hash> data;
    int64_t width = 0, height = 0, depth = 0;
    int64_t min_x = 0, min_y = 0, min_z = 0;

    Grid(int64_t width, int64_t height, int64_t depth) {
        this->width = width;
        this->height = height;
        this->depth = depth;
    }

    Grid() {
        this->width = 10;
        this->height = 10;
        this->depth = 10;
    }

    T& operator[](Index3 i) {
        if (i.x < min_x) min_x = i.x;
        if (i.y < min_y) min_y = i.y;
        if (i.z < min_z) min_z = i.z;
        if (i.x >= min_x + width) width = (i.x - min_x) + 1;
        if (i.y >= min_y + height) height = i.y - min_y + 1;
        if (i.z >= min_z + depth) depth = i.z - min_z + 1;
        return data[i];
    }

    const T& operator[](Index3 i) const {
        static const T default_value{};
        auto it = data.find(i);
        if (it != data.end()) return it->second;
        return default_value;
    }

    bool in_bounds(Index3 i) const {
        return i.x >= min_x && i.x < min_x + this->width && 
            i.y >= min_y && i.y < min_y + this->height && 
            i.z >= min_z && i.z < min_z + this->depth;
    }

    bool has(Index3 i) const {
        return data.find(i) != data.end();
    }

    void clear() {
        data.clear();
    }

    size_t size() const {
        return data.size();
    }

    typename unordered_map<Index3, T, Index3Hash>::iterator begin() { return data.begin(); }
    typename unordered_map<Index3, T, Index3Hash>::iterator end() { return data.end(); }
    typename unordered_map<Index3, T, Index3Hash>::const_iterator begin() const { return data.begin(); }
    typename unordered_map<Index3, T, Index3Hash>::const_iterator end() const { return data.end(); }


    vector<T> values() const {
        vector<T> vals;
        for (const auto& pair : this->data) {
            if (pair.second != nullptr)
                vals.push_back(pair.second);
        }
        return vals;
    }
};



template<typename T>
struct ProximityGrid {
    struct Object {
        T obj;
        Index3 real_position;
    };

    vector<int> levels;
    unordered_map<int, unordered_map<Index3, unordered_map<int64_t, shared_ptr<Object>>, Index3Hash>> data;
    explicit ProximityGrid(vector<int> level_sizes) {
        for (int level : level_sizes)
            if (level > 0) levels.push_back(level);
        sort(levels.begin(), levels.end());
        levels.erase(unique(levels.begin(), levels.end()), levels.end());
        for (int level : levels)
            data.emplace(level, unordered_map<Index3, unordered_map<int64_t, shared_ptr<Object>>, Index3Hash>{});
    }

private:
    static int64_t floor_div(int64_t value, int64_t divisor) {
        int64_t quotient = value / divisor;
        return value % divisor < 0 ? quotient - 1 : quotient;
    }

    static Index3 cell_index(Index3 position, int level) {
        return {floor_div(position.x, level), floor_div(position.y, level), floor_div(position.z, level)};
    }

    void remove_id(int64_t id) {
        for (auto& level_data : data) {
            for (auto cell = level_data.second.begin(); cell != level_data.second.end();) {
                cell->second.erase(id);
                if (cell->second.empty()) cell = level_data.second.erase(cell);
                else ++cell;
            }
        }
    }

    shared_ptr<Object> find_object(Index3 position, int64_t id) const {
        for (int level : levels) {
            auto level_it = data.find(level);
            if (level_it == data.end()) continue;
            auto cell_it = level_it->second.find(cell_index(position, level));
            if (cell_it == level_it->second.end()) continue;
            auto object_it = cell_it->second.find(id);
            if (object_it != cell_it->second.end()) return object_it->second;
        }
        return nullptr;
    }

public:
    vector<shared_ptr<Object>> query(Index3 center, int radius) const {
        vector<shared_ptr<Object>> result;
        if (levels.empty()) return result;
        radius = max(0, radius);

        int level = levels.front();
        for (int candidate : levels) {
            if (candidate > radius) break;
            level = candidate;
        }

        Index3 min_cell = cell_index({center.x - radius, center.y - radius, center.z - radius}, level);
        Index3 max_cell = cell_index({center.x + radius, center.y + radius, center.z + radius}, level);
        auto level_it = data.find(level);
        if (level_it == data.end()) return result;

        int64_t radius_squared = static_cast<int64_t>(radius) * radius;
        for (int64_t x = min_cell.x; x <= max_cell.x; ++x)
            for (int64_t y = min_cell.y; y <= max_cell.y; ++y)
                for (int64_t z = min_cell.z; z <= max_cell.z; ++z) {
                    auto cell_it = level_it->second.find({x, y, z});
                    if (cell_it == level_it->second.end()) continue;
                    for (const auto& entry : cell_it->second) {
                        Index3 p = entry.second->real_position;
                        int64_t dx = p.x - center.x;
                        int64_t dy = p.y - center.y;
                        int64_t dz = p.z - center.z;
                        if (dx * dx + dy * dy + dz * dz <= radius_squared)
                            result.push_back(entry.second);
                    }
                }
        return result;
    }

    void update(Index3 old_i, Index3 new_i, int64_t id) {
        shared_ptr<Object> object = find_object(old_i, id);
        if (!object) return;
        T value = object->obj;
        remove_id(id);
        add(new_i, id, value);
    }

    void add(Index3 position, int64_t id, T& object) {
        remove_id(id);
        shared_ptr<Object> stored = make_shared<Object>(Object{object, position});
        for (int level : levels)
            data[level][cell_index(position, level)][id] = stored;
    }

    bool has(Index3 position) const {
        if (levels.empty()) return false;
        auto level_it = data.find(levels.front());
        if (level_it == data.end()) return false;
        auto cell_it = level_it->second.find(cell_index(position, levels.front()));
        if (cell_it == level_it->second.end()) return false;
        for (const auto& entry : cell_it->second)
            if (entry.second->real_position == position) return true;
        return false;
    }

    void clear() {
        data.clear();
    }

    size_t size() const {
        if (levels.empty()) return 0;
        auto level_it = data.find(levels.front());
        if (level_it == data.end()) return 0;
        size_t result = 0;
        for (const auto& cell : level_it->second) result += cell.second.size();
        return result;
    }

    vector<T> values() const {
        vector<T> vals;
        if (levels.empty()) return vals;
        auto level_it = data.find(levels.front());
        if (level_it == data.end()) return vals;
        for (const auto& cell : level_it->second)
            for (const auto& entry : cell.second)
                vals.push_back(entry.second->obj);
        return vals;
    }
};

