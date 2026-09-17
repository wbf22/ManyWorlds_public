// Fill out your copyright notice in the Description page of Project Settings.util/Offset.h

#pragma once
// #pragma message("Compiling WorldBlockPool.hpp")


#include <unordered_map>
#include <functional>

#include "util/Offset.h"
#include "util/Position.h"
#include <stack>
#include "util/CoordinateConversion.hpp"
#include "util/concurrent_map.hpp"
#include "util/concurrent_vector.hpp"
#include "../chunks/DownBlock.h"
#include "../chunks/Chunk.h"
#include "../plants/Leaf.hpp"
#include "util/BMaterial.hpp"
#include <unordered_map>
#include "../chunks/Chunk.h"
#include "MeshMaker.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/classes/static_body3d.hpp>



template<typename T>
using uptr = std::unique_ptr<T>;


// UE_DISABLE_OPTIMIZATION

using namespace std;

class Chunk;


/**
 * 
 */
class WorldBlockPool
{
public:

	shared_ptr<Planet> planet;
	shared_ptr<Position> terrainZeroZero; // what position should be rendered at 0,0 in the UE5 scene
	Node* node;
	int blocks_spawned = 0; // num blocks currently in world
	function<void(int64_t,int64_t)> chunk16_changed_trigger;


	// for terrain blocks: group_id (chunk 16 xz) -> mesh_instances (one for each material in group)
	// Plant blocks:
	// player blocks: (0 xyz) of the block being place. Each block has their own Static Mesh Component
	concurrent_map<string, concurrent_vector<MultiMeshInstance3D*>> group_id_to_mesh_instances;




	// player placed blocks
	unordered_map<string, unordered_set<string>> chunk_16_x_z_to_added_block_x_y_z; // contains only blocks added by players or NPCs
	unordered_set<string> x_y_z_deleted_blocks; // contains only deleted blocks from terrain (deleted player blocks are just forgotten)


	// world gen blocks
	unordered_map<string, shared_ptr<Block>> blocks_in_world; // position sizexyz string to block (all blocks currently spawned in the world)




	WorldBlockPool(
		Node* node,
		shared_ptr<Position> terrainZeroZero,
		shared_ptr<Planet> planet,
		function<void(int64_t,int64_t)> chunk16_changed_trigger
	)
	{
		this->node = node;
		this->terrainZeroZero = terrainZeroZero;
		this->planet = planet;
		this->chunk16_changed_trigger = chunk16_changed_trigger;
	}

	~WorldBlockPool() { }



	// TERRAIN

