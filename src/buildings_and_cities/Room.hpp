#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>

#include "util/Position.h"
#include "styles/Styles.hpp"


using namespace std;



enum class RoomPattern {
    OPEN_ROOM,
    HORIZONTAL_HALLWAY,
    VERTICAL_HALLWAY,
    CROSS_HALLWAY,
    BACK_FRONT_LEFT_HALLWAY,
    BACK_FRONT_RIGHT_HALLWAY,
    LEFT_RIGHT_BACK_HALLWAY,
    LEFT_RIGHT_FRONT_HALLWAY,
    LEFT_BACK_HALLWAY,
    RIGHT_BACK_HALLWAY,
    RIGHT_FRONT_HALLWAY,
    LEFT_FRONT_HALLWAY,
    HALLWAY,
    NONE
};


enum class RoomType {
    HALLWAY,
    KITCHEN,
    DINING,
    BEDROOM,
    MEETING_AREA,
    OFFICE,
    WHAREHOUSE,
    BATHROOM,
    VEHICLE_AREA,
    FUN,
    EMPTY,
    OTHER
};


struct Room {
    shared_ptr<Position> start;
    shared_ptr<Position> end;

    vector<int> rounded_sides; // 0= north, 1=east, 2=south, 3=west
    vector<int> exterior_sides; // 0= north, 1=east, 2=south, 3=west


    double scale; // 1, 3, 5, 7, 9, 11 ... (represents what scale this room is)
    unordered_set<RoomType> types; // type of the room, or if room was combined with other room it can have multiple types

    struct DoorHole { bool active = false; int lo, hi, lo_y, hi_y; };
    DoorHole holes[4]; // indexed by side: 0=front, 1=left, 2=back, 3=right

    vector<int> doors; // 0=front, 1=left, 2=back, 3=right
    int door_width;
    int door_height;
    int window_sides = 0; // bitmask: bit 0=south, 1=west, 2=north, 3=east

    void get_blocks(Grid<shared_ptr<Block>>& blocks, string material) {

        // XXX: handle circular rooms

        if (this->start == nullptr) return;

        // floor
        for (int x = this->start->x; x <= this->end->x; ++x) {
            for (int z = this->start->z; z <= this->end->z; ++z) {
                shared_ptr<Block> block = std::make_shared<Block>();
                block->position = std::make_shared<Position>(x, this->start->y, z);
                block->size = 1;
                block->material = material;
                blocks[{block->position->x, block->position->y, block->position->z}] = block;
            }
        }


        if (this->types.find(RoomType::HALLWAY) != this->types.end()) {
            return; // hallways don't have walls
        }

        // resolve door holes — use pre-computed if available, else centered fallback
        DoorHole hole_south, hole_north, hole_west, hole_east;

        if (!this->doors.empty()) {
            int cx = (this->start->x + this->end->x) / 2;
            int cz = (this->start->z + this->end->z) / 2;
            int hw = this->door_width / 2;
            int door_lo_y = this->start->y + 1;
            int door_hi_y = door_lo_y + this->door_height;

            for (int d : this->doors) {
                DoorHole hole = this->holes[d];
                if (!hole.active) {
                    switch (d) {
                        case 0: hole = {true, cx - hw, cx + hw, door_lo_y, door_hi_y}; break;
                        case 1: hole = {true, cz - hw, cz + hw, door_lo_y, door_hi_y}; break;
                        case 2: hole = {true, cx - hw, cx + hw, door_lo_y, door_hi_y}; break;
                        case 3: hole = {true, cz - hw, cz + hw, door_lo_y, door_hi_y}; break;
                    }
                }
                switch (d) {
                    case 0: hole_south = hole; break;
                    case 1: hole_west  = hole; break;
                    case 2: hole_north = hole; break;
                    case 3: hole_east  = hole; break;
                }
            }
        }

        // north wall (z = end->z) — door 2 (back)
        for (int x = this->start->x; x <= this->end->x; ++x) {
            for (int y = this->start->y+1; y <= this->end->y; ++y) {
                if (hole_north.active && x >= hole_north.lo && x <= hole_north.hi
                    && y >= hole_north.lo_y && y <= hole_north.hi_y)
                    continue;
                shared_ptr<Block> block = std::make_shared<Block>();
                block->position = std::make_shared<Position>(x, y, this->end->z);
                block->size = 1;
                block->material = material;
                blocks[{block->position->x, block->position->y, block->position->z}] = block;
            }
        }

        // east wall (x = end->x) — door 3 (right)
        for (int z = this->start->z; z < this->end->z; ++z) {
            for (int y = this->start->y+1; y <= this->end->y; ++y) {
                if (hole_east.active && z >= hole_east.lo && z <= hole_east.hi
                    && y >= hole_east.lo_y && y <= hole_east.hi_y)
                    continue;
                shared_ptr<Block> block = std::make_shared<Block>();
                block->position = std::make_shared<Position>(this->end->x, y, z);
                block->size = 1;
                block->material = material;
                blocks[{block->position->x, block->position->y, block->position->z}] = block;
            }
        }

        // south wall (z = start->z) — door 0 (front)
        for (int x = this->start->x; x < this->end->x; ++x) {
            for (int y = this->start->y+1; y <= this->end->y; ++y) {
                if (hole_south.active && x >= hole_south.lo && x <= hole_south.hi
                    && y >= hole_south.lo_y && y <= hole_south.hi_y)
                    continue;
                shared_ptr<Block> block = std::make_shared<Block>();
                block->position = std::make_shared<Position>(x, y, this->start->z);
                block->size = 1;
                block->material = material;
                blocks[{block->position->x, block->position->y, block->position->z}] = block;
            }
        }

        // west wall (x = start->x) — door 1 (left)
        for (int z = this->start->z+1; z < this->end->z; ++z) {
            for (int y = this->start->y+1; y <= this->end->y; ++y) {
                if (hole_west.active && z >= hole_west.lo && z <= hole_west.hi
                    && y >= hole_west.lo_y && y <= hole_west.hi_y)
                    continue;
                shared_ptr<Block> block = std::make_shared<Block>();
                block->position = std::make_shared<Position>(this->start->x, y, z);
                block->size = 1;
                block->material = material;
                blocks[{block->position->x, block->position->y, block->position->z}] = block;
            }
        }
    }

};

struct Section;

struct StairConnection {
    int side; // 0=front, 1=left, 2=back, 3=right
    shared_ptr<Section> target;
};

struct Section {
    RoomPattern pattern;
    shared_ptr<Position> start;
    shared_ptr<Position> end;
    int scale;
    int vertical_scale;

    vector<int> doors; // 0=front, 1=left, 2=back, 3=right
    vector<StairConnection> stairs;
    vector<int> combined_with; // 0=front, 1=left, 2=back, 3=right
    vector<int> rounded_sides; // 0=front, 1=left, 2=back, 3=right

    vector<shared_ptr<Room>> rooms;
};


