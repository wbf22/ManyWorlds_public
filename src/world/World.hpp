// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <chrono>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <tuple>
#include <deque>
#include <unordered_set>
#include <cstdio>
#include <array>
#include <cmath>


#include "../chunks/Chunk.h"
#include "space/Planet.h"
#include "../chunks/RootChunk.h"
#include "../blocks/WorldBlockPool.hpp"
#include "../blocks/DeferredWorldBlockPool.hpp"
#include "util/Position.h"
#include "util/concurrent_map.hpp"
#include "util/BMaterial.hpp"
#include "util/Random.h"
#include "../blocks/Block.h"
#include "util/CoordinateConversion.hpp"
#include "util/serialization/KVSerializer.hpp"
#include "RiverGen.hpp"
#include "../buildings_and_cities/BuildingGen.hpp"
#include "../buildings_and_cities/City.hpp"
#ifdef GODOT_ENABLED
#include "GodotHdr.hpp"
#endif
#include "util/StellarCoordinate.hpp"
#include "../plants/PlantGen.hpp"
#include "util/Grid.hpp"
#include "util/PartContour.hpp"

#ifdef GODOT_ENABLED
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/panorama_sky_material.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include "util/stb_image.h"
#endif



template<typename T>
using uptr = std::unique_ptr<T>;



using namespace std;
using namespace godot;


class WorldBlockPool;
class PlantGen;
class Block;

enum class GenType {
	TERRAIN,
	SPACE,
	SKY,
	MAP
};

/**
 * 
 */
class World {

public:	
#ifdef GODOT_ENABLED
	// Game node that parents our scene nodes (block meshes, deferred pools)
	Node* node = nullptr;
#endif

	GenType genType = GenType::SPACE;

    thread worker_thread;
    atomic<bool> running{false};

	shared_ptr<Position> playerPosition = Position::buildS(0, 1000, 0);
	shared_ptr<Position> player_position_snapshot = Position::buildS(0, 1000, 0);
	mutable mutex player_position_mutex;
	shared_ptr<Position> last_player_position = Position::buildS(-10000000, 0, -10000000); // induces reset on first tick

	// THROTTLING VARIABLES
	int load_size = 512;
	int load_cnt = 0;
	bool genDone = false;
	bool spawnDone = false;
	int recalculateCounter;

	shared_ptr<Planet> planet;
	shared_ptr<RootChunk> rootChunk;
	shared_ptr<WorldBlockPool> cubePool;
	DeferredWorldBlockPool* deferred_cube_pool;

	// TERRAIN GEN
	vector<ChunkType> priority_order = {ChunkType::CHUNK_1, ChunkType::CHUNK_4, ChunkType::CHUNK_16}; // which chunk types we prefer to be spawned first

	deque<pair<shared_ptr<Chunk>, ChunkType>> chunks_to_gen; // position xz strings of chunks that should be genned in, ordered by proximity to player (does include chunks that are parents of desired chunks)
	deque<pair<shared_ptr<Chunk>, ChunkType>> finished_gen_chunks; // position xyz string of chunk-16s that have been genned to their desired type
	deque<shared_ptr<Chunk>> surface_chunks_to_gen;
	unordered_set<shared_ptr<Chunk>> desired_surface_chunks;
	unordered_set<shared_ptr<Chunk>> surface_chunks_to_despawn;
	unordered_set<string> active_surface_groups;
	unordered_map<string, ChunkType> chunk_to_current_type_in_world; // position xz of chunk to current type in world
	unordered_set<shared_ptr<Chunk>> desired_chunks; // chunks we want to be in world

