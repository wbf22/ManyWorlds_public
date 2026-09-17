#pragma once


#include "util/Position.h"
#include "util/Random.h"
#include "space/Planet.h"
#include "../blocks/Block.h"
#include "../chunks/Chunk.h"
#include "util/BMaterial.hpp"


using namespace std;


struct PathBlock {
    shared_ptr<Block> block;
    shared_ptr<PathBlock> previous;
    shared_ptr<PathBlock> next;
    int path;

    bool was_sunk;
};


/**
 * After terrain is loaded in, rivers are carved through chunks. This class is in charge of that.
 */
struct FluidSimulation {



    string fluid_type = BMaterial::WATER;
    shared_ptr<Position> start;
    int flow_amount = 1;
    ChunkType start_chunk_type;
    bool started = false;



    vector<shared_ptr<PathBlock>> path_heads;
    unordered_map<string, shared_ptr<Block>> xyz_string_tag_to_fluid_block;
    unordered_map<string, unordered_set<shared_ptr<PathBlock>>> chunk16xz_string_tag_to_fluid_block;
    unordered_set<int> done_paths;
    unordered_set<shared_ptr<PathBlock>> dying_paths;



    void init(
        shared_ptr<WorldBlockPool> world_block_pool
    ) {

        // spawn initial blocks
        for (int i = 0; i < this->flow_amount; ++i) {

            string chunk_16_xz = Chunk::chunk16_group_id(this->start->x, this->start->z);
            
            shared_ptr<Block> new_block = std::make_shared<Block>();
            new_block->position = std::make_shared<Position>(this->start->x, this->start->y, this->start->z);
            new_block->size = 1;
            new_block->material = this->fluid_type;
        
            vector<shared_ptr<Block>> spawn_block = {new_block};
            world_block_pool->spawn_blocks(
                chunk_16_xz,
                spawn_block
            );

            xyz_string_tag_to_fluid_block[new_block->stringTag()] = new_block;
            path_heads.push_back(std::make_shared<PathBlock>());
            path_heads[i]->block = new_block;
            path_heads[i]->previous = nullptr;
            path_heads[i]->next = nullptr;

        }


    }


