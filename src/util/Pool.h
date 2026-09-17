#pragma once


#include <deque>
#include <iostream>
#include <stdexcept>
#include <utility>




using namespace std;


/**
 * Data structure that acts like a stack except elements are randomly (but deterministically)
 * stored and removed. (Pool)
 * 
 * So you can put an item in, and get out a random item with O(1) time complexity.
 */
template <typename T>
class Pool
{
public:
	Pool() {};
	~Pool() {};

	// Gets a random item from the pool (deterministically) and removes it from the pool
	T fish(){
		if (this->internal_deque.empty())
			throw std::runtime_error("Pool was empty");

		int current_count = this->random;
		this->random += this->last_random;
		this->last_random = current_count;
		if (this->random > 10000000) {
			this->random = 1;
			this->last_random = 1;
		}

		int n = this->internal_deque.size();
		int w_i = this->random % n;
		shared_ptr<T> ptr = this->internal_deque[w_i];
		if (!ptr)
			throw std::runtime_error("Pool contained an empty item");

		T result = *ptr;
		if (w_i != n - 1)
			this->internal_deque[w_i] = std::move(this->internal_deque[n - 1]);
		this->internal_deque.pop_back();
		return result;
	}

	// Adds an item to the pool
	void add(T item){

		// if (item.first > 10000000) {
		// 	string s = "bad";
		// }
		// cout << "ADD: dist=" << item.first << " deque_size=" << internal_deque.size() << endl;
		if constexpr (std::is_pointer<T>::value) {
			// If T is a pointer type, dereference it to make a copy of the object it points to
			this->internal_deque.push_front(make_unique<remove_pointer_t<T>>(*item));
		} else {
			// If T is not a pointer type, just use the item directly
			this->internal_deque.push_front(make_unique<T>(item));
		}
	}

	// Clears all items in the pool
	void clear(){
		this->internal_deque.clear();
		this->random = 1;
		this->last_random = 1;    
	}

	// check is pool is empty
	bool empty(){
		if (this->internal_deque.empty()) return true;

		for (shared_ptr<T> item : this->internal_deque) {
			if (item) return false;
		} 

		return true;
	}

private:
	int random = 1;
	int last_random = 1;
	deque<shared_ptr<T>> internal_deque;


};