	/**
	 * Spawns a group of the same type of block. Used by terrain generation to spawn chunk 16s all together 
	 * for easy deletion later
	 */
	void spawn_blocks(
		string group_id,
		const vector<shared_ptr<Block>>& blocks
	) {
		this->blocks_spawned += blocks.size();

		// determine which materials and sizes we need, and spawn 512 blocks
		unordered_map<string, vector<shared_ptr<Block>>> material_and_size_to_blocks;
		for (shared_ptr<Block> block : blocks) {
			if (!block->is_surface_patch) {
				string material = block->material;
				int size = block->getSize();
				string material_and_size = material + std::to_string(size);
				material_and_size_to_blocks[material_and_size].push_back(block);
			}
			else {

				shared_ptr<Chunk> chunk = static_pointer_cast<Chunk>(block);
				const int chunk_size = Chunk::CHUNK_SIZES[ChunkType::CHUNK_512];
				const int64_t x = chunk->position->x;
				const int64_t z = chunk->position->z;
				vector<shared_ptr<Position>> corners = {
					make_shared<Position>(x, chunk->surface_sw_height, z),
					make_shared<Position>(x + chunk_size, chunk->surface_se_height, z),
					make_shared<Position>(x + chunk_size, chunk->surface_ne_height, z + chunk_size),
					make_shared<Position>(x, chunk->surface_nw_height, z + chunk_size)
				};
				vector<Vector3> vertices;
				for (const shared_ptr<Position>& corner : corners) {
					vertices.push_back(CoordinateConversion::convert_position_to_fVector(
						corner, this->terrainZeroZero, this->planet->worldSize, 1
					));
				}

				PackedVector3Array mesh_vertices;
				PackedVector3Array normals;
				PackedInt32Array indices;
				Vector3 normal = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]).normalized();
				for (int i = 0; i < 4; ++i) {
					mesh_vertices.push_back(vertices[i]);
					normals.push_back(normal);
				}
				for (int i : {0, 2, 1, 0, 3, 2})
					indices.push_back(i);

				// repeat the texture per block-unit so the far surface reads textured
				// (the pixel shader needs UVs or the whole quad samples one texel)
				PackedVector2Array uvs;
				const double uv_scale = (double)Chunk::CHUNK_SIZES[ChunkType::CHUNK_512];
				for (int i = 0; i < 4; ++i) {
					double u = (i == 1 || i == 2) ? uv_scale : 0.0;
					double v = (i == 2 || i == 3) ? uv_scale : 0.0;
					uvs.push_back(Vector2(u, v));
				}

				Array arrays;
				arrays.resize(Mesh::ARRAY_MAX);
				arrays[Mesh::ARRAY_VERTEX] = mesh_vertices;
				arrays[Mesh::ARRAY_NORMAL] = normals;
				arrays[Mesh::ARRAY_INDEX] = indices;
				arrays[Mesh::ARRAY_TEX_UV] = uvs;
				Ref<ArrayMesh> mesh;
				mesh.instantiate();
				mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

				vector<Vector3> origin = {Vector3()};
				MultiMeshInstance3D* instance = MeshMaker::spawn_multimesh_instances(
					this->node, MeshMaker::get_material(chunk->material, true), mesh, origin, nullptr, 1
				);
				this->group_id_to_mesh_instances[group_id].push_back(instance);

			}
		}

#ifdef GODOT_ENABLED
		// for each material and size, make a new UInstancedStaticMeshComponent and batch spawn the blocks
		for (auto& [material_size_string, blocks_this_mat] : material_and_size_to_blocks) {
			string material = blocks_this_mat[0]->material;
			int size = blocks_this_mat[0]->getSize();
			
			// get material
			Ref<ShaderMaterial> godot_material = MeshMaker::get_material(material);

			// get positions
			vector<Vector3> positions;
			for (shared_ptr<Block> block : blocks_this_mat) {

				// get position
				Vector3 pos = CoordinateConversion::convert_position_to_fVector(
					block->position,
					this->terrainZeroZero,
					this->planet->worldSize,
					block->getSize()
				);
				positions.push_back(pos);
			}
			
			// make collider container
			StaticBody3D* collider_container = memnew(StaticBody3D);
			
			// spawn instances
			MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
				this->node,
				godot_material,
				MeshMaker::meshes[size],
				positions,
				collider_container,
				size
			);

			// save it in the map for deleting
			this->group_id_to_mesh_instances[group_id].push_back(mm_instance);
		}
