
#pragma once

#include <memory>

#include "util/CoordinateConversion.hpp"
#include "WorldBlockPool.hpp"
#include "util/concurrent_map.hpp"
#include <atomic>
#include "util/DebugShapes.hpp"

using namespace std;



/**
 * A wrapper around WorldBlockPool that can be called from a worker thread so that the actual
 * block spawning occurs on the game thread. (if you don't do this you get errors)
 */
#ifdef GODOT_ENABLED
class DeferredWorldBlockPool : public Node {
#else
class DeferredWorldBlockPool {
#endif


public:
	shared_ptr<WorldBlockPool> cubePool;
	concurrent_map< int,std::any > worker_thread_data;
	int tag_i = 0;


	void spawn_blocks(string group_id, shared_ptr<vector<shared_ptr<Block>>> group_blocks) {
		this->worker_thread_data.push(this->tag_i, group_blocks);
#ifdef GODOT_ENABLED
		callable_mp(this, static_cast<void(DeferredWorldBlockPool::*)(String, int)>(&DeferredWorldBlockPool::spawn_blocks_deferred))
			.call_deferred(GodotUtil::g_str(group_id), this->tag_i); // spawn on main game thread
#else
		this->spawn_blocks_cubepool(group_id, this->tag_i);
#endif
		++this->tag_i;
	}

	void spawn_space_blocks(string group_id, shared_ptr<vector<shared_ptr<Block>>> group_blocks, Vector3 offset, double game_scale) {
		this->worker_thread_data.push(this->tag_i, group_blocks);
#ifdef GODOT_ENABLED
		callable_mp(this, static_cast<void(DeferredWorldBlockPool::*)(String, int, Vector3, double)>(&DeferredWorldBlockPool::spawn_space_blocks_deferred))
			.call_deferred(GodotUtil::g_str(group_id), this->tag_i, offset, game_scale); // spawn on main game thread
#else
		this->spawn_space_blocks_cubepool(group_id, this->tag_i, offset, game_scale);
#endif
		++this->tag_i;
	}


	void despawn_blocks(string group_id) {
#ifdef GODOT_ENABLED
		callable_mp(this, &DeferredWorldBlockPool::despawn_blocks_deferred)
			.call_deferred(GodotUtil::g_str(group_id)); // on main game thread
#else
		this->despawn_blocks_cubepool(group_id);
#endif
	}




	// DEFERRED FUNCTIONS

	void draw_line_deferred(Vector3 start, Vector3 end) {
		DebugLine::draw_line(start, end);
	}


	void spawn_blocks_deferred(String group_id, int tag_for_blocks_to_spawn) {
		this->spawn_blocks_cubepool(GodotUtil::c_str(group_id), tag_for_blocks_to_spawn);
	}

	void spawn_space_blocks_deferred(String group_id, int tag_for_blocks_to_spawn, Vector3 offset, double game_scale) {
		this->spawn_space_blocks_cubepool(GodotUtil::c_str(group_id), tag_for_blocks_to_spawn, offset, game_scale);
	}

	void despawn_blocks_deferred(String group_id) {
		this->despawn_blocks_cubepool(GodotUtil::c_str(group_id));
	}


private:
	/*
	
	actual calls to cubepool 
	- these are seperated out because above we need to call from both godot callable method that don't accept c++ strings
	- as well as from local tests without godot that can't make godot String objects
	- but we want to make sure both these calls go through the same code.
	- So all the above methods should just be stubs
	*/


	void spawn_blocks_cubepool(string group_id, int tag_for_blocks_to_spawn) {
		shared_ptr<vector<shared_ptr<Block>>> blocks = std::any_cast<shared_ptr<vector<shared_ptr<Block>>>>(
			worker_thread_data.pop(tag_for_blocks_to_spawn)
		);
		this->cubePool->spawn_blocks(group_id, *blocks.get());
	}

	void spawn_space_blocks_cubepool(string group_id, int tag_for_blocks_to_spawn, Vector3 offset, double game_scale) {
		shared_ptr<vector<shared_ptr<Block>>> blocks = std::any_cast<shared_ptr<vector<shared_ptr<Block>>>>(
			worker_thread_data.pop(tag_for_blocks_to_spawn)
		);
		this->cubePool->spawn_space_blocks(group_id, *blocks.get(), offset, game_scale);
	}


	void despawn_blocks_cubepool(string group_id) {
		this->cubePool->despawn_blocks(group_id);
	}
};