	int DESPAWN_DISTANCE = Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16] * 6;
	unordered_set<shared_ptr<Chunk>> chunks_to_despawn; // chunks we want to despawn eventually (meaning they're not at the despawn distance yet but are no longer in desired_chunks)
	unordered_set<shared_ptr<Chunk>>::iterator despawn_iterator;


	// SPACE GEN (LY-based: planet shell blocks are emitted in the stellar LY frame,
	// matching SpaceGen; game_scale = LY->godot factor so markers render as ~1-godot-unit cubes)
	static constexpr double LY_PER_M_WORLD = 1.0 / 9.4607304725808e15; // matches SpaceGen::LY_PER_M
	double game_scale = 1.0;
	Vector3 player_location;
	Vector3 planet_center;          // legacy meters/10^k godot center (init_for_space)
	Vector3 planet_to_player_ly;       // planet LY offset from viewer (shell frame)

	static double cap_angle_for_blocks(double radius_ly, double block_size_ly, int max_blocks) {
		if (radius_ly <= 0.0 || block_size_ly <= 0.0 || max_blocks <= 0) return 0.0;
		double area_fraction = (double)max_blocks * block_size_ly * block_size_ly /
			(1.5 * 2.0 * M_PI * radius_ly * radius_ly);
		return acos(clamp(1.0 - area_fraction, -1.0, 1.0));
	}

	static double cap_sphere_radius_for_blocks(double radius_ly, double block_size_ly, int max_blocks) {
		double angle = cap_angle_for_blocks(radius_ly, block_size_ly, max_blocks);
		return 2.0 * radius_ly * sin(angle * 0.5);
	}

	static ChunkType closest_chunk_type(double block_size) {
		const ChunkType types[] = {
			ChunkType::CHUNK_1, ChunkType::CHUNK_4, ChunkType::CHUNK_16,
			ChunkType::CHUNK_64, ChunkType::CHUNK_512, ChunkType::CHUNK_8192,
			ChunkType::CHUNK_131072, ChunkType::CHUNK_2097152,
			ChunkType::CHUNK_16777216, ChunkType::CHUNK_134217728
		};
		ChunkType closest = types[0];
		double difference = INFINITY;
		for (ChunkType type : types) {
			double size = (double)Chunk::CHUNK_SIZES[type];
			double current_difference = abs(size - block_size);
			if (current_difference < difference) {
				closest = type;
				difference = current_difference;
			}
		}
		if (Chunk::CHUNK_SIZES[closest] < block_size) {
			closest = larger(closest);
		}
		return closest;
	}

	bool godot_enabled = true;


	// RIVER GEN
	shared_ptr<RiverGen> river_gen;

	// PLANT GEN
	shared_ptr<PlantGen> plant_gen;

	/*
	We'll keep track of whether a block is in world or not

	Every reload we'll make a list of what blocks should be in world
	*/

	// BUILDING GEN
	shared_ptr<BuildingGen> building_gen;
	

	int smoothness = 100;


	// METHODS

	// Sets default values
	World()
		: despawn_iterator(chunks_to_despawn.end())
	{ }

	shared_ptr<Position> get_player_position() const {
		lock_guard<mutex> lock(this->player_position_mutex);
		return this->playerPosition;
	}

	void set_player_position(shared_ptr<Position> position) {
		lock_guard<mutex> lock(this->player_position_mutex);
		this->playerPosition = std::move(position);
	}

	// Called when the game starts or when spawned
	void init(shared_ptr<Planet> planet)
	{

		cout << "Can you see me here biji?" << endl;
		cout << "dunk!" << endl;

		/*
		* determine world attributes (size, starlight, atmosphere, water, vegetation)
		* 
		* WORLD CALCULATE ( 2,097,152; 131,072; 8192 )
		*  - heights for initial chunks (random, water flows / sand)
		*/
		this->planet = planet;

		/*
			Init Terrain And Erosion
		*/

		// init cube pool
		this->cubePool = std::make_shared<WorldBlockPool>(
			#ifdef GODOT_ENABLED
				this->node,
			#else
				nullptr,
			#endif
			std::make_shared<Position>(0,0,0),
			this->planet,
			[this](int64_t x, int64_t z) {
				this->report_chunk_change(x, z);
			}
		);

		if (godot_enabled) {
			#ifdef GODOT_ENABLED
				this->deferred_cube_pool = memnew(DeferredWorldBlockPool);
				this->node->add_child(this->deferred_cube_pool);
			#else
				this->deferred_cube_pool = new DeferredWorldBlockPool();
			#endif
			this->deferred_cube_pool->cubePool = this->cubePool;
		}


		/*
			Init Plant Generation
		*/

		this->plant_gen = std::make_shared<PlantGen>(this->planet);
		PLANT_GEN_ON = planet->hasLife;

		/*
			Init River Gen
		*/
		if (godot_enabled) {
			river_gen = std::make_shared<RiverGen>(this->planet, this->rootChunk, this->cubePool);
		}




		// /*
		// 	Building Gen
		// */
		// this->building_gen = std::make_shared<BuildingGen>();
		// shared_ptr<City> city = std::make_shared<City>();
		// city->smallest_citizens = 8;
		// city->largest_citizens = 8;
		// vector<shared_ptr<Block>> blocks = this->building_gen->gen_building(456, 0, BuildingType::HOME, 100, 100, 80, this->planet, city);

		// this->cubePool->spawn_blocks(blocks[0]->position->toString(), blocks);


	}


	Transform3D init_for_space(shared_ptr<StellarCoordinate> player_location, double scale = 1.0) {

		this->genType = GenType::SPACE;

		// stellar LY frame: planet shell blocks are emitted in LY coords around the
		// planet's LY offset from the viewer; game_scale converts to godot on spawn
		this->game_scale = scale;
		auto& to = this->planet->location;
		constexpr double KM_PER_LY_WORLD = 9460730472581.0;
		double lx = (to->quadrant_x - player_location->quadrant_x) * 10000.0
		          + (int)to->light_year_x - (int)player_location->light_year_x
		          + ((double)(int64_t)to->km_x - (int64_t)player_location->km_x) / KM_PER_LY_WORLD;
		double ly = (to->quadrant_y - player_location->quadrant_y) * 10000.0
		          + (int)to->light_year_y - (int)player_location->light_year_y
		          + ((double)(int64_t)to->km_y - (int64_t)player_location->km_y) / KM_PER_LY_WORLD;
		double lz = (to->quadrant_z - player_location->quadrant_z) * 10000.0
		          + (int)to->light_year_z - (int)player_location->light_year_z
		          + ((double)(int64_t)to->km_z - (int64_t)player_location->km_z) / KM_PER_LY_WORLD;
		this->planet_to_player_ly = Vector3(lx, ly, lz);

		// place player just off the planet in godot space
		Transform3D player_transform;
		player_transform.origin = this->planet_to_player_ly * this->game_scale;

		return player_transform;
	}

	void start() {
		running = true;
    	worker_thread = std::thread(&World::worker_loop, this);
	}

	~World() {
		running = false;
		if (worker_thread.joinable())
			worker_thread.join();
	}


	// Called every frame
	bool TERRAIN_GEN_ON = true;
	bool PLANT_GEN_ON = true;
	bool RIVER_GEN_ON = false;
	bool SPACE_GEN_ON = false;
	bool detect_movement_reset = true;
	bool playerMovementReset = true;

	// SPACE TRANSITION (leave-planet despawn)
	atomic<bool> terrain_despawn_requested{false};
	unordered_set<string> terrain_groups_to_despawn;
	int terrain_despawn_per_tick = 3;
	void queue_all_terrain_for_despawn() {
		this->terrain_despawn_requested = true;
	}
	void despawn_terrain_paced() {
		int freed = 0;
		while (freed < this->terrain_despawn_per_tick && !this->terrain_groups_to_despawn.empty()) {
			string group_id = *this->terrain_groups_to_despawn.begin();
			this->terrain_groups_to_despawn.erase(this->terrain_groups_to_despawn.begin());
			this->deferred_cube_pool->despawn_blocks(group_id);
			++freed;
		}
	}

	void worker_loop()
	{
		while(running) {

			while(this->deferred_cube_pool->worker_thread_data.size() > 10) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			// leaving a planet: stop generating and pace-despawn the surface instead
			if (!TERRAIN_GEN_ON) {
				if (this->terrain_despawn_requested.exchange(false)) {
					this->terrain_groups_to_despawn.clear();
					for (auto& [group_id, type] : this->chunk_to_current_type_in_world) {
						this->terrain_groups_to_despawn.insert(group_id);
					}
				}
				despawn_terrain_paced();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			// Copy under the lock, then let generation use a stable snapshot.
			{
				lock_guard<mutex> lock(this->player_position_mutex);
				this->player_position_snapshot = std::make_shared<Position>(
					this->playerPosition->x, this->playerPosition->y, this->playerPosition->z);
			}

			// reset desired_chunks if player has moved
			if (detect_movement_reset) {
				if (this->genType == GenType::TERRAIN) {
					int dist = CoordinateConversion::cheapWrapMaxDistance(
						this->player_position_snapshot->x, 
						this->player_position_snapshot->z, 
						this->last_player_position->x, 
						this->last_player_position->z,
						this->planet->worldSize
					);
					this->playerMovementReset = dist > Chunk::RENDER_DISTANCES[ChunkType::CHUNK_1] * 0.75;
					if (this->playerMovementReset) {
						this->last_player_position = this->player_position_snapshot;
					}
				}
			}


			// doing gen and spawn, adjusting base off the performance
			if (TERRAIN_GEN_ON) {
				terrain_gen();
			}
			
			if (RIVER_GEN_ON) {
				this->river_gen->tick(this->player_position_snapshot, this->playerMovementReset);  
			}

			this->playerMovementReset = false;

		}
	}


	// TERRAIN GEN
	double TARGET_GEN_SPAWN_TIME = 0.009;
	double MAX_ITERATIONS = 500;
	double MIN_ITERATIONS = 8;
	double iterations = 100;
	double average_iterations = iterations;
	void terrain_gen() {
		this->world_gen();
	}

	ChunkType START_TYPE = ChunkType::CHUNK_16;
	ChunkType MID_TYPE = ChunkType::CHUNK_4;
	ChunkType END_TYPE = ChunkType::CHUNK_1;

	void world_gen() {

		// XXX: delete pointers to chunks above chunk 16 that are out of range

		int available_iterations = iterations;
		if (!this->chunks_to_gen.empty()) available_iterations *= 8; // chunk gen is much faster than spawning to we can multiple iterations in that case


		// DETERMINE HIGHEST PRIORITY BLOCKS TO LOAD

		if (this->playerMovementReset) {
			
			this->resetDesiredChunks();

			// set used_iterations to 1 since we've done a fair amout of work here
			available_iterations /= 2;

			
		}


		// GEN AND SPAWN WORK	
		
		// gen chunks and set to spawn
		while (available_iterations > 0) {

			--available_iterations;

			// Generate coarse surfaces outside the detailed terrain range.
			if (!this->surface_chunks_to_gen.empty()) {
				shared_ptr<Chunk> surface_chunk = this->surface_chunks_to_gen.front();
				this->surface_chunks_to_gen.pop_front();
				if (this->desired_surface_chunks.find(surface_chunk) == this->desired_surface_chunks.end())
					continue;
				bool finished = genOrGenBlocker(surface_chunk);
				while (!finished) finished = genOrGenBlocker(surface_chunk);
				this->set_surface_corner_heights(surface_chunk);
				// texture the far surface like voxel planet chunks (vegetation speckle)
				shared_ptr<Position> surface_world_pos = make_shared<Position>(
					surface_chunk->position->x, 0, surface_chunk->position->z
				);
				string surface_mat = assign_block_material(surface_world_pos, ChunkType::CHUNK_512);
				if (!surface_mat.empty()) surface_chunk->material = surface_mat;
				surface_chunk->is_surface_patch = true;
				string group_id = surface_chunk->stringTagNoY();
				if (this->active_surface_groups.insert(group_id).second) {
					shared_ptr<vector<shared_ptr<Block>>> group_blocks = make_shared<vector<shared_ptr<Block>>>();
					group_blocks->push_back(surface_chunk);
					this->deferred_cube_pool->spawn_blocks(group_id, group_blocks);
				}
				continue;
			}

			// GEN all chunks in chunks_to_gen (makes sure all needed chunks are loaded in too)
			if (!this->chunks_to_gen.empty()) {

				pair<shared_ptr<Chunk>, ChunkType> pair = this->chunks_to_gen.front();
				shared_ptr<Chunk> chunk = pair.first;
				ChunkType desired_type = pair.second;
				this->chunks_to_gen.pop_front();

				if (this->chunks_to_gen.size() % 100 == 0) cout << this->chunks_to_gen.size() << " chunks left to gen" << endl;

				// gen chunk
				bool finished = genOrGenBlocker(chunk);
				while(!finished) finished = genOrGenBlocker(chunk);

				if (this->genType == GenType::TERRAIN) {
					string next_block_tag = chunk->stringTagNoY();

					// gen chunk_4 or chunk_1's down to desired_type
					if (desired_type != START_TYPE) {
						for (shared_ptr<Chunk> subChunk : chunk->subChunks) {

							bool subchunk_finished = genOrGenBlocker(subChunk);
							while(!subchunk_finished) subchunk_finished = genOrGenBlocker(subChunk);

							if (desired_type == END_TYPE) {
								for (shared_ptr<Chunk> subSubChunk : subChunk->subChunks) {
									bool subsubchunk_finished = genOrGenBlocker(subSubChunk);
									while(!subsubchunk_finished) subsubchunk_finished = genOrGenBlocker(subSubChunk);
								}
							}
						}
					}
				
					this->chunk_to_current_type_in_world[next_block_tag] = ChunkType::NONE;
				}
				this->finished_gen_chunks.push_back(std::pair(chunk, desired_type));

			}
			// SPAWN once done with gen, start spawning in blocks
			else if (!this->finished_gen_chunks.empty()) {

				pair<shared_ptr<Chunk>, ChunkType> pair = this->finished_gen_chunks.front();
				shared_ptr<Chunk> chunk = pair.first;
				ChunkType desired_type = pair.second;

				unordered_set<shared_ptr<Block>> blocks_to_spawn;
				if (this->genType == GenType::TERRAIN) {
					string next_block_tag = chunk->stringTagNoY();

					// determine which chunktype we want to spawn next
					ChunkType current_type = this->chunk_to_current_type_in_world[next_block_tag];
					ChunkType next_type = this->determineNextType(desired_type, current_type);

					// spawn those blocks or pop off queue once done
					if (current_type == next_type) {
						this->finished_gen_chunks.pop_front();

						// if done but not to desired chunk type yet, add to back of chunks_to_gen
						if (current_type != desired_type) {
							this->finished_gen_chunks.push_back(pair);
						}
						continue;
					}
					else {

						/*
							instead:
							- check if children are spawned in (if so do nothing)
						*/


						// check if lower children are spawned in (16, 4, 1)
						bool children_spawned_in = false;
						if (next_type == START_TYPE) { // check if 16 is in world
							children_spawned_in = children_spawned_in || this->cubePool->blocks_in_world.find(chunk->stringTag()) != this->cubePool->blocks_in_world.end();
						}

						if (!chunk->subChunks.empty()) {
							shared_ptr<Chunk> subChunk = chunk->subChunks[0];
							if (next_type == START_TYPE || next_type == MID_TYPE) {  // check if 4 is in world
								children_spawned_in = children_spawned_in || this->cubePool->blocks_in_world.find(subChunk->stringTag()) != this->cubePool->blocks_in_world.end();
							}
							if (!subChunk->subChunks.empty()) {  // check if 1 is in world
								shared_ptr<Chunk> subsubChunk = subChunk->subChunks[0];
								children_spawned_in = children_spawned_in || this->cubePool->blocks_in_world.find(subsubChunk->stringTag()) != this->cubePool->blocks_in_world.end();
							}
						}

						// if no children are spawned in go ahead and spawn the block (clearing anything there)
						if (!children_spawned_in) {

							// despawn parents or existing children
							this->despawnChunk(chunk);
							this->despawn_surface_for_chunk(chunk);
			
							// collect downblocks for spawning
							if (next_type == START_TYPE) {
								addChunkAndDownBlocksToSpawn(blocks_to_spawn, chunk);
								--available_iterations;
							}
							else {

								// plant spawning: only for detailed terrain (chunk_1 / chunk_4),
								// one batch per chunk_16 so plants despawn with it
								if (PLANT_GEN_ON && !this->plant_gen->plants.empty()) {
									this->spawn_plants_for_chunk16(chunk, blocks_to_spawn);
								}
		
								for (int s = 0; s < chunk->subChunks.size(); ++s) {
		
									shared_ptr<Chunk> subChunk = chunk->subChunks[s];
									if (next_type == MID_TYPE) {
										addChunkAndDownBlocksToSpawn(blocks_to_spawn, subChunk);
										--available_iterations;
									}
									else if (next_type == END_TYPE) {
										for (int ss = 0; ss < subChunk->subChunks.size(); ++ss) {
											shared_ptr<Chunk> subSubChunk = subChunk->subChunks[ss];
											addChunkAndDownBlocksToSpawn(blocks_to_spawn, subSubChunk);
											--available_iterations;
										}
									}
								}
							}

							
						}

					}


					// set to next type
					this->chunk_to_current_type_in_world[next_block_tag] = next_type;
				}
				// spawn blocks
				if (this->genType == GenType::TERRAIN) {
					unordered_map<string, vector<shared_ptr<Block>>> groups;
					for(const shared_ptr<Block>& b : blocks_to_spawn) {
						string tag = Chunk::chunk16_group_id(b->position->x, b->position->z);
						groups[tag].push_back(b);
					}
					for (std::pair<const string, vector<shared_ptr<Block>>>& group : groups ) {
						shared_ptr<vector<shared_ptr<Block>>> group_blocks = std::make_shared<vector<shared_ptr<Block>>>(std::move(group.second));
						this->deferred_cube_pool->spawn_blocks(group.first, group_blocks);
					}
				}

				// alert river gen
				vector<shared_ptr<Block>> block_change_report;
    			std::copy(blocks_to_spawn.begin(), blocks_to_spawn.end(), std::back_inserter(block_change_report));
				if (this->river_gen)
					this->river_gen->report_chunk_change(block_change_report);


				// print when done
				if (this->finished_gen_chunks.size() % 100 == 0) cout << this->finished_gen_chunks.size() << " chunks left to spawn" << endl;
			}
			else {
				// gen_mem_efficiency_test();
				break;
			}


		}

		// DESPAWN chunks no longer in range (any 16 chunk and descendants outside of the spawn range)
		if (!this->chunks_to_despawn.empty()) {

			if (this->genType == GenType::TERRAIN) {

				if (this->despawn_iterator == this->chunks_to_despawn.end()) this->despawn_iterator = this->chunks_to_despawn.begin();
				shared_ptr<Chunk> chunk_to_despawn = *this->despawn_iterator;

				// despawn if at despawn distance
				int dist = CoordinateConversion::cheapWrapMaxDistance(
					this->player_position_snapshot->x, 
					this->player_position_snapshot->z, 
					chunk_to_despawn->position->x, 
					chunk_to_despawn->position->z,
					planet->worldSize
				);
				bool is_at_despawn_distance = dist > DESPAWN_DISTANCE;
				if (is_at_despawn_distance) {
					this->despawn_iterator = this->chunks_to_despawn.erase(this->despawn_iterator);
		
					// despawn blocks
					despawnChunk(chunk_to_despawn);

				}
				// otherwise skip to revisit later
				else {
					this->despawn_iterator++;
				}
			}
		}

		if (!this->surface_chunks_to_despawn.empty()) {
			shared_ptr<Chunk> surface_chunk = *this->surface_chunks_to_despawn.begin();
			this->surface_chunks_to_despawn.erase(surface_chunk);
			string group_id = surface_chunk->stringTagNoY();
			this->deferred_cube_pool->despawn_blocks(group_id);
			this->active_surface_groups.erase(group_id);
		}
		
		// check if we're done spawning
		if (this->chunks_to_gen.empty() && this->finished_gen_chunks.empty() &&
			this->surface_chunks_to_gen.empty() && !this->spawnDone) {
			this->spawnDone = true;
			// cout << "blocks spawned " << this->cubePool->blocks_spawned << endl;
			// Chunk::dump_to_string(0, 0, this->planet);
		} 
	}

	/*
		Ensures all necessary parent neighbors are loaded in before we gen this chunk, as that is necessary
		for rivers and erosion etc..

		If not loaded in then thaat parent neighbor will be loaded one iteration. This may need to be repeated
		in order to load them all the way down to the right level. True will be returned once that's the case.
	*/
	bool genOrGenBlocker(shared_ptr<Chunk> chunk) {

		if (chunk->getType() == ChunkType::CHUNK_1) return true;

		if (chunk->initialized ) {
			if (chunk->subChunksInitialized) return true;
		}

		shared_ptr<Chunk> chunk_that_needs_to_be_gen_first = chunk->genSubChunks(this->player_position_snapshot, this->planet);

		// if we need to gen a chunk first, do that
		if (chunk_that_needs_to_be_gen_first != nullptr) {

			// figure out which chunk needs to be gen first, and then gen it and put children in all chunks
			while (chunk_that_needs_to_be_gen_first != nullptr) {
				shared_ptr<Chunk> gen_first = chunk_that_needs_to_be_gen_first->genSubChunks(this->player_position_snapshot, this->planet);
				if (gen_first == nullptr) {
					chunk_that_needs_to_be_gen_first = nullptr;
				}
				else {
					chunk_that_needs_to_be_gen_first = gen_first;
				}
			}
		}

		return false;
	}

	bool addChunkAndDownBlocksToSpawn(unordered_set<shared_ptr<Block>>& blocks_to_spawn, shared_ptr<Chunk> chunk) {
		bool was_set_to_spawn = false;
		if (this->genType == GenType::TERRAIN) {


			if (chunk->substrate_depth > 0) {
				chunk->material = chunk->substrate_material;
			}

			// add to spawn and do downblocks
			was_set_to_spawn = addBlockToSpawn(blocks_to_spawn, chunk);
			if (was_set_to_spawn) {
				determineDownBlocksAndNeededNeighborDownBlocks(blocks_to_spawn, chunk, chunk->substrate_material, chunk->substrate_depth);
			}
		}
		else if (this->genType == GenType::SPACE) {
			was_set_to_spawn = addBlockToSpawn(blocks_to_spawn, chunk);
		}

		return was_set_to_spawn;
	}

	/**
	 * Add the plants that land inside a chunk_16 to the spawn batch so they ride
	 * the same chunk16 group as the terrain and despawn with it. Placements were
	 * partitioned into this chunk_16 during terrain generation (512 -> 64 -> 16).
	 */
	void spawn_plants_for_chunk16(shared_ptr<Chunk>& chunk, unordered_set<shared_ptr<Block>>& blocks_to_spawn) {
		if (!this->planet || !chunk) return;
		if (chunk->getType() != ChunkType::CHUNK_16) return;

		// use this chunk_16's surface height - 6 for the plant origin
		int plant_y = chunk->position->y;

		for (const PlantPlacement& placement : chunk->plant_placements) {
			if (!placement.position) continue;
			if (placement.species < 0 ||
				(size_t)placement.species >= this->plant_gen->plants.size()) continue;
			// load the blocks from the plant design and apply transforms
			shared_ptr<Position> plant_pos = make_shared<Position>(
				placement.position->x, plant_y, placement.position->z);
			shared_ptr<Plant> plant = this->plant_gen->plants[placement.species][placement.type];
			shared_ptr<Plant> translated = plant->translate(plant_pos);
			for (const shared_ptr<Block>& b : *translated->blocks) {
				this->addBlockToSpawn(blocks_to_spawn, b);
			}
		}
	}

	bool addBlockToSpawn(unordered_set<shared_ptr<Block>>& blocks_to_spawn, shared_ptr<Block> block) {
		// int64_t id = block->getId();
		if (this->genType == GenType::TERRAIN) {
			if (this->cubePool->blocks_in_world.find(block->stringTag()) == this->cubePool->blocks_in_world.end()) {
				blocks_to_spawn.insert(block);
				this->cubePool->blocks_in_world[block->stringTag()] = block;
				return true;
			}
		}
		return false;
	}

	void despawnChunk(shared_ptr<Chunk> chunk, bool mark_only=false) {

		// remove chunk
		this->cubePool->blocks_in_world.erase(chunk->stringTag());
		
		// remove down blocks
		for (shared_ptr<DownBlock> downBlock : chunk->downBlocks) {
			this->cubePool->blocks_in_world.erase(downBlock->stringTag());
		}

		// despawn chunk and downblocks (and maybe some nieghbor blocks)
		if (!mark_only) {
			this->deferred_cube_pool->despawn_blocks(chunk->stringTagNoY());
		}

		// remove sub chunks as well
		if (!chunk->subChunks.empty()) {
			for (shared_ptr<Chunk> sub_chunk : chunk->subChunks) {
				this->despawnChunk(sub_chunk, true);
			}
		}

		// alert river simulations
		if (this->river_gen)
			this->river_gen->report_chunk_change(chunk->position->x, chunk->position->z);
	}


	void determineDownBlocksAndNeededNeighborDownBlocks(unordered_set<shared_ptr<Block>>& blocks_to_spawn, shared_ptr<Chunk>& chunk, string substrate_material, int substrate_depth) {
		// down blocks
		ChunkType type = chunk->getType();
		vector<shared_ptr<Chunk>> neighbors_in_world = planet->rootChunk->getNESWNeighborsInitialized(chunk->getType(), chunk->position);
		for (shared_ptr<DownBlock> dBlock : chunk->determineDownBlocks(neighbors_in_world, planet)) {
			// set to substrate material if not too deep
			if (chunk->position->y - dBlock->position->y <= substrate_depth) {
				dBlock->material = substrate_material;
			}
			addBlockToSpawn(blocks_to_spawn, dBlock);
		}

		// lake blocks
		for (shared_ptr<WaterBlock> dBlock : chunk->waterBlocks) {
			addBlockToSpawn(blocks_to_spawn, dBlock);
		}
	}

	ChunkType determineNextType(ChunkType desired_type, ChunkType current_type) {

		if (current_type == desired_type) return current_type;

		if (current_type == ChunkType::NONE) return START_TYPE;

		// CHUNK1 CHUNK4 CHUNK16
		// CHUNK16 CHUNK1 CHUNK4
		for (ChunkType type : this->priority_order) {
			if (type < current_type && type >= desired_type) {
				return type;
			}
		}
		
		return current_type;
	}

	void resetDesiredChunks() {
			
		if (this->genType == GenType::TERRAIN) {

			// reset desired_chunks and populate with new positions
			this->chunks_to_gen.clear();
			this->finished_gen_chunks.clear();
			this->surface_chunks_to_gen.clear();
			this->surface_chunks_to_despawn.insert(this->desired_surface_chunks.begin(), this->desired_surface_chunks.end());
			this->desired_surface_chunks.clear();
			this->chunk_to_current_type_in_world.clear();
			this->chunks_to_despawn.insert(this->desired_chunks.begin(), this->desired_chunks.end());
			this->desired_chunks.clear();
			this->spawnDone = false;

			int64_t render_distance_64 = Chunk::RENDER_DISTANCES[ChunkType::CHUNK_64];
			int64_t render_distance_16 = Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16];
			int64_t render_distance_4 = Chunk::RENDER_DISTANCES[ChunkType::CHUNK_4];
			int64_t render_distance_1 = Chunk::RENDER_DISTANCES[ChunkType::CHUNK_1];


			// Start below the root's adaptive base tier, then fill every lower tier.
			ChunkType base_type = this->planet->rootChunk->topType();
			const ChunkType terrain_types[] = {
				ChunkType::CHUNK_134217728,
				ChunkType::CHUNK_16777216,
				ChunkType::CHUNK_2097152,
				ChunkType::CHUNK_131072,
				ChunkType::CHUNK_8192,
				ChunkType::CHUNK_512,
				ChunkType::CHUNK_64,
				ChunkType::CHUNK_16,
				// ChunkType::CHUNK_4,
				// ChunkType::CHUNK_1
			};
			for (ChunkType type : terrain_types) {
				if (level(type) <= level(base_type)) continue;
				int64_t render_distance = render_distance_64;
				this->makeEmptySubchunkForType(type, render_distance);
			}


			// mark desired chunks
			int64_t start_x = -render_distance_16;
			int64_t start_z = -render_distance_16;
			int64_t end_x = render_distance_16;
			int64_t end_z = render_distance_16;
			int step = (int) ChunkType::CHUNK_16;
			vector<shared_ptr<Chunk>> chunk16s;
			vector<shared_ptr<Chunk>> chunk4s;
			vector<shared_ptr<Chunk>> chunk1s;
			for (int64_t x = start_x; x <= end_x; x += step) {
				for (int64_t z = start_z; z <= end_z; z += step) {

					// wrap position
					int64_t x_wrap = this->player_position_snapshot->x + x;
					int64_t z_wrap = this->player_position_snapshot->z + z;
					CoordinateConversion::wrap(x_wrap, z_wrap, planet->worldSize);

					// determine desired level
					shared_ptr<Chunk> chunk = planet->rootChunk->getChunk(ChunkType::CHUNK_16, x_wrap, z_wrap);
					int64_t right_render_dist = Chunk::render_distance_for_position(x_wrap, z_wrap, this->player_position_snapshot->x, this->player_position_snapshot->z, this->planet);
					// if (std::abs(x) < render_distance_1 && std::abs(z) < render_distance_1)
					// 	chunk1s.push_back(chunk);
					// else if(std::abs(x) < render_distance_4 && std::abs(z) < render_distance_4) 
					// 	chunk4s.push_back(chunk);
					// else 
					// 	chunk16s.push_back(chunk);
					if (right_render_dist == render_distance_1)
						chunk1s.push_back(chunk);
					else if(right_render_dist == render_distance_4) 
						chunk4s.push_back(chunk);
					else 
						chunk16s.push_back(chunk);
		
					this->desired_chunks.insert(chunk);
					this->chunks_to_despawn.erase(chunk);
				}
			}

			// Keep a coarse 512 surface outside the detailed terrain radius.
			const int surface_distance = 10000;
			const int surface_step = (int)ChunkType::CHUNK_512;
			this->makeEmptySubchunkForType(ChunkType::CHUNK_512, surface_distance);
			for (int64_t x = -surface_distance; x <= surface_distance; x += surface_step) {
				for (int64_t z = -surface_distance; z <= surface_distance; z += surface_step) {
					if (max(abs(x), abs(z)) <= render_distance_16) continue;
					int64_t x_wrap = this->player_position_snapshot->x + x;
					int64_t z_wrap = this->player_position_snapshot->z + z;
					CoordinateConversion::wrap(x_wrap, z_wrap, planet->worldSize);
					shared_ptr<Chunk> surface_chunk = planet->rootChunk->getChunk(ChunkType::CHUNK_512, x_wrap, z_wrap);
					if (!surface_chunk) continue;
					this->desired_surface_chunks.insert(surface_chunk);
					this->surface_chunks_to_despawn.erase(surface_chunk);
					if (this->active_surface_groups.find(surface_chunk->stringTagNoY()) == this->active_surface_groups.end())
						this->surface_chunks_to_gen.push_back(surface_chunk);
				}
			}
			this->despawn_iterator = this->chunks_to_despawn.end();




			// determine priority
			shared_ptr<Chunk> chunk = this->planet->rootChunk->getChunkOrClosestAncestorInWorld(ChunkType::CHUNK_1, this->player_position_snapshot->x, this->player_position_snapshot->z, this->cubePool->blocks_in_world);
			if (this->player_position_snapshot->y - chunk->position->y > 200) {
				this->priority_order = {ChunkType::CHUNK_16, ChunkType::CHUNK_4, ChunkType::CHUNK_1};
			}
			else {
				this->priority_order = {ChunkType::CHUNK_1, ChunkType::CHUNK_4, ChunkType::CHUNK_16};
			}



			// add to chunks_to_gen in priority order
			for (ChunkType type : this->priority_order) {
				if (type == ChunkType::CHUNK_16) {
					for (shared_ptr<Chunk> chunk16 : chunk16s) {

						this->chunks_to_gen.push_back(
							std::make_pair(shared_ptr<Chunk>(chunk16), ChunkType::CHUNK_16)
						);
					}
				}
				else if (type == ChunkType::CHUNK_4) {
					for (shared_ptr<Chunk> chunk4 : chunk4s) {
						this->chunks_to_gen.push_back(
							std::make_pair(shared_ptr<Chunk>(chunk4), ChunkType::CHUNK_4)
						);
					}
				}
				else if (type == ChunkType::CHUNK_1) {
					for (shared_ptr<Chunk> chunk1 : chunk1s) {
						this->chunks_to_gen.push_back(
							std::make_pair(shared_ptr<Chunk>(chunk1), ChunkType::CHUNK_1)
						);
					}
				}
			}

		}
	}

	void set_surface_corner_heights(const shared_ptr<Chunk>& chunk) {
		vector<shared_ptr<Chunk>> neighbors = this->planet->rootChunk->getChunkNeighbors_N_E_S_W_NE(
			ChunkType::CHUNK_512, chunk->position
		);
		chunk->surface_sw_height = chunk->position->y;
		chunk->surface_nw_height = neighbors[0] ? neighbors[0]->position->y : chunk->position->y;
		chunk->surface_ne_height = neighbors[4] ? neighbors[4]->position->y : chunk->position->y;
		chunk->surface_se_height = neighbors[1] ? neighbors[1]->position->y : chunk->position->y;
	}

	void despawn_surface_for_chunk(const shared_ptr<Chunk>& chunk) {
		shared_ptr<Chunk> surface_chunk = this->planet->rootChunk->getChunk(
			ChunkType::CHUNK_512, chunk->position->x, chunk->position->z
		);
		if (!surface_chunk) return;
		string group_id = surface_chunk->stringTagNoY();
		if (this->active_surface_groups.erase(group_id) > 0)
			this->deferred_cube_pool->despawn_blocks(group_id);
		this->desired_surface_chunks.erase(surface_chunk);
		this->surface_chunks_to_despawn.erase(surface_chunk);
	}

	void replace_in_desired_chunks(shared_ptr<Chunk> chunk, unordered_map<string, vector<shared_ptr<Chunk>>>& group_id_to_chunks) {

		for (string group_id : chunk->space_group_ids) {
			vector<shared_ptr<Chunk>>& group = group_id_to_chunks[group_id];
			for (shared_ptr<Chunk>& e_chunk : group) {

				if (chunk->type != e_chunk->type) {

					vector<Vector3> new_space_positions;
					vector<string> new_group_ids;
					for (int i = 0; i < e_chunk->space_group_ids.size(); ++i) {
						if (group_id != e_chunk->space_group_ids[i]) {
							new_space_positions.push_back(e_chunk->space_positions[i]);
							new_group_ids.push_back(e_chunk->space_group_ids[i]);
						}
					}

					if (new_space_positions.empty()) {
						this->desired_chunks.erase(e_chunk);
					}
					else {
						e_chunk->space_positions = new_space_positions;
						e_chunk->space_group_ids = new_group_ids;
					}

				}
			}

			group_id_to_chunks[group_id].push_back(chunk);
		}




	}

	void makeEmptySubchunkForType(ChunkType desired_type, int render_distance) {
		/**
		 * 
		 */

		int64_t start_x = -render_distance;
		int64_t start_z = -render_distance;
		int64_t end_x = render_distance;
		int64_t end_z = render_distance;
		ChunkType parent_type = larger(desired_type);
		int step = (int) parent_type;
		if (step > render_distance) step = render_distance;

		for (int64_t x = start_x; x <= end_x; x += step) {
			for (int64_t z = start_z; z <= end_z; z += step) {
				int64_t x_wrap = this->player_position_snapshot->x + x;
				int64_t z_wrap = this->player_position_snapshot->z + z;
				CoordinateConversion::wrap(x_wrap, z_wrap, planet->worldSize);
				shared_ptr<Chunk> chunk = planet->rootChunk->getChunk(parent_type, x_wrap, z_wrap);
				if (!chunk) continue;
				if (chunk->subChunks.empty() && chunk->getType() > ChunkType::CHUNK_1) {
					int square = chunk->getSquareOfSub();
					int numSubChunks = square * square;
					chunk->makeEmptySubChunks(square, numSubChunks, planet);	
				}
			}
		}
	}

	shared_ptr<Chunk> makeEmptySpaceSubchunks(ChunkType desired_type, int64_t x, int64_t z) {
		ChunkType top = this->planet->rootChunk->topType();
		vector<Offset> path = Chunk::getPath(desired_type, x, z, top);
		shared_ptr<Chunk> current_chunk = this->planet->rootChunk;
		int64_t chunk_level = level(desired_type) - level(top) + 1;
		for (int64_t i = 0; i < chunk_level; i++) {
			if (current_chunk->subChunks.empty() && current_chunk->getType() > ChunkType::CHUNK_1) {
				int square = current_chunk->getSquareOfSub();
				int numSubChunks = square * square;
				current_chunk->makeEmptySubChunks(square, numSubChunks, planet);
			}
			current_chunk = current_chunk->getSubChunk(path[i]);
		}
		return current_chunk;
	}

	void report_chunk_change(int64_t x, int64_t z) {
		if (this->river_gen)
			this->river_gen->report_chunk_change(x, z);
	}





	// SPACE TERRAIN GEN

	/* Generates chunks for the visible hemisphere of a target planet
	   (SKY mode) so SkyGen can sample terrain heights via
	   getChunkOrClosestAncestorInitialized for ray-sphere rendering.
	   Returns {viewer_pos, planet_center, real_dist_to_planet_m}. */
	tuple<Vector3, Vector3, int64_t> setup_skybox_planet_view(
		shared_ptr<StellarCoordinate> player_coord,
		shared_ptr<StellarCoordinate> planet_coord,
		shared_ptr<Planet> target_planet
	) {
		cout << "setup_skybox_planet_view: start" << endl;
		this->iterations = 1000;

		// Distance in km between player and planet
		int64_t dist_km = Util::dist(
			player_coord->km_x, player_coord->km_y, player_coord->km_z,
			planet_coord->km_x, planet_coord->km_y, planet_coord->km_z
		);
		dist_km *= 1000; // km -> m
		cout << "dist_km: " << dist_km << endl;

		SpaceGenLevel space_gen_level = CoordinateConversion::space_gen_level(dist_km);
		cout << "space_gen_level: " << (int)space_gen_level << endl;
		double game_dist = CoordinateConversion::game_scale(space_gen_level, dist_km);
		double game_radius = CoordinateConversion::game_scale(space_gen_level, (int64_t)target_planet->radius);

		// Position planet and viewer in local game space
		this->planet_center = Vector3(0, 0, -game_radius);
		this->player_location = this->planet_center + Vector3(0, 0, game_dist);

		// Switch to SKY gen mode
		this->genType = GenType::SKY;
		{
			lock_guard<mutex> lock(this->player_position_mutex);
			this->player_position_snapshot = std::make_shared<Position>(0, 1000, 0);
		}

		// Determine visible hemisphere chunks and gen them
		this->resetDesiredChunks();

		// Run the gen loop until all chunks are generated
		int safety = 0;
		while (!this->chunks_to_gen.empty() || !this->finished_gen_chunks.empty()) {
			if (safety++ > 100000) break;
			this->world_gen();
		}

		return {this->player_location, this->planet_center, dist_km};
	}


	string space_group_id(Vector3 offset_from_sphere_center, double radius, double circumferance) {
		pair<double, double> lat_long = CoordinateConversion::get_lat_and_long(offset_from_sphere_center);
		double current_lat_angle = lat_long.first;
		double current_long_angle = lat_long.second;

		return space_group_id(current_lat_angle, current_long_angle, radius, circumferance);
	}
	
	string space_group_id(double lat, double longi, double raidus, double circumferance) {
		int64_t chunk_per_circumference = ceil(circumferance / (double) START_TYPE);
		double chunk_angle = 2 * M_PI / chunk_per_circumference;

		double radius_at_y_level = cos(lat) * raidus;
		double circumferance_at_y_level = 2 * M_PI * radius_at_y_level;
		int64_t chunks_per_circumferance_at_y_level = ceil(circumferance_at_y_level / (int)START_TYPE);
		double chunk_angle_at_y_level = 2 * M_PI / Util::no_zero(chunks_per_circumferance_at_y_level);

		int chunk_lateral = round(lat / chunk_angle);
		int chunk_longitudinal = round(longi / chunk_angle_at_y_level);

		return Util::format("%d %d", chunk_lateral, chunk_longitudinal);
	}

	string type_size_space_group_tag(ChunkType type, string space_group_id) {
		return Util::format("%d %s", (int)type, space_group_id.c_str());
	}

	bool addBlockToSpawn(unordered_set<shared_ptr<Block>>& blocks_to_spawn, shared_ptr<Block> block, ChunkType chunk_type, string group_id) {

		if (this->genType == GenType::SPACE) {

			blocks_to_spawn.insert(block);
			string checker = type_size_space_group_tag(chunk_type, group_id);
			this->cubePool->blocks_in_world[checker] = block;
		}

		return true;
	}


	void despawnSpaceGroup(string space_group_id) {

		// remove chunk
		this->cubePool->blocks_in_world.erase(type_size_space_group_tag(START_TYPE, space_group_id));
		this->cubePool->blocks_in_world.erase(type_size_space_group_tag(MID_TYPE, space_group_id));
		this->cubePool->blocks_in_world.erase(type_size_space_group_tag(END_TYPE, space_group_id));
		
		// despawn chunk and downblocks (and maybe some nieghbor blocks)

		if (godot_enabled) {
			#ifdef GODOT_ENABLED
				callable_mp(this->deferred_cube_pool, &DeferredWorldBlockPool::despawn_blocks_deferred).call_deferred(GodotUtil::g_str(space_group_id)); // spawn on main game thread
			#endif
		}
		else {
			this->deferred_cube_pool->despawn_blocks_deferred(GodotUtil::g_str(space_group_id));
		}
	}



	// build (and cache) a vegetated texture: base material speckled with random
	// pixels sampled from the two leaf textures (preserving their natural colour
	// variation, skipping transparent gaps). deterministic per base+veg+leaves.
	static string make_vegetated_material(
		const string& base, int veg_level,
		const string& leaf_mat1, const string& leaf_mat2
	) {
		char tag[128];
		snprintf(tag, sizeof(tag), "%s_veg%d_%s_%s",
			base.c_str(), veg_level, leaf_mat1.c_str(), leaf_mat2.c_str());
		string mat_name = string(tag);

		if (MeshMaker::materials.count(mat_name)) return mat_name;

		// load the base texture pixels so the ground keeps its natural variation
		int tw, th, n;
		uint8_t* tex = stbi_load(BMaterial::path(base).c_str(), &tw, &th, &n, 4);
		if (!tex) return base;
		// load both leaf textures to sample real pixels from them
		int l1w, l1h, l2w, l2h;
		uint8_t* leaf1 = stbi_load(BMaterial::path(leaf_mat1).c_str(), &l1w, &l1h, &n, 4);
		uint8_t* leaf2;
		if (leaf_mat2 == leaf_mat1) { leaf2 = leaf1; l2w = l1w; l2h = l1h; }
		else leaf2 = stbi_load(BMaterial::path(leaf_mat2).c_str(), &l2w, &l2h, &n, 4);
		int square = MeshMaker::TEXTURE_FILE_SQUARE;
		int w = max(1, square); int h = max(1, square);

		// speckle density scales with the vegetation level
		double speckle = veg_level == 3 ? 0.45 : 0.22;
		PackedByteArray pba;
		pba.resize(w * h * 3);
		uint8_t* px = pba.ptrw();
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				int si = (y * w + x);
				// wrap into the real texture so small tiles still sample safely
				int src = ((y % max(1, th)) * tw + (x % max(1, tw))) * 4;
				double r = tex[src + 0] / 255.0;
				double g = tex[src + 1] / 255.0;
				double b = tex[src + 2] / 255.0;
				if (Random::randDouble(si * 7919 + veg_level * 31, 0.0, 1.0) < speckle) {
					// pick one of the two leaf textures, then sample a random
					// pixel from it, retrying a few times to skip transparent gaps
					int which = Random::randBool(si * 104729 + 1) ? 0 : 1;
					uint8_t* leaf = which == 0 ? leaf1 : leaf2;
					int lw = which == 0 ? l1w : l2w;
					int lh = which == 0 ? l1h : l2h;
					bool got = false;
					for (int t = 0; t < 6 && !got; ++t) {
						int tx = Random::randInt(si * 1543 + t * 31, 0, max(1, lw) - 1);
						int ty = Random::randInt(si * 1543 + t * 97 + 7, 0, max(1, lh) - 1);
						int li = (ty * lw + tx) * 4;
						if (leaf[li + 3] < 128) continue; // skip transparent
						r = leaf[li + 0] / 255.0;
						g = leaf[li + 1] / 255.0;
						b = leaf[li + 2] / 255.0;
						got = true;
					}
				}
				px[si * 3 + 0] = (uint8_t)(r * 255);
				px[si * 3 + 1] = (uint8_t)(g * 255);
				px[si * 3 + 2] = (uint8_t)(b * 255);
			}
		}
		stbi_image_free(tex);
		if (leaf2 != leaf1) stbi_image_free(leaf2);
		if (leaf1) stbi_image_free(leaf1);

		// cache the finished material so we don't re-speckle per block.
		// double-sided so the 512 surface patches (single flat quads) render from
		// both sides; cull_disabled is free for these quads.
		Ref<Image> img;
		img.instantiate();
		img->set_data(w, h, false, Image::FORMAT_RGB8, pba);
		Ref<ShaderMaterial> material = MeshMaker::make_material_from_image(img, mat_name, 1.0f, 0.0f, true);
		MeshMaker::materials[mat_name] = material;
		// also register under the double-sided key so get_material(name, true) hits
		// instead of trying to load a non-existent texture file
		MeshMaker::materials[mat_name + "_double_sided"] = material;
		return mat_name;
	}

	/**
	 * Assign the material for the block by doing some math. 
	 * 
	 * Explanation:
	 * - we get the player_world_pos with sphere_location_to_world_position() which does the fancy wrap of a square around a sphere
	 * - but since this shows visible folds and compression on the planet from space, we do something different here when assigning materials
	 * - we use player_world_pos and the block_pos (that should have almost the same magnitude) and figure how far they are roughly and just sample
	 * blocks off the flat world square with that
	 */
	string assign_block_material(
		Vector3& block_pos, 
		Vector3& player_dir, 
		Vector3& player_up,
		Vector3& player_right,
		shared_ptr<Position>& player_world_pos, 
		ChunkType desired_type
	) {

		// take dot of block_pos relative to player up and right (assumes all blocks on are side of the planet facing player), then use that to get world coordinates
		Vector3 dir = block_pos.normalized();
		double dot_up = (double)vec3_dot(dir, player_up);
		double dot_right = (double)vec3_dot(dir, player_right);
		int64_t half_world_size = planet->worldSize / 2;
		int64_t x = CoordinateConversion::wrap(player_world_pos->x + half_world_size * dot_up, planet->worldSize);
		int64_t z = CoordinateConversion::wrap(player_world_pos->z + half_world_size * dot_right, planet->worldSize);
		shared_ptr<Position> world_pos = make_shared<Position>(x, 0, z);

		return assign_block_material(world_pos, desired_type);
	}

	/**
	 * Shared material assignment once we already have a world position.
	 * Looks up the chunk containing world_pos, generates it if needed, then
	 * picks the substrate/rock material and bakes in plant-pixel vegetation.
	 * Used by both the space block-dir path and the terrain 512 surface patches.
	 */
	string assign_block_material(
		shared_ptr<Position>& world_pos,
		ChunkType desired_type
	) {

		shared_ptr<Chunk> chunk;
		if (this->rootChunk && world_pos) {
			chunk = this->makeEmptySpaceSubchunks(desired_type, world_pos->x, world_pos->z);
			shared_ptr<Chunk> generation_chunk = chunk->getParent();
			bool finished = generation_chunk ? genOrGenBlocker(generation_chunk) : true;
			while(!finished) finished = genOrGenBlocker(generation_chunk);
		}

		string mat;
		if (chunk) {
			// get substrate or rock material
			mat = chunk->substrate_depth > 0 ? chunk->substrate_material : chunk->material;
			
			// apply plant pixels and make texture if missing
			if (this->planet->hasLife) {
				double vegetation = chunk->vegetation_amount();

				// vegetation drives how much the ground is covered:
				//   < .2  -> bare ground, no plant pixels (level 1)
				//   < .7  -> moderate speckle (level 2)
				//   >= .7 -> near-complete coverage (level 3)
				int veg_level = vegetation < 0.2 ? 1 : (vegetation < 0.7 ? 2 : 3);

				if (veg_level >= 2) {
					// find the 2 most common plant species by ratio weight
					int species1 = -1, species2 = -1;
					double w1 = -1.0, w2 = -1.0;
					for (auto& [species, weight] : chunk->ratios_plant_types) {
						if (weight > w1) { w2 = w1; species2 = species1; w1 = weight; species1 = species; }
						else if (weight > w2) { w2 = weight; species2 = species; }
					}

					if (species1 >= 0 && (size_t)species1 < this->plant_gen->plants.size()) {
						// leaf material of the two species -> sampled for speckle pixels
						string leaf_mat1 = this->plant_gen->plants[species1][PlantType::FOREST]->leaf->material;
						string leaf_mat2 = species2 >= 0 &&
							(size_t)species2 < this->plant_gen->plants.size()
							? this->plant_gen->plants[species2][PlantType::FOREST]->leaf->material
							: leaf_mat1;

						mat = World::make_vegetated_material(mat, veg_level, leaf_mat1, leaf_mat2);
					}
				}
			}

		}

		return mat.empty() ? BMaterial::DEFAULT : mat;
	}

	vector<shared_ptr<Block>> generate_shell_blocks(int max_blocks, ChunkType desired_type) {
		vector<shared_ptr<Block>> blocks;
		blocks.reserve(max_blocks);

		// emit in the stellar LY frame: radius from meters, uniform marker size so
		// every shell block renders as a ~1-godot-unit cube (same as other space blocks)
		double R_ly = (double)this->planet->radius * LY_PER_M_WORLD;
		double block_size_ly = 1.0 / max(this->game_scale, 1e-12);
		Vector3 player_dir = -this->planet_to_player_ly;
		if (player_dir.length_squared() > 1e-24)
			player_dir = player_dir.normalized();
		else
			player_dir = Vector3(0, 1, 0);
		if (R_ly < 2.0 * block_size_ly) return blocks;
		shared_ptr<Position> world_pos = CoordinateConversion::sphere_location_to_world_position(player_dir, this->planet);

		int int_R = (int)ceil(R_ly / block_size_ly);

		vector<shared_ptr<Block>> ring;
		ring.reserve(2000);


		// DETERMINE y_min and y_max

		// determine angle to make visible cap
		double cap_angle = cap_angle_for_blocks(R_ly, block_size_ly, max_blocks);
		cap_angle = min(cap_angle, M_PI/2); // limit to pi/2 on either side to only render side facing player
		double cap_cos = cos(cap_angle);
		double cap_alignement = (1.0 - cap_cos);

		// DETERMINE  player up and right vectors
		Vector3 player_right = (abs(player_dir.x) != 0 || abs(player_dir.x) != 1 || abs(player_dir.x) != 0) ? vec3_cross(player_dir, Vector3(0,-1,0)).normalized() : Vector3(0,0,1);
		Vector3 player_up = (abs(player_dir.x) != 1 || abs(player_dir.x) != 0 || abs(player_dir.x) != 0) ? vec3_cross(player_dir, Vector3(1,0,0)).normalized() : Vector3(0,0,1);

		auto rot_on_axis = [&](Vector3 along_axis, Vector3 axis_start, double angle) {
			Vector3 rot_axis = vec3_cross(along_axis, axis_start);
			if (rot_axis.x == 0 && rot_axis.y == 0 && rot_axis.z == 0) return along_axis;
			rot_axis.normalize();
			return along_axis * cos(angle) + vec3_cross(rot_axis, along_axis) * sin(angle);
		};

		// determine y bounds by rotating player_dir by cap_angle up and down
		Vector3 pos_rot = rot_on_axis(player_dir, Vector3(0, 1, 0), cap_angle);
		Vector3 neg_rot = rot_on_axis(player_dir, Vector3(0, 1, 0), -cap_angle);
		int y_min = Util::min(pos_rot.y, neg_rot.y) * int_R;
		int y_max = Util::max(pos_rot.y, neg_rot.y) * int_R;
		double player_dir_angle = acos( clamp( (double)vec3_dot(player_dir, Vector3(0, 1, 0)), -1.0, 1.0) );
		if (player_dir_angle < M_PI) {
			if (player_dir_angle + cap_angle > M_PI) y_min = -int_R;
			else if (player_dir_angle - cap_angle < 0) y_max = int_R;
		}
		if (player_dir_angle > M_PI) {
			if (player_dir_angle + cap_angle > 2*M_PI) y_max = int_R;
			else if (player_dir_angle - cap_angle < M_PI) y_min = -int_R;
		}

		// Match the cap radius used to select the surface terrain area.
		Vector3 player_dir_on_planet_sphere = player_dir * int_R; 
		double cap_sphere_radius = cap_sphere_radius_for_blocks(R_ly, block_size_ly, max_blocks) / block_size_ly;
		

		// iterate on y bounds
		int last_r = 0;
		for (int y = y_min; y <= y_max; y++) {
			double r_sq = R_ly * R_ly - ((double)y * block_size_ly) * ((double)y * block_size_ly);
			if (r_sq <= 0) continue;
			int r = (int)ceil(sqrt(r_sq) / block_size_ly);
			if (r < 1) continue;

			ring.clear();

			auto filter = [&](double x, double y, double z) {
				Vector3 diff = player_dir_on_planet_sphere - Vector3(x, y, z);
				return diff.length() > cap_sphere_radius;
			};
			PartContour::add_blocks_with_midpoint_circle_algorithm(
				y, r, r, block_size_ly, 0, 0, ring, "y", max(1, abs(r - last_r)), filter
			);
			last_r = r;


			for (auto& block : ring) {

				// The contour is generated around the local planet origin. Keep that
				// offset for emission, but sample terrain at the absolute planet point.
				Vector3 local_pos(
					block->position_double->x,
					block->position_double->y,
					block->position_double->z
				);
				Vector3 block_dir = local_pos.normalized();
				double alignment = clamp((double)player_dir.dot(block_dir), 0.0, 1.0);
				if (alignment < cap_cos) continue; // player_dir.dot(block_dir) = cos(angle_between_player_and_block)

				string material = assign_block_material(local_pos, player_dir, player_up, player_right, world_pos, desired_type);
				if (material.empty()) continue;

				block->material = material;
				block->position_double->x += this->planet_to_player_ly.x;
				block->position_double->y += this->planet_to_player_ly.y;
				block->position_double->z += this->planet_to_player_ly.z;
				blocks.push_back(block);

			}

			if (y % 20 == 0) {
				cout << y << " ring level complete of " << y_min << " to " << y_max << endl;
				cout << "mem used " << MemInfo::system_ram_percent_used() << endl;
			}
		}

		return blocks;
	}

	vector<shared_ptr<Block>> generate_voxel_planet(int max_blocks = 500000) {
		if (!this->rootChunk || !this->planet) return {};

		double block_size_ly = 1.0 / max(this->game_scale, 1e-12);
		ChunkType desired_type = closest_chunk_type(block_size_ly / LY_PER_M_WORLD);
		ChunkType top = this->rootChunk->topType();
		if ((int)desired_type > (int)top)
			desired_type = top;

		vector<shared_ptr<Block>> blocks = generate_shell_blocks(max_blocks, desired_type);
		if (blocks.empty()) return {};
		// Timer::stop("generating blocks"); 0.00178099s
		cout << blocks.size() << endl;

		return blocks;
	}



	// TESTS

	void gen_mem_efficiency_test() {

		vector<shared_ptr<Chunk>> to_explore = {this->rootChunk};
		double in_world = 0;
		double total = 0;
		unordered_set<ChunkType> c_types = {ChunkType::CHUNK_16, ChunkType::CHUNK_4, ChunkType::CHUNK_1};
		while(!to_explore.empty()) {
			shared_ptr<Chunk> chunk = to_explore.back();
			to_explore.pop_back();

			if (c_types.find(chunk->getType()) != c_types.end()) {
				if (this->cubePool->blocks_in_world.find(chunk->stringTag()) != this->cubePool->blocks_in_world.end()) {
					in_world++;
				}
				else {
					if (chunk->subChunks.empty()) {
						total++;
					}
				}
			}

			for (shared_ptr<Chunk> chunk : chunk->subChunks) {
				to_explore.push_back(chunk);
			}
		}

		Util::print("precent used: ", in_world/total * 100.0);
	}
};