#endif


	}

	/**
	 * Calls the above method but first sorts blocks into chunk16 groups
	 */
	void spawn_blocks(const unordered_set<shared_ptr<Block>>& blocks) {

		// sort into chunk 16 groups
		unordered_map<string, vector<shared_ptr<Block>>> groups;
		for(const shared_ptr<Block>& b : blocks) {
			string chunk_16_xz = Chunk::chunk16_group_id(b->position->x, b->position->z);
			groups[chunk_16_xz].push_back(b);
		}
		for (pair<string, vector<shared_ptr<Block>>> group : groups ) {
			this->spawn_blocks(
				group.first,
				group.second
			);
		}
	}

	/**
	 * Spawns a group of space blocks at position_double + offset.
	 * Space positions are already in the common Godot frame by this point.
	 */
	void spawn_space_blocks(
		string group_id,
		const vector<shared_ptr<Block>>& blocks,
		Vector3 offset,
		double game_scale
	) {
		this->blocks_spawned += blocks.size();

		unordered_map<string, vector<shared_ptr<Block>>> material_and_size_to_blocks;
		for (shared_ptr<Block> block : blocks) {
			double render_size = block->render_size > 0.0 ? block->render_size : block->size * game_scale;
			string material_and_size = block->material + std::to_string(render_size);
			material_and_size_to_blocks[material_and_size].push_back(block);
		}

#ifdef GODOT_ENABLED
		for (auto& [material_size_string, blocks_this_mat] : material_and_size_to_blocks) {
			string material = blocks_this_mat[0]->material;
			double size = blocks_this_mat[0]->size;

			Ref<ShaderMaterial> godot_material = MeshMaker::get_material(material);

			vector<Vector3> positions;
			positions.reserve(blocks_this_mat.size());
			for (shared_ptr<Block> block : blocks_this_mat) {
				shared_ptr<PositionDouble> p = block->position_double;
				if (p == nullptr) p = block->position->to_position_double();
				positions.push_back(Vector3(p->x, p->y, p->z) + offset);
			}

			// 3D object blocks can use an object-specific size; older space markers
			// still fall back to the common frame conversion.
			double render_size = blocks_this_mat[0]->render_size;
			if (render_size <= 0.0) render_size = size * game_scale;
			// repeat the texture once across the whole block, regardless of size
			Ref<Mesh> mesh = MeshMaker::make_cube((float)render_size, "", (float)render_size);
			// Ref<Mesh> mesh = MeshMaker::make_cube((float)render_size);			

			// no collider container — space blocks get no hit box
			MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
				this->node,
				godot_material,
				mesh,
				positions,
				nullptr,
				render_size
			);

			this->group_id_to_mesh_instances[group_id].push_back(mm_instance);
		}
