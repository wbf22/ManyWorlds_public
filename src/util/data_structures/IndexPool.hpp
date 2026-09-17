#pragma once




#include <unordered_map>
#include <vector>
#include <limits> 




using namespace std;


/**
 * A data structure that allows random access, and efficient insert and erase functions.
 */
template <typename T>
class IndexPool {

public:

    IndexPool() {};

    /**
     * Inserts an item into the pool returning a key
     * to be used for random access. (constant time)
     */
    uint64_t insert(T item) {
        // get a key
        while (this->pool.find(this->key_counter) != this->pool.end()) {
            this->key_counter++;
            if (this->key_counter == this->max_key) this->key_counter = 0;
        }

        // add to map
        this->pool[key_counter] = item;

        // add to available_keys
        this->available_keys.push_back(key_counter);
        this->key_to_index[key_counter] = (int) this->available_keys.size() - 1;

        return key_counter;
    }

    /**
     * Removes a key from the pool. (constant time)
     */
    void erase(uint64_t key) {

        // check if key exists
        if (this->pool.find(key) == this->pool.end()) return;

        // remove from pool
        this->pool.erase(key);
        
        // remove from available_keys
        int available_keys_index = this->key_to_index[key];
        this->swap_index_to_back(available_keys_index);
        this->available_keys.pop_back();

        // remove from key_to_index
        this->key_to_index.erase(key);
    }


    /**
     * Returns a random element from the pool using the provided random value
     * as a seed or spike to get an element from the pool. (constant time)
     * 
     * Sets the key of the element returned.
     */
    T random(uint64_t random_value, uint64_t& key) {
        uint64_t index = random_value % this->available_keys.size();
        key = this->available_keys[index];
        return this->pool[key];
    }

    /**
     * Checks whether the pool contains a key. (constant time)
     */
    bool contains(uint64_t key) {
        return this->pool.find(key) != this->pool.end();
    }


    /**
     * Access operator overload to retrieve an element from the pool using the key. (constant time)
     */
    T& operator[](uint64_t key) {
        auto it = this->pool.find(key);
        if (it != this->pool.end()) {
            return it->second;
        } else {
            // Return a sentinel value
            static T sentinel_value = T(); // or some other sentinel value
            return sentinel_value;
        }
    }


private:

    unordered_map<uint64_t, T> pool;
    vector<uint64_t> available_keys;
    unordered_map<uint64_t, int> key_to_index;

    uint64_t key_counter = 0;
    uint64_t max_key = (std::numeric_limits<uint64_t>::max)();


    void swap_index_to_back(int index) {
        // get keys for index and back
        uint64_t back_key = this->available_keys.back();
        uint64_t index_key = this->key_to_index[index];

        // swap key positions in available_keys
        swap(this->available_keys[index], this->available_keys.back());

        // update key_to_index
        this->key_to_index[back_key] = index;
        this->key_to_index[index_key] = (int) this->available_keys.size() - 1;
    }


// allow test methods in 'test.cpp' to access private fields
friend void index_pool_test();


};