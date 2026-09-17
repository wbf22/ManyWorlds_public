#pragma once


#include "Position.h"
#include <iostream>


using namespace std;


/**
 * A position map designed to efficiently find nearby blocks
 */
class PositionMap
{
public:
	int size = 0;
	

	PositionMap() : PositionMap(10, 1024, 0) {};

	PositionMap(int resize_size, int max_coordinate, int min_coordinate) 
		: PositionMap(
			resize_size, 
			std::make_shared<PositionDouble>(min_coordinate, min_coordinate, min_coordinate), 
			std::make_shared<PositionDouble>(max_coordinate, max_coordinate, max_coordinate) 
		) {
			int diff = max_coordinate - min_coordinate;

			int pow = 1;
			while (pow < diff) {
				pow *= 2;
			}

			if (diff != pow) {
				cout << "Coordinate range must be a power of 2 (eg 64, 256, 1024, 4096, 16384)" << endl;
				throw std::runtime_error("Coordinate range must be a power of 2 (eg 64, 256, 1024, 4096, 16384)"); 
			}
		};

	PositionMap(int resize_size, shared_ptr<PositionDouble> space_start, shared_ptr<PositionDouble> space_end) {
		this->resize_size = resize_size;
		this->space_start = space_start;
		this->space_end = space_end;
		this->space_size = space_end->x - space_start->x;
	};

	~PositionMap() {};


	void add (shared_ptr<PositionDouble> pos) {

		// check if in space
		if (!is_in_space(pos)) {
			cout << "shared_ptr<PositionDouble> is not in space " + pos->toString() + " " + space_start->toString() + " " + space_end->toString() << endl;
				throw std::runtime_error("shared_ptr<PositionDouble> is not in space " + pos->toString() + " " + space_start->toString() + " " + space_end->toString()); 
		}

		// add to positions 
		if (!this->partitioned) {
			this->positions.push_back(pos);
			
			// resize if necessary
			if (this->positions.size() > resize_size) {
				if (this->can_resize()) {
					resize();
				}
			}
		}
		else {
			// add to submap
			for (shared_ptr<PositionMap>& submap : this->submaps) {
				if (submap->is_in_space(pos)) {
					submap->add(pos);
					break;
				}
			}
		}

		this->size++;
	}

	vector<shared_ptr<PositionDouble>> getNearby(shared_ptr<PositionDouble> pos, int radius, int limit = 10) {
		vector<shared_ptr<PositionDouble>> nearby;

		if (this->partitioned) {
			// get most nearby from assigned submap
			for (shared_ptr<PositionMap>& submap : this->submaps) {
				if (submap->is_in_space(pos)) {
					vector<shared_ptr<PositionDouble>> submap_nearby = submap->getNearby(pos, radius, limit);
					nearby.insert(nearby.end(), submap_nearby.begin(), submap_nearby.end());
				}
			}

			// return if limit reached
			if (nearby.size() >= limit) {
				return nearby;
			}

			// explore other nearby submaps
			int submap_size = (*this->submaps.begin())->space_size;
			for (int x : {pos->x - submap_size, pos->x, pos->x + submap_size}) {
				for (int y : {pos->y - submap_size, pos->y,  pos->y + submap_size}) {
					for (int z : {pos->z - submap_size, pos->z, pos->z + submap_size}) {

						shared_ptr<PositionDouble> neighborPos = std::make_shared<PositionDouble>(x, y, z);
						for (shared_ptr<PositionMap> submap : this->submaps) {
							if (submap->is_in_space(neighborPos)) {
								vector<shared_ptr<PositionDouble>> submap_nearby = submap->getNearby(neighborPos, radius, limit - nearby.size());
								nearby.insert(nearby.end(), submap_nearby.begin(), submap_nearby.end());
							}					
						}

						if (nearby.size() >= limit) {
							return nearby;
						}
					}
				}
			}

		}
		else {
			return this->positions;
		}

		return nearby;
	}

	bool remove(shared_ptr<PositionDouble> pos) {
		bool removed = false;
		if (this->partitioned) {
			for (shared_ptr<PositionMap>& submap : this->submaps) {
				if (submap->is_in_space(pos)) {
					removed = submap->remove(pos);
					break;
				}
			}
		}
		else {
			size_t initial_size = this->positions.size();
			this->positions.erase(
				remove_if(
					this->positions.begin(),
					this->positions.end(),
					[pos](const shared_ptr<PositionDouble>& p) {
						return p->x == pos->x && p->y == pos->y && p->z == pos->z;
					}
				),
				this->positions.end()   // <-- add this
			);
			removed = this->positions.size() < initial_size;
		}

		if (removed) this->size--;
		return removed;
	}

	


private:
	shared_ptr<PositionDouble> space_start;
	shared_ptr<PositionDouble> space_end;
	int space_size;
	int resize_size = 10;
	bool partitioned = false;
	vector<shared_ptr<PositionDouble>> positions;
	vector<shared_ptr<PositionMap>> submaps;

	bool is_in_space(shared_ptr<PositionDouble> pos) {
		return 
			// less than end
			pos->x < space_end->x && 
			pos->y < space_end->y && 
			pos->z < space_end->z &&
			// greater than start
			pos->x >= space_start->x && 
			pos->y >= space_start->y && 
			pos->z >= space_start->z;
	}



	void resize() {
		// get new space sizes
		vector<int> new_sizes = getNewquadrantSizes();		
		int x_step = new_sizes[0];
		int y_step = new_sizes[1];
		int z_step = new_sizes[2];

		// create new submaps
		for (int x = space_start->x; x < space_end->x; x += x_step) {
			for (int y = space_start->y; y < space_end->y; y += y_step) {
				for (int z = space_start->z; z < space_end->z; z += z_step) {
					shared_ptr<PositionMap> submap = std::make_shared<PositionMap>(
						resize_size,
						std::make_shared<PositionDouble>(x, y, z),
						std::make_shared<PositionDouble>(x + x_step, y + y_step, z + z_step)
					);
					this->submaps.push_back(submap);
				}
			}
		}

		// move positions to submaps
		for (shared_ptr<PositionDouble>& pos : this->positions) {
			for (shared_ptr<PositionMap>& submap : this->submaps) {
				if (submap->is_in_space(pos)) {
					submap->add(pos);
					break;
				}
			}
		}

		// set partitioned to true and clear positions
		this->partitioned = true;
		this->positions.clear();
	}

	vector<int> getNewquadrantSizes() {
		int x_step = (space_end->x - space_start->x) / 2;
		int y_step = (space_end->y - space_start->y) / 2;
		int z_step = (space_end->z - space_start->z) / 2;

		return {x_step, y_step, z_step};
	}

	bool can_resize() {
		vector<int> new_sizes = getNewquadrantSizes();
		for (int new_size : new_sizes) {
			if (new_size < 1) return false;
		}
		return true;
	}
};