#endif
	}

	void despawn_blocks(
		string group_id
	) {
#ifdef GODOT_ENABLED
		// destroy each instance
		for (MultiMeshInstance3D* mesh : this->group_id_to_mesh_instances[group_id]) {
			mesh->queue_free();
			this->blocks_spawned -= mesh->get_multimesh()->get_instance_count();
		}
		this->group_id_to_mesh_instances.erase(group_id);
#endif
		
	}


	static string materialSizeString(string material, int size)
	{
		return material + std::to_string(size);
	}



	// PLAYER

	void place_block(shared_ptr<PositionDouble>& position, double size, const Ref<Mesh>& selected_block, const Ref<Material>& material, bool player_edit) {
		
		// PLACE BLOCK

		// get position
		vector<Vector3> positions;
		Vector3 pos = CoordinateConversion::convert_position_double_to_fVector(
			position,
			this->terrainZeroZero,
			this->planet->worldSize,
			size
		);
		positions.push_back(pos);

		// make collider container
		StaticBody3D* collider_container = memnew(StaticBody3D);
		
		// spawn instances
		MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
			this->node,
			material,
			MeshMaker::meshes[size],
			positions,
			collider_container,
			size
		);


		// ADD TO LISTS
		string chunk_16_x_z = Chunk::chunk16_group_id(position->x, position->z);

        
		if (player_edit) {
			string block_x_y_z = Block::stringTag_double(0, position->x, position->y, position->z);
			this->group_id_to_mesh_instances[block_x_y_z].push_back(mm_instance);
			this->chunk_16_x_z_to_added_block_x_y_z[chunk_16_x_z].insert(block_x_y_z);
			cout << "placed block at " << block_x_y_z << endl;
		}
		else {
			this->group_id_to_mesh_instances[chunk_16_x_z].push_back(mm_instance);
		}
		
		
	}

	// place a block with no collision, no persistence — for ghost preview
	MultiMeshInstance3D* place_block_no_collision(
		shared_ptr<PositionDouble>& position, double size,
		const Ref<Mesh>& mesh, const Ref<Material>& material
	) {
		Vector3 pos = CoordinateConversion::convert_position_double_to_fVector(
			position, this->terrainZeroZero, this->planet->worldSize, size
		);
		vector<Vector3> positions = {pos};
		return MeshMaker::spawn_multimesh_instances(
			this->node, material, MeshMaker::meshes[size], positions, nullptr, size
		);
	}

	void delete_block(shared_ptr<Block>& block) {
		shared_ptr<PositionDouble> pos = block->position_double;
		if (pos == nullptr) {
			pos = block->position->to_position_double();
		}

		this->delete_block(pos);
	}

	void delete_block(shared_ptr<PositionDouble>& position) {

		MultiMeshInstance3D* mesh = nullptr;
		int index = -1;
		bool player_placed = false;
		this->get_mesh_and_index(mesh, index, player_placed, position);
		if (mesh != nullptr) {

			Ref<MultiMesh> mm = mesh->get_multimesh();
			string block_x_y_z = Block::stringTag_double(0, position->x, position->y, position->z);
			string chunk_16_x_z = Chunk::chunk16_group_id(position->x, position->z);
			if (player_placed) {
				mesh->queue_free();
				this->chunk_16_x_z_to_added_block_x_y_z[chunk_16_x_z].erase(block_x_y_z);
				this->group_id_to_mesh_instances.erase(block_x_y_z);
				cout << "placed block destroyed at " << block_x_y_z << endl;
			}
			else {
				MeshMaker::remove_instance_from_multimesh(
					mesh,
					index
				);
				cout << "world block destroyed at " << position->toString() << endl;

				if (mm->get_instance_count() == 0) {
        			
					concurrent_vector<MultiMeshInstance3D*>& instances = this->group_id_to_mesh_instances[chunk_16_x_z];
					instances.erase(mesh);
					if (instances.empty()) {
						this->group_id_to_mesh_instances.erase(chunk_16_x_z);
					}
					mesh->queue_free();
				}
				this->x_y_z_deleted_blocks.insert(block_x_y_z);
			}
		}
		
	}

	vector<shared_ptr<Block>> get_blocks_in_range(shared_ptr<Position>& start, shared_ptr<Position>& end) {

		vector<shared_ptr<Block>> blocks;
		this->get_blocks_in_range(
			blocks,
			this->terrainZeroZero,
			this->planet->worldSize,
			start->x,
			start->y,
			start->z,
			end->x,
			end->y,
			end->z
		);

		return blocks;
	}

	void get_blocks_in_range(vector<shared_ptr<Block>>& blocks, shared_ptr<Position>& terrain_zero, int64_t world_size, double min_x, double min_y, double min_z, double max_x, double max_y, double max_z) {
		
		// get all blocks in range
		double start_x = Util::floor_to_precision(min_x, 16);
		double start_z = Util::floor_to_precision(min_z, 16);
		double end_x = Util::ceil_to_precision(max_x, 16) + 16;
		if (end_x > world_size) end_x -= world_size;
		double end_z = Util::ceil_to_precision(max_z, 16) + 16;
		if (end_z > world_size) end_z -= world_size;
		
		int64_t x = start_x;
		while(x != end_x) {
			
			int64_t z = start_z;
			while(z != end_z) {
				string chunk_16_tag = Chunk::chunk16_group_id(x, z);

				if (this->group_id_to_mesh_instances.contains(chunk_16_tag)) {
					concurrent_vector<MultiMeshInstance3D*>& terrain_meshes = this->group_id_to_mesh_instances[chunk_16_tag];
					vector<MultiMeshInstance3D*> meshes;
					for (MultiMeshInstance3D* mesh : terrain_meshes) {
						// meshes.push_back(mesh);
					}

					// player placed blocks
					if (this->chunk_16_x_z_to_added_block_x_y_z.find(chunk_16_tag) != this->chunk_16_x_z_to_added_block_x_y_z.end()) {
						unordered_set<string> blocks_in_chunk = this->chunk_16_x_z_to_added_block_x_y_z[chunk_16_tag];
						for(string block_x_y_z : blocks_in_chunk) {
							MultiMeshInstance3D* mesh = this->group_id_to_mesh_instances[block_x_y_z][0];
							meshes.push_back(mesh);
						}
					}

					for(MultiMeshInstance3D* mesh : meshes) {

						Ref<MultiMesh> mm = mesh->get_multimesh();
						int num_instances = mm->get_instance_count();
						for(int i = 0; i < num_instances; ++i) {

							shared_ptr<Block> block = std::make_shared<Block>();
							StaticBody3D* body = Object::cast_to<StaticBody3D>(mesh->get_child(0));
							CollisionShape3D* hit_collider = Object::cast_to<CollisionShape3D>(body->get_child(0));
							block->size = hit_collider->get_meta("size");
							Vector3 instance_location = mm->get_instance_transform(i).origin;
							block->position_double = CoordinateConversion::convert_fVector_to_position_double(
								instance_location, 
								terrain_zero, 
								world_size, 
								block->size
							);
							block->position = Position::from_position_double(block->position_double);
							if (
								CoordinateConversion::is_between(block->position_double->x, min_x, max_x) &&
								CoordinateConversion::is_between(block->position_double->y, min_y, max_y) &&
								CoordinateConversion::is_between(block->position_double->z, min_z, max_z)
							) {
								
								block->material = mesh->get_material_override().is_valid()? mesh->get_material_override()->get_name().utf8().get_data() : "";
								blocks.push_back(block);
							}
						}
					}
				}

				z+=16;
				if (z >= world_size && z != end_z) z -= world_size;
			}
			x += 16;
			if (x >= world_size && x != end_x) x -= world_size;
		}
	
	}

	void player_destroy_block(shared_ptr<Block> block) {

		if (this->is_terrain_block(block)) {

			// SPAWN BLOCKS to ensure player can't see below terrain
			// ** get chunk where we're deleting
			ChunkType type = (ChunkType) block->size;
			shared_ptr<Chunk> chunk = this->planet->rootChunk->getChunk(type, block->position_double->x, block->position_double->z);
			cout << "chunk pos " << chunk->stringTag() << endl;

			// ** gen downblocks underneath if needed
			unordered_set<shared_ptr<Block>> blocks_to_spawn;
			chunk->addDownBlocks(block->size, block->position_double->y - 2 * block->size, planet);
			for (shared_ptr<DownBlock> dBlock : chunk->downBlocks) {
				add_block_to_list_if_not_in_world(blocks_to_spawn, dBlock);
				// cout << "chunk dBlock " << dBlock->stringTag() << endl;
			}

			// ** for each neighbor gen downblocks until we're underneath
			vector<shared_ptr<Chunk>> neighbors_in_world = planet->rootChunk->getNESWNeighborsInitialized(chunk->getType(), chunk->position);
			for (shared_ptr<Chunk> neighbor : neighbors_in_world) {
				neighbor->addDownBlocks(block->size, block->position_double->y - 2 * block->size, planet);
				for (shared_ptr<DownBlock> dBlock : neighbor->downBlocks) {
					add_block_to_list_if_not_in_world(blocks_to_spawn, dBlock);
					// cout << "neighbor dBlock " << dBlock->stringTag() << endl;
				}
			}

			// ** spawn downblocks
			this->spawn_blocks(blocks_to_spawn);

			cout << endl;

		}


		// DELETE BLOCK
		if (block->position_double == nullptr) {

			block->position_double = std::make_shared<PositionDouble>(
				block->position->x,
				block->position->y,
				block->position->z
			);
		}
		this->delete_block(block->position_double);
		
	}

	bool add_block_to_list_if_not_in_world(unordered_set<shared_ptr<Block>>& blocks_to_spawn, shared_ptr<Block> block) {
		// int64_t id = block->getId();
		if (this->blocks_in_world.find(block->stringTag()) == this->blocks_in_world.end()) {
			blocks_to_spawn.insert(block);
			this->blocks_in_world[block->stringTag()] = block;
			return true;
		}
		return false;
	}

	bool is_terrain_block(shared_ptr<Block> block) {

		if (block->position_double != nullptr) {
			string block_xyz = Block::stringTag_double(0, block->position_double->x, block->position_double->y, block->position_double->z);
			string chunk_16_x_z = Chunk::chunk16_group_id(block->position_double->x, block->position_double->z);
			for(string block_x_y_z : this->chunk_16_x_z_to_added_block_x_y_z[chunk_16_x_z]) {
				if (block_xyz == block_x_y_z) {
					return false;
				}
			}

		}

		return true;
	}

	/**
	 * Returns the lowest block y with a top exposed to air. This could be underground in a cave or if an entity 
	 * deletes a world block
	 */
	int64_t get_surface_height(shared_ptr<Position> pos, int64_t terrain_height) {
		

		// check if block has been deleted
		bool block_was_deleted = false;
		int64_t y_level = terrain_height;
		string block_x_y_z = Block::stringTag_double(0, pos->x, terrain_height, pos->z);
		while(x_y_z_deleted_blocks.find(block_x_y_z) != x_y_z_deleted_blocks.end()) {
			block_was_deleted = true;
			--y_level;
			block_x_y_z = Block::stringTag_double(0, pos->x, y_level, pos->z);
		}

		// if not check if any blocks have been placed on top
		if (!block_was_deleted) {
			string chunk_16_x_z = Chunk::chunk16_group_id(pos->x, pos->z);
			unordered_set<string> added_blocks = chunk_16_x_z_to_added_block_x_y_z[chunk_16_x_z];

			while (added_blocks.find(block_x_y_z) != added_blocks.end()) {
				++y_level;
				block_x_y_z = Block::stringTag_double(0, pos->x, y_level, pos->z);
				
			}
		}

		return y_level;
	}



	// FLUID SIM

	/**
	 * Returns the block at the specified position. 
	 * 
	 * Sometimes there is multiple blocks at a position, so this function 
	 * does the following in order:
	 * - check for a block at the exact position
	 * - check for any block that overlaps the position
	 * 
	 * If mutliple blocks come back for any of these queries then the first block is returned.
	 * This will be random just based off which mesh we get from the map first and which instance 
	 * in that mesh is the first to match.
	 * 
	 */
	shared_ptr<Block> get_block_at_position(shared_ptr<PositionDouble> pos) {

		// look for block at exact position
		MultiMeshInstance3D* mesh = nullptr;
		int index = -1;
		bool player_placed;
		this->get_mesh_and_index(mesh, index, player_placed, pos);


		if (mesh != nullptr) {

			Ref<MultiMesh> mm = mesh->get_multimesh();
			shared_ptr<Block> block = std::make_shared<Block>();
			StaticBody3D* body = Object::cast_to<StaticBody3D>(mesh->get_child(0));
			CollisionShape3D* hit_collider = Object::cast_to<CollisionShape3D>(body->get_child(0));
			block->size = hit_collider->get_meta("size");
			Vector3 instance_location = mm->get_instance_transform(index).origin;
			block->position_double = CoordinateConversion::convert_fVector_to_position_double(
				instance_location, 
				this->terrainZeroZero, 
				this->planet->worldSize, 
				block->size
			);
			block->position = Position::from_position_double(block->position_double);
			block->material = mesh->get_material_override().is_valid()? mesh->get_material_override()->get_name().utf8().get_data() : "";

			return block;
		}
		// if not found look for overlapping blocks
		else {

			vector<shared_ptr<Block>> overlap_blocks;
			this->get_blocks_in_range(
				overlap_blocks,
				this->terrainZeroZero,
				this->planet->worldSize,
				pos->x,
				pos->y,
				pos->z,
				pos->x,
				pos->y,
				pos->z
			);

			return overlap_blocks.size() > 0? overlap_blocks[0] : nullptr;
		}
		return nullptr;
	}

	/**
	 * Returns the block at the specified position. 
	 * 
	 * Sometimes there is multiple blocks at a position, so this function 
	 * does the following in order:
	 * - check for a block at the exact position
	 * - check for any block that overlaps the position
	 * 
	 * If mutliple blocks come back for any of these queries then the first block is returned.
	 * This will be random just based off which mesh we get from the map first and which instance 
	 * in that mesh is the first to match.
	 * 
	 */
	shared_ptr<Block> get_block_at_position(shared_ptr<Position> pos) {
		return this->get_block_at_position(pos->to_position_double());
	}


	// SPACE

	void spawn_blocks(
		string group_id,
		const vector<shared_ptr<Block>>& blocks,
		Vector3 planet_center,
		SpaceGenLevel level
	) {


		this->blocks_spawned += blocks.size();

		// determine which materials and sizes we need
		unordered_map<string, vector<shared_ptr<Block>>> material_and_size_to_blocks;
		for (shared_ptr<Block> block : blocks) {
			string material = block->material;
			int size = block->getSize();
			string material_and_size = material + std::to_string(size);
			material_and_size_to_blocks[material_and_size].push_back(block);
		}

		// for each material and size, make a new UInstancedStaticMeshComponent and batch spawn the blocks
		for (auto& [material_size_string, blocks_this_mat] : material_and_size_to_blocks) {
			string material = blocks_this_mat[0]->material;
			double size = blocks_this_mat[0]->getSize();

			// get material
			Ref<ShaderMaterial> godot_material = MeshMaker::get_material(material);

			// get positions
			vector<Transform3D> trans;
			for (shared_ptr<Block> block : blocks_this_mat) {

				shared_ptr<Chunk> chunk = std::static_pointer_cast<Chunk>(block);
				for (int i = 0; i < chunk->space_positions.size(); i++) {
					if (chunk->space_group_ids[i] == group_id) {
						Vector3 space_position = chunk->space_positions[i];
						// get position
						Transform3D tran = CoordinateConversion::convert_position_to_space_transform(
							chunk,
							space_position,
							planet_center,
							this->planet,
							level
						);
						trans.push_back(tran);
					}
				}
			}
			
			// make collider container
			StaticBody3D* collider_container = memnew(StaticBody3D);
			
			// spawn instances
			Ref<Mesh> mesh;
			if (level == SpaceGenLevel::MILLION_TO_BILLION) {
				mesh = MeshMaker::meshes_MILLION_TO_BILLION[size];
			}
			else if (level == SpaceGenLevel::THOUSAND_TO_MILLION) {
				mesh = MeshMaker::meshes_THOUSAND_TO_MILLION[size];
			}
			MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
				this->node,
				godot_material,
				mesh,
				trans,
				collider_container,
				CoordinateConversion::game_scale(level, size)
			);

			// save it in the map for deleting
			this->group_id_to_mesh_instances[group_id].push_back(mm_instance);

		}


	}


	// OTHER
	
	int getCount()
	{
		int count = 0;
		// for (const auto& pair : MeshMaker::materialsAndSizeToStaticMesh) {
		// 	Ref<MultiMesh> staticMeshComponent = pair.second;

		// 	count += staticMeshComponent->GetInstanceCount();
		// }

		return count;
	}


	bool is_lower_level_spawned_in(string group_id, double current_level_size) {
        lock_guard<recursive_mutex> lock(this->group_id_to_mesh_instances.mute);
		if (this->group_id_to_mesh_instances.contains(group_id)) {
			concurrent_vector<MultiMeshInstance3D*>& meshes = this->group_id_to_mesh_instances[group_id];

			MultiMeshInstance3D* mesh = meshes[0];
			double size = mesh->get_meta("size");

			return current_level_size > size;
		}

		return false;
	}
	


