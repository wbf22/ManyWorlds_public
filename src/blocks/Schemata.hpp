#pragma once


#include "Block.h"
#include "util/Position.h"
#include "util/PositionDouble.h"
#include "util/CoordinateConversion.hpp"
#include "util/serialization/KVSerializer.hpp"
#include <vector>
#include <memory>
#include <limits>


using namespace std;



struct Schemata {


    vector<shared_ptr<Block>> blocks;
    shared_ptr<PositionDouble> size;


    /*
        blocks in the schemata, stored as offsets from the min corner
    */
    Schemata(vector<shared_ptr<Block>>& blocks, shared_ptr<PositionDouble> player_position, int64_t world_size){
        this->blocks.insert(this->blocks.begin(), blocks.begin(), blocks.end());

        // find min corner across all blocks
        double min_x = numeric_limits<double>::max();
        double min_y = numeric_limits<double>::max();
        double min_z = numeric_limits<double>::max();
        double max_x = -numeric_limits<double>::max();
        double max_y = -numeric_limits<double>::max();
        double max_z = -numeric_limits<double>::max();

        // first pass: find min/max
        for (auto& block : blocks) {
            double bx = block->position_double->x;
            double by = block->position_double->y;
            double bz = block->position_double->z;
            if (bx < min_x) min_x = bx;
            if (by < min_y) min_y = by;
            if (bz < min_z) min_z = bz;
            if (bx + block->size > max_x) max_x = bx + block->size;
            if (by + block->size > max_y) max_y = by + block->size;
            if (bz + block->size > max_z) max_z = bz + block->size;
        }

        // second pass: convert to offsets from min corner
        for (auto& block : this->blocks) {
            block->position_double->x -= min_x;
            block->position_double->y -= min_y;
            block->position_double->z -= min_z;
        }

        this->size = std::make_shared<PositionDouble>(
            max_x - min_x,
            max_y - min_y,
            max_z - min_z
        );

        cout << "schemata size " << size->toString() << endl;
    }

    Schemata() = default;

    void rotate_90_cw() {
        for (auto& block : blocks) {
            double x = block->position_double->x;
            double z = block->position_double->z;
            block->position_double->x = z;
            block->position_double->z = -x;
        }
        recompute_size();
    }

    void rotate_90_ccw() {
        for (auto& block : blocks) {
            double x = block->position_double->x;
            double z = block->position_double->z;
            block->position_double->x = -z;
            block->position_double->z = x;
        }
        recompute_size();
    }

    void recompute_size() {
        double min_x = numeric_limits<double>::max();
        double min_y = numeric_limits<double>::max();
        double min_z = numeric_limits<double>::max();
        double max_x = -numeric_limits<double>::max();
        double max_y = -numeric_limits<double>::max();
        double max_z = -numeric_limits<double>::max();

        for (auto& block : blocks) {
            double bx = block->position_double->x;
            double by = block->position_double->y;
            double bz = block->position_double->z;
            if (bx < min_x) min_x = bx;
            if (by < min_y) min_y = by;
            if (bz < min_z) min_z = bz;
            if (bx + block->size > max_x) max_x = bx + block->size;
            if (by + block->size > max_y) max_y = by + block->size;
            if (bz + block->size > max_z) max_z = bz + block->size;
        }

        this->size = std::make_shared<PositionDouble>(
            max_x - min_x,
            max_y - min_y,
            max_z - min_z
        );
    }


    DECLARE_NAMED_FIELDS(
        FIELD("blocks", &Schemata::blocks),
        FIELD("size", &Schemata::size)
    );
};