    void tick(
        shared_ptr<RootChunk> root_chunk, 
        shared_ptr<WorldBlockPool> world_block_pool
    ) {


        // DYING PATHS
        for (int i = 0; i < this->flow_amount; ++i) {

            if (dying_paths.empty()) break;

            // get block that has descendants that need to be cleaned up
            shared_ptr<PathBlock> head = *dying_paths.begin();
            while (head != nullptr && head->next == nullptr) {
                string chunk16_xz = Chunk::chunk16_group_id(head->block->position->x, head->block->position->z);
                if (chunk16xz_string_tag_to_fluid_block.find(chunk16_xz) != chunk16xz_string_tag_to_fluid_block.end()) {
                    chunk16xz_string_tag_to_fluid_block[chunk16_xz].erase(head);
                }
                dying_paths.erase(head);
                head = dying_paths.empty()? nullptr : *dying_paths.begin();
            }
            if (head == nullptr) break;

            // clean up block and add descendant to list
            xyz_string_tag_to_fluid_block.erase(head->block->stringTag());
            dying_paths.insert(head->next);
        }
        

        // ADVANCE each path
        vector<shared_ptr<Block>> blocks_to_spawn;
        int world_size = world_block_pool->planet->worldSize;
        for (int i = 0; i < this->flow_amount; ++i) {

            // skip if path is done
            if (done_paths.find(i) != done_paths.end()) continue;

            // DETERMINE best option
            shared_ptr<PathBlock> head = this->path_heads[i];
            shared_ptr<Chunk> chunk16 = root_chunk->getChunk(ChunkType::CHUNK_16, head->block->position->x, head->block->position->z);

            // stop once we fall off the world (and reset once new chunks are loaded)
            if (chunk16 == nullptr) {
                done_paths.insert(i);
                continue;
            }

            shared_ptr<Position> pos = std::make_shared<Position>(head->block->position->x, head->block->position->y, head->block->position->z);
            vector<string> options = get_options(pos, head, world_block_pool);

            // choose one randomly
            string chosen_direction = options[Random::randInt(pos->makeSpike(), 0, options.size())];

            // raise position if block is sunken in terrain
            if (chosen_direction == "up" && head->was_sunk) {
                pos->y += 1;
                options = get_options(pos, head, world_block_pool);
                chosen_direction = options[Random::randInt(pos->makeSpike(), 0, options.size())];
            }


            // apply direction
            if (chosen_direction == "up") {
                pos->y += 1;
            }
            else if (chosen_direction == "down") {
                pos->y -= 1;
            }
            else if (chosen_direction == "north") {
                pos->z = CoordinateConversion::wrap(pos->z+1, world_size);
            }
            else if (chosen_direction == "south") {
                pos->z = CoordinateConversion::wrap(pos->z-1, world_size);
            }
            else if (chosen_direction == "east") {
                pos->x = CoordinateConversion::wrap(pos->x+1, world_size);
            }
            else if (chosen_direction == "west") {
                pos->x = CoordinateConversion::wrap(pos->x-1, world_size);
            }


            // XXX: maybe if there's an option to butt up against another path that's good do so based on random chance with viscosity

            // if we're on top of other water blocks dig down beneath them if applicable
            int max_dig_down = this->flow_amount * 10.0 / chunk16->getMaterialHardness();
            int depth = 0;
            bool water_underneath = false;
            bool at_least_one_underneath = false;
            do {
                ++depth;
                water_underneath = xyz_string_tag_to_fluid_block.find(
                    Block::stringTag(
                        1, 
                        pos->x, 
                        pos->y - depth, 
                        pos->z
                    )
                ) != xyz_string_tag_to_fluid_block.end();
                at_least_one_underneath = water_underneath || at_least_one_underneath;

            } while (water_underneath && depth < max_dig_down);
            
            if (at_least_one_underneath && depth < max_dig_down) {
                pos->y -= depth;
            }


            // if on terrain block dig down 1
            bool was_sunk = false;
            shared_ptr<Position> pos_down = std::make_shared<Position>(
                pos->x, 
                pos->y - 1,
                pos->z
            );
            if (
                xyz_string_tag_to_fluid_block.find(
                    Block::stringTag(
                        1, 
                        pos_down->x, 
                        pos_down->y, 
                        pos_down->z
                    )
                ) == xyz_string_tag_to_fluid_block.end()
            ) {
                shared_ptr<Block> down_block = world_block_pool->get_block_at_position(pos_down);
                bool last_is_above = false;
                if (head->block != nullptr) {
                    last_is_above = head->block->position->y == pos->y+1;
                }
                // if below isn't water block, but is a terrain block, and the last water block isn't right above us, then sink
                if (down_block != nullptr && !last_is_above) {
                    pos = pos_down;
                    world_block_pool->player_destroy_block(down_block);
                    was_sunk = true;
                }
            }


            //  set path head and blocks
            shared_ptr<Block> new_block = std::make_shared<Block>();
            new_block->position = pos;
            new_block->size = 1;
            new_block->material = this->fluid_type;
            // XXX: use water foam material if on drop (maybe with some randomness)

            shared_ptr<PathBlock> new_head = std::make_shared<PathBlock>();
            new_head->block = new_block;
            new_head->previous = head;
            new_head->next = nullptr;
            new_head->path = i;
            new_head->was_sunk = was_sunk;
            path_heads[i] = new_head;

            // save references
            xyz_string_tag_to_fluid_block[new_block->stringTag()] = new_block;
            string chunk16_xz = Chunk::chunk16_group_id(new_block->position->x, new_block->position->z);
            if (chunk16xz_string_tag_to_fluid_block.find(chunk16_xz) == chunk16xz_string_tag_to_fluid_block.end())
                chunk16xz_string_tag_to_fluid_block[chunk16_xz] = {};
            chunk16xz_string_tag_to_fluid_block[chunk16_xz].insert(new_head);
            blocks_to_spawn.push_back(new_block);
        }


        // SPAWN water blocks
        // sort into chunk 16 groups
        unordered_map<string, vector<shared_ptr<Block>>> groups;
        for(shared_ptr<Block>& b : blocks_to_spawn) {
            string chunk_16_xz = Chunk::chunk16_group_id(b->position->x, b->position->z);
            groups[chunk_16_xz].push_back(b);
        }
        for (pair<string, vector<shared_ptr<Block>>> group : groups ) {
            world_block_pool->spawn_blocks(
                group.first,
                group.second
            );

        }

    }