private:


	void get_mesh_and_index(MultiMeshInstance3D*& mesh_result, int& index_result, bool& player_placed, shared_ptr<PositionDouble> position) {

		string chunk_16_x_z = Chunk::chunk16_group_id(position->x, position->z);
		string player_block_x_y_z = Block::stringTag_double(0, position->x, position->y, position->z);

		// player placed
		if (this->group_id_to_mesh_instances.contains(player_block_x_y_z)) {
			mesh_result = this->group_id_to_mesh_instances[player_block_x_y_z][0];
			index_result = 0;
			player_placed = true;
			return;
		}
		// if world block
		else {
			shared_ptr<Position> int_position = std::make_shared<Position>(
				round(position->x), 
				round(position->y), 
				round(position->z)
			);

			// go through each instance in each mesh in group until we find the block we're deleting
			if (this->group_id_to_mesh_instances.contains(chunk_16_x_z)) {
				concurrent_vector<MultiMeshInstance3D*>& group_meshes = this->group_id_to_mesh_instances[chunk_16_x_z];
				for (MultiMeshInstance3D* mesh : group_meshes) {
					Ref<MultiMesh> mm = mesh->get_multimesh();
					int num_instances = mm->get_instance_count();
					for (int index = 0; index < num_instances; ++index)
					{
						Transform3D instance_transform = mm->get_instance_transform(index);
						Vector3 instance_location = instance_transform.origin;
						
						shared_ptr<Position> instance_position = CoordinateConversion::convert_fVector_to_position(
							instance_location,
							this->terrainZeroZero,
							this->planet->worldSize,
							0
						);

						if (
							instance_position->x == int_position->x && 
							instance_position->y == int_position->y && 
							instance_position->z == int_position->z
						) {
							mesh_result = mesh;
							index_result = index;
							player_placed = false;
							return;
						}
					}
				}
			}
		}
	
	}
	

	// void remove_instance(MultiMeshInstance3D* mesh, int index) {
	// 	if (!mesh) return;
		
	// 	Ref<MultiMesh> mm = mesh->get_multimesh();
	// 	if (!mm.is_valid()) return;
		
	// 	int count = mm->get_instance_count();
	// 	if (index < 0 || index >= count) return;
		
	// 	// Get the raw buffer
	// 	PackedFloat32Array buffer = mm->get_buffer();
		
	// 	// Each instance is 12 floats for TRANSFORM_3D (3x4 matrix)
	// 	// [basis.x.x, basis.x.y, basis.x.z, origin.x,
	// 	//  basis.y.x, basis.y.y, basis.y.z, origin.y,
	// 	//  basis.z.x, basis.z.y, basis.z.z, origin.z]
	// 	int floats_per_instance = 12;
		
	// 	// Calculate byte positions
	// 	int index_start = index * floats_per_instance;
	// 	int last_start = (count - 1) * floats_per_instance;
		
	// 	// If not removing the last instance, copy last instance over the one we're removing
	// 	if (index < count - 1) {
	// 		for (int i = 0; i < floats_per_instance; i++) {
	// 			buffer[index_start + i] = buffer[last_start + i];
	// 		}
	// 	}
		
	// 	// Resize the buffer to remove the last instance
	// 	buffer.resize((count - 1) * floats_per_instance);
		
	// 	// Set the buffer back and update count
	// 	mm->set_buffer(buffer);
	// 	mm->set_instance_count(count - 1);
	// }

};
