#pragma once


#include "../chunks/Chunk.h"
#include "space/Planet.h"
#include "util/Position.h"
#include "util/Offset.h"
#include "util/Random.h"
#include "util/CoordinateConversion.hpp"
#include "FluidSimulation.hpp"

#include <memory>
#include <deque>



using namespace std;


/**
 * After terrain is loaded in, rivers are carved through chunks. This class is in charge of that.
 */
struct RiverGen {


    shared_ptr<Planet> planet;
    shared_ptr<RootChunk> root_chunk;
    shared_ptr<WorldBlockPool> world_block_pool;
    deque<shared_ptr<Position>> chunk_512s_pos; 
    deque<pair<shared_ptr<Chunk>, ChunkType>> delete_queue;
	vector<shared_ptr<FluidSimulation>> river_simulations;



    RiverGen(
        shared_ptr<Planet> planet, 
        shared_ptr<RootChunk> root_chunk, 
        shared_ptr<WorldBlockPool> world_block_pool
    ) {
        this->planet = planet;
        this->root_chunk = root_chunk;
        this->world_block_pool = world_block_pool;

    }

	void tick(shared_ptr<Position>& player_position, bool player_movement_reset) {
        

        // step fluid simulations
        for (shared_ptr<FluidSimulation> sim : this->river_simulations) {

            if (!sim->started) {
                // shared_ptr<Chunk> chunk = this->root_chunk->getChunk(sim->start_chunk_type, sim->start->x, sim->start->z);
                // if (this->world_block_pool->get_block_at_position(chunk->position) != nullptr) {
                //     sim->started = true;
                // }

            }
            else {
                sim->tick(this->root_chunk, this->world_block_pool);
            }

        }



        // determine player position and get nearby chunks at the Erosion level
        if (player_movement_reset) {

            this->chunk_512s_pos.clear();
            for(int x = -Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16]; x < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16]; x += 512) {
                for(int z = -Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16]; z < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16]; z += 512) {
                    int64_t wrap_x = x + player_position->x;
                    int64_t wrap_z = z + player_position->z;
                    CoordinateConversion::wrap(wrap_x, wrap_z, this->planet->worldSize);
                    shared_ptr<Position> pos = std::make_shared<Position>(
                        wrap_x,
                        -1,
                        wrap_z
                    );
                    this->chunk_512s_pos.push_back(pos);
                }
            }
        }

        if (this->chunk_512s_pos.empty()) return;

        shared_ptr<Position> next = this->chunk_512s_pos.back();
        this->chunk_512s_pos.pop_back();

        shared_ptr<Chunk> chunk_512 = this->planet->rootChunk->getChunk(ChunkType::CHUNK_512, next->x, next->z);
        if (chunk_512 != nullptr && chunk_512->subChunksInitialized) {
            int64_t id = chunk_512->getId();

            // plan rivers through 64 chunks first
            int square = chunk_512->getSquareOfSub();
            if (chunk_512->river_starts__to_rivers.empty()) {

                // MAKE RIVERS from inflows to outflow, or 1 river from random chunk to outflow
                
                // get river starts
                vector<int> river_starts = get_river_starts(chunk_512, square);

                // make rivers randomly (favoring low spots and avoiding edges)
                for (int river_start : river_starts) {
                    vector<int> path = make_river(river_start, chunk_512, square);
                    chunk_512->river_starts__to_rivers[river_start] = path;
                }


                // requeue for next step
                chunk_512s_pos.push_front(next);
            }
            // then start fluid simulations
            else {


                // START FLUID simulations

                if (river_simulations.size() == 0) {
                    shared_ptr<FluidSimulation> test_sim = std::make_shared<FluidSimulation>();
                    test_sim->start= std::make_shared<Position>(8388606, 2274, 15);
                    test_sim->flow_amount = 1;
                    test_sim->init(this->world_block_pool);
                    test_sim->start_chunk_type = ChunkType::CHUNK_1;
                    river_simulations.push_back(test_sim);
                }


            }

        }
        // requeue if not ready
        else {
            chunk_512s_pos.push_front(next);
        }







    
    }

    vector<int> get_river_starts(shared_ptr<Chunk> chunk, int square) {
        vector<int> river_starts;
        if (chunk->inFlows.empty()) {
            int spike = chunk->position->makeSpike();
            int index = Random::randInt(this->planet->seed + spike, 0, chunk->subChunks.size());
            int attempts = 0;
            while(
                chunk->subChunks[index]->position->y < chunk->subChunks[chunk->outFlow]->position->y || // lower than outflow
                chunk->isEdgeOfChunk(index, chunk->subChunks.size(), square) // edge index
            ) {
                if (attempts < chunk->subChunks.size()) { // or we've tried to many times
                    break;
                }
                ++attempts;
                index = Random::randInt(this->planet->seed + spike + attempts, 0, chunk->subChunks.size());
            }
            if (
                chunk->subChunks[index]->position->y < chunk->subChunks[chunk->outFlow]->position->y || 
                chunk->isEdgeOfChunk(index, chunk->subChunks.size(), square)) {
                index = chunk->outFlow;
            }
            river_starts.push_back(index);
        }
        else {
            for(pair<const int, int>& in_flow : chunk->inFlows) {
                river_starts.push_back(in_flow.first);
            }
        }

        return river_starts;
    }


    vector<int> make_river(int river_start, shared_ptr<Chunk> chunk, int square) {

        // make rivers randomly (favoring low spots and avoiding edges)
        int index = river_start;

        vector<int> path;
        int current_flow = 1;
        while(index != chunk->outFlow) {
            vector<int> options = get_options_for_flow(index, square, chunk->outFlow);

            int lowest_index = options[0];
            int lowest_height = chunk->subChunks[lowest_index]->position->y;
            for (int option : options) {
                if (chunk->subChunks[option]->position->y < lowest_height) {
                    lowest_index = option;
                    lowest_height = chunk->subChunks[option]->position->y;
                }
            }

            path.push_back(index);

            current_flow += Chunk::calculate_added_flow_amount(path.size() + chunk->getSquareOfSub(), chunk->getSquareOfSub(), chunk->precipitation);
            chunk->subChunks[index]->flowSide = chunk->flowDir(index, lowest_index, square);
            chunk->subChunks[index]->totalFlow = current_flow;
            chunk->subChunks[index]->outFlow = Chunk::getSubOutFlow(index, chunk->subChunks[index]->flowSide, chunk->subChunks[index]->subChunks.size(), chunk->subChunks[index]->getSquareOfSub());

            int nextChunkInflowIndex = Chunk::matchFlowToFlow(chunk->subChunks[index]->outFlow, chunk->subChunks[index]->flowSide, chunk->subChunks[index]->subChunks.size(), chunk->subChunks[index]->getSquareOfSub());
            chunk->subChunks[lowest_index]->inFlows[nextChunkInflowIndex] = current_flow;

            index = lowest_index;
        }
        chunk->subChunks[chunk->outFlow]->flowSide = chunk->flowSide;
        chunk->subChunks[chunk->outFlow]->totalFlow = chunk->totalFlow;
        chunk->subChunks[chunk->outFlow]->outFlow = Chunk::getSubOutFlow(chunk->outFlow, chunk->flowSide, chunk->subChunks[chunk->outFlow]->subChunks.size(), chunk->subChunks[chunk->outFlow]->getSquareOfSub());
        path.push_back(chunk->outFlow);
       
        return path;
    }

    /**
     * returns any neighbors to the index that are inside the chunk and are not edges
     * 
     * if one neighbor is the outflow then only that one is returned
     */
    vector<int> get_options_for_flow(int currentIndex, int square, int outflow) {

        int num_subchunks = square * square;

        int north = currentIndex + square;
        int east = currentIndex + 1;
        int south = currentIndex - square;
        int west = currentIndex - 1;

        if (north == outflow) {
            return {north};
        }
        else if (east == outflow) {
            return {east};
        }
        else if (south == outflow) {
            return {south};
        }
        else if (west == outflow) {
            return {west};
        }



        int current_o_x, current_o_z;
        Chunk::convertIndex(currentIndex, square, current_o_x, current_o_z);
        int ouftflow_o_x, ouftflow_o_z;
        Chunk::convertIndex(outflow, square, ouftflow_o_x, ouftflow_o_z);
        int current_dist = Chunk::manhattenDistance(current_o_x, current_o_z, ouftflow_o_x, ouftflow_o_z);

        vector<int> neighbors;
        if ( north < num_subchunks && !Chunk::isEdgeOfChunk(north, num_subchunks, square) ) {
            int o_x, o_z;
            Chunk::convertIndex(north, square, o_x, o_z);
            int north_dist = Chunk::manhattenDistance(o_x, o_z, ouftflow_o_x, ouftflow_o_z);
            if (north_dist <= current_dist) {
                neighbors.push_back(north);
            }
        }

        if (east % square != 0 && !Chunk::isEdgeOfChunk(east, num_subchunks, square) ) {
            int o_x, o_z;
            Chunk::convertIndex(east, square, o_x, o_z);
            int east_dist = Chunk::manhattenDistance(o_x, o_z, ouftflow_o_x, ouftflow_o_z);
            if (east_dist <= current_dist) {
                neighbors.push_back(east);
            }
        }

        if (south >= 0 && !Chunk::isEdgeOfChunk(south, num_subchunks, square) ) {
            int o_x, o_z;
            Chunk::convertIndex(south, square, o_x, o_z);
            int south_dist = Chunk::manhattenDistance(o_x, o_z, ouftflow_o_x, ouftflow_o_z);
            if (south_dist <= current_dist) {
                neighbors.push_back(south);
            }
        }

        if ((west + 1) % square != 0 && !Chunk::isEdgeOfChunk(west, num_subchunks, square) ) {
            int o_x, o_z;
            Chunk::convertIndex(west, square, o_x, o_z);
            int west_dist = Chunk::manhattenDistance(o_x, o_z, ouftflow_o_x, ouftflow_o_z);
            if (west_dist <= current_dist) {
                neighbors.push_back(west);
            }
        }


        return neighbors;
    }


    bool is_in_16_range(shared_ptr<Position> player_position, shared_ptr<Chunk> chunk) {
        int dist = CoordinateConversion::cheapWrapMaxDistance(player_position->x, player_position->z, chunk->position->x, chunk->position->z, this->planet->worldSize);
        return dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16];
    }


	void report_chunk_change(int64_t x, int64_t z) {

        Chunk::get_coordinates_by_chunk_type(ChunkType::CHUNK_16, x, z);
		
        for (shared_ptr<FluidSimulation> sim : this->river_simulations) {
            sim->report_chunk_change(x, z);
        }
	}


	void report_chunk_change(const vector<shared_ptr<Block>>& blocks) {


        unordered_map<string, pair<int64_t, int64_t>> chunk16s;
        for (const shared_ptr<Block>& block : blocks) {
            int64_t x = block->position_double != nullptr? block->position_double->x : block->position->x;
            int64_t z = block->position_double != nullptr? block->position_double->z : block->position->z;
            Chunk::get_coordinates_by_chunk_type(ChunkType::CHUNK_16, x, z);

            chunk16s[Block::stringTagNoY(-1, x, z)] = pair<int64_t, int64_t>(x, z);
        }


		for (pair<string, pair<int64_t, int64_t>> chunk16 : chunk16s) {
            int64_t x = chunk16.second.first;
            int64_t z = chunk16.second.second;
            for (shared_ptr<FluidSimulation> sim : this->river_simulations) {
                sim->report_chunk_change(x, z);
            }
        }
	}

    

};