    /**
     * Recalculates flows if a block is deleted and the the deleted block would change the flow of the river
     * 
     */
    void report_chunk_change(int64_t chunk16_x, int64_t chunk16_z) {
        string tag = Chunk::chunk16_group_id(chunk16_x, chunk16_z);
        if (chunk16xz_string_tag_to_fluid_block.find(tag) != chunk16xz_string_tag_to_fluid_block.end()) {
            unordered_set<shared_ptr<PathBlock>> chunk_blocks = chunk16xz_string_tag_to_fluid_block[tag];
            for (shared_ptr<PathBlock> head : chunk_blocks) {
                dying_paths.insert(head);
                head->previous->next = nullptr; // disconnect from path
                xyz_string_tag_to_fluid_block.erase(head->block->stringTag());
                done_paths.erase(head->path);

                string previous_chunk16_tag = Chunk::chunk16_group_id(head->previous->block->position->x, head->previous->block->position->z);
                if (previous_chunk16_tag != tag) {
                    path_heads[head->path] = head->previous;
                }
            }
            chunk16xz_string_tag_to_fluid_block.erase(tag);

        }

    }


    vector<string> get_options(shared_ptr<Position>& pos, const shared_ptr<PathBlock>& head, const shared_ptr<WorldBlockPool>& world_block_pool) {

        bool up, down, north, south, east, west;
        this->check_for_neighbors(pos, world_block_pool, up, down, north, south, east, west);

        vector<string> options;
        if (!down) {
            options.push_back("down");
        }
        else if (!north || !south || !east || !west) {
            if (!north) {
                options.push_back("north");
            }
            if (!south) {
                options.push_back("south");
            }
            if (!east) {
                options.push_back("east");
            }
            if (!west) {
                options.push_back("west");
            }
        }
        else if (!up) {
            options.push_back("up");
        }

        
        // backtrack if no options
        shared_ptr<PathBlock> old_head = head;
        while (options.empty()) {
            old_head = old_head->previous;
            if (old_head == nullptr) {
                break;
            }
            pos = std::make_shared<Position>(old_head->block->position->x, old_head->block->position->y, old_head->block->position->z);
            options = get_options(pos, old_head, world_block_pool);
        }

        return options;
    }


    // returns true in the result bools if there is a block at that position
    void check_for_neighbors(shared_ptr<Position>& position, const shared_ptr<WorldBlockPool>& world_block_pool, bool& up, bool& down, bool& north, bool& south, bool& east, bool& west) {

            // check for water blocks: up-down-north-south-east-west
            int world_size = world_block_pool->planet->worldSize;
            up = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    position->x, 
                    position->y + 1, 
                    position->z
                )
            ) != xyz_string_tag_to_fluid_block.end();
            down = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    position->x, 
                    position->y - 1, 
                    position->z
                )
            ) != xyz_string_tag_to_fluid_block.end();
            north = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    position->x, 
                    position->y, 
                    CoordinateConversion::wrap(position->z + 1, world_size)
                )
            ) != xyz_string_tag_to_fluid_block.end();
            south = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    position->x, 
                    position->y,
                    CoordinateConversion::wrap(position->z - 1, world_size)
                )
            ) != xyz_string_tag_to_fluid_block.end();
            east = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    CoordinateConversion::wrap(position->x + 1, world_size), 
                    position->y, 
                    position->z
                )
            ) != xyz_string_tag_to_fluid_block.end();
            west = xyz_string_tag_to_fluid_block.find(
                Block::stringTag(
                    1, 
                    CoordinateConversion::wrap(position->x - 1, world_size), 
                    position->y, 
                    position->z
                )
            ) != xyz_string_tag_to_fluid_block.end();

            // check for solid blocks not in water blocks: up-down-north-south-east-west
            if (!up) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    position->x, 
                    position->y + 1, 
                    position->z
                );
                up = world_block_pool->get_block_at_position(pos) != nullptr;
            }
            if (!down) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    position->x, 
                    position->y - 1, 
                    position->z
                );
                down = world_block_pool->get_block_at_position(pos) != nullptr;
            }
            if (!north) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    position->x, 
                    position->y, 
                    CoordinateConversion::wrap(position->z + 1, world_size)
                );
                north = world_block_pool->get_block_at_position(pos) != nullptr;
            }
            if (!south) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    position->x, 
                    position->y, 
                    CoordinateConversion::wrap(position->z - 1, world_size)
                );
                south = world_block_pool->get_block_at_position(pos) != nullptr;
            }
            if (!east) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    CoordinateConversion::wrap(position->x + 1, world_size), 
                    position->y, 
                    position->z
                );
                east = world_block_pool->get_block_at_position(pos) != nullptr;
            }
            if (!west) {
                shared_ptr<Position> pos = std::make_shared<Position>(
                    CoordinateConversion::wrap(position->x - 1, world_size), 
                    position->y, 
                    position->z
                );
                west = world_block_pool->get_block_at_position(pos) != nullptr;
            }
    }







};