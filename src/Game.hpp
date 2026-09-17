#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H


#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include "util/StellarCoordinate.hpp"


#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/memory.hpp>  // for memnew
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/display_server.hpp>


#include "Player.hpp"
#include "world/World.hpp"
#include "space/SpaceGen.hpp"
#include "world/GodotHdr.hpp"
#include "space/SkyGen.hpp"
#include "server/Interface.hpp"

using namespace std;
using namespace godot;

struct Game : public Node {
	GDCLASS(Game, Node)

public: 
	float time_passed;

	
	Player* player;
	World* world;

	// ─── SPACE TRANSITION ───────────────────────────────
	shared_ptr<StellarCoordinate> base_stellar;
	shared_ptr<StellarCoordinate> current_stellar;
	Vector3 last_regen_view;
	Vector3 last_player_godot_pos = Vector3(0, 0, 0);
	double last_game_scale = 1.0;              // LY -> godot-units from the last generate_space

	// Currently-displayed planet's godot geometry, used to preserve its on-screen
	// angular size across a regen swap (prevents the planet "popping" bigger).
	Vector3 prev_planet_center_godot = Vector3(0, 0, 0);
	double prev_planet_radius_godot = 0.0;
	double regen_accumulator = 0.0;
	double space_regen_interval = 0.5;         // regen at most every 0.5s
	double SPACE_REGEN_MOVED_LY = 0.05;        // don't regen unless moved this many LY
	double SPACE_REGEN_MOVED_GODOT = 500.0;      // refine the cadence as the frame scale grows
	double space_skybox_interval = 5.0;        // only refresh the skybox every this many seconds
	double skybox_accumulator = 5.0;           // primed so the first regen applies its skybox immediately
	bool on_planet = false;
	shared_ptr<Planet> pending_planet;
	Vector3 landed_dir = Vector3(0, 0, 1);   // unit vector planet -> player, for leaving

	// space-block pool: Game-owned so it exists in pure space (world->cubePool is only created on planet entry)
	DeferredWorldBlockPool* space_deferred = nullptr;
	shared_ptr<WorldBlockPool> space_cube_pool;
	MeshInstance3D* planet_background_sphere = nullptr;
	MeshInstance3D* planet_atmosphere_sphere = nullptr;
	MeshInstance3D* terrain_min_height_plane = nullptr;
	StaticBody3D* terrain_min_height_body = nullptr;

	// paced space-block swap: blocks are split into this many sub-groups and
	// despawned/spawned one sub-group per frame so a regen doesn't spike one frame
	const int SPACE_SWAP_CHUNKS = 8;
	std::atomic<bool> space_swap_active{false};   // main thread is still loading a swap
	vector<string> space_swap_old_groups;          // live sub-group ids, despawned one per frame
	vector<shared_ptr<vector<shared_ptr<Block>>>> space_swap_new_chunks;
	int space_swap_cursor = 0;
	double space_swap_game_scale = 1.0;
	Vector3 space_swap_offset = Vector3();

	// space regen worker thread: generate_space + block spawn/despawn off the main game thread
	std::thread space_worker;
	std::atomic<bool> space_running{false};
	std::mutex space_request_mutex;
	bool space_regen_requested = false;
	shared_ptr<StellarCoordinate> space_gen_target;
	Vector3 space_gen_offset = Vector3();
	std::mutex space_result_mutex;
	shared_ptr<SpaceGen::SpaceGenResult> pending_space_result;

	Game() {
		// Initialize any variables here.
		time_passed = 0.0;
	}

	~Game() {
		// Add your cleanup here.

		this->space_running = false;
		if (this->space_worker.joinable())
			this->space_worker.join();

		delete this->world;
		this->world = nullptr;

		// 0,8,-11
		// -36, -180, 0
	}

	void _exit_tree() override {
		this->space_running = false;
		if (this->space_worker.joinable())
			this->space_worker.join();
	}

	static void _bind_methods() {
	}

	void _ready() {

		// init models
		MeshMaker::init_models();

		// fullscreen
		DisplayServer::get_singleton()->window_set_mode(DisplayServer::WindowMode::WINDOW_MODE_FULLSCREEN);

		// planet with no life, 85,445.659 km radius
		shared_ptr<StellarCoordinate> planet_location_not_life = std::make_shared<StellarCoordinate>();
		planet_location_not_life->quadrant_x = 10966621603166629;
		planet_location_not_life->quadrant_y = -89418746164600678;
		planet_location_not_life->quadrant_z = 62932456601427350;
		planet_location_not_life->light_year_x = 5725;
		planet_location_not_life->light_year_y = 3180;
		planet_location_not_life->light_year_z = 24;
		planet_location_not_life->km_x = 6354938493899;
		planet_location_not_life->km_y = 6928655042441;
		planet_location_not_life->km_z = 2856407988948;

		// planet with life, 8,010,530.573 m radius
		shared_ptr<StellarCoordinate> planet_location_with_life = std::make_shared<StellarCoordinate>();
		planet_location_not_life->quadrant_x = 10966621603166630;
		planet_location_not_life->quadrant_y = -89418746164600676;
		planet_location_not_life->quadrant_z = 62932456601427347;
		planet_location_not_life->light_year_x = 8977;
		planet_location_not_life->light_year_y = 9957;
		planet_location_not_life->light_year_z = 9535;
		planet_location_not_life->km_x = 2238696550845;
		planet_location_not_life->km_y = 7415650432515;
		planet_location_not_life->km_z = 2104350772427;

		shared_ptr<StellarCoordinate> galaxy_location = std::make_shared<StellarCoordinate>();
		galaxy_location->quadrant_x = 10966621603166628;
		galaxy_location->quadrant_y = -89418746164600680;
		galaxy_location->quadrant_z = 62932456601427352;
		galaxy_location->light_year_x = 0;
		galaxy_location->light_year_y = 0;
		galaxy_location->light_year_z = 0;
		galaxy_location->km_x = 0;
		galaxy_location->km_y = 0;
		galaxy_location->km_z = 0;

		shared_ptr<StellarCoordinate> player_location = std::make_shared<StellarCoordinate>();
		player_location = planet_location_not_life;
		// player_location = galaxy_location;

		// 8010 km
		// player_location->km_z += 85'000;
		player_location->km_z += 8020;

		this->world = new World;
		this->world->node = this;

		// space-block pool: Game-owned so it works in pure space (before any planet entry)
		this->space_cube_pool = std::make_shared<WorldBlockPool>(
			this,
			std::make_shared<Position>(0, 0, 0),
			std::make_shared<Planet>(0, std::make_shared<StellarCoordinate>()), // unused by the space path
			[](int64_t, int64_t) {}
		);
		this->space_deferred = memnew(DeferredWorldBlockPool);
		add_child(this->space_deferred);
		this->space_deferred->cubePool = this->space_cube_pool;

		// space regen worker thread (generate + spawn/despawn off the main thread)
		this->space_running = true;
		this->space_worker = std::thread(&Game::space_worker_loop, this);
		start_game(player_location);

		// random_test(player_location);


    }

	void start_game(shared_ptr<StellarCoordinate> player_location) {

		this->base_stellar = player_location;
		this->current_stellar = player_location;
		this->last_player_godot_pos = Vector3(0, 0, 0);

		// make the player first so the transition code can reposition it
		spawn_player(player_location, LocationType::SPACE);

		// first space generation (skybox + blocks + landing detection + terrain handoff)
		request_regen();
	}

	// ─── SPACE TRANSITION LOOP ────────────────────────────

	// Move the player's stellar coordinate by their godot-scene movement,
	// converting godot units -> LY through the space game_scale.
	void update_stellar_from_movement() {
		if (!this->player || !this->current_stellar) return;

		Vector3 pos = this->player->hit_box->get_global_position();
		Vector3 delta = pos - this->last_player_godot_pos;
		if (delta.length() < 0.0001) return;

		double scale = this->last_game_scale > 0 ? this->last_game_scale : 1.0;
		this->current_stellar = SpaceGen::offset_coordinate(
			this->current_stellar,
			delta.x / scale,
			delta.y / scale,
			delta.z / scale
		);
		this->last_player_godot_pos = pos;
	}

	// Queue a regen for the worker thread (main thread only: snapshots the stellar
	// coord + player offset the worker needs, so there's no shared mutable state).
	void request_regen() {
		if (!this->current_stellar || !this->space_deferred) return;
		lock_guard<mutex> lock(this->space_request_mutex);
		this->space_gen_target = std::make_shared<StellarCoordinate>(*this->current_stellar);
		this->space_gen_offset = this->world ? this->world->player_location : Vector3();
		this->space_regen_requested = true;
		this->last_regen_view = this->player->hit_box->get_position();
	}

	// Worker thread: runs generate_space off the main thread; the main thread does the
	// paced block swap + skybox/landing node work.
	void space_worker_loop() {
		while (this->space_running) {
			shared_ptr<StellarCoordinate> target;
			bool requested = false;
			{
				lock_guard<mutex> lock(this->space_request_mutex);
				if (this->space_regen_requested) {
					requested = true;
					target = this->space_gen_target;
					this->space_regen_requested = false;
				}
			}
			if (!requested || !target) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			// don't produce a new swap until the main thread has finished loading the previous one
			while (this->space_swap_active.load() && this->space_running) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if (!this->space_running) break;

			shared_ptr<SpaceGen::SpaceGenResult> result = SpaceGen::generate_space(target);

			// hand the result to the main thread for the node work (paced block swap, skybox, re-anchor, landing)
			{
				lock_guard<mutex> lock(this->space_result_mutex);
				this->pending_space_result = result;
			}
		}
	}

	// Apply a finished regen on the main thread: skybox + player re-anchor + planet handoff.
 	void apply_space_result() {
		shared_ptr<SpaceGen::SpaceGenResult> result;
		{
			lock_guard<mutex> lock(this->space_result_mutex);
			if (!this->pending_space_result) return;
			result = this->pending_space_result;
			this->pending_space_result.reset();
		}

		this->last_game_scale = result->game_scale;

		GodotHdr::set_hdr_skybox(this, result->sky_box, SkyGen::IMAGE_WIDTH, SkyGen::IMAGE_HEIGHT);
		this->skybox_accumulator -= this->space_skybox_interval;
		result->sky_box = nullptr;

		this->pending_planet = result->planet;
		if (result->planet) {
			clear_planet_background_sphere();
			// over a surface: hand off to terrain (space blocks are useless on the ground)
			// remove any live space groups before dropping the swap bookkeeping
			if (this->space_cube_pool) {
				for (const string& group_id : this->space_swap_old_groups)
					this->space_cube_pool->despawn_blocks(group_id);
			}
			this->space_swap_old_groups.clear();
			this->space_swap_new_chunks.clear();
			this->space_swap_active = false;
			enter_planet(result->planet, result->planet_background_sphere);
		} else {
			// preserve the planet's on-screen angular size across the swap by placing
			// the player at a godot offset instead of always snapping to the origin
			Vector3 reanchor = Vector3(0, 0, 0);
			if (result->planet_background_sphere && this->prev_planet_radius_godot > 0.0) {
				auto& s = result->planet_background_sphere;

				// new planet's godot center and radius (where its blocks/sphere will live)
				Vector3 C_new = Vector3(s->center_x_ly, s->center_y_ly, s->center_z_ly) *
					result->game_scale + this->space_gen_offset;
				double R_new = s->radius_godot;

				// angular radius from the current player->planet distance and the OLD radius
				Vector3 player_pos = this->player->hit_box->get_global_position();
				double dist_cur = (this->prev_planet_center_godot - player_pos).length();
				double ang = dist_cur > 0.0 ? this->prev_planet_radius_godot / dist_cur : 3.14;

				// distance that gives that same angular radius with the NEW radius
				double target_d = R_new / ang;
				double c_len = C_new.length();
				reanchor = C_new * (1.0 - target_d / c_len);
			}

			// re-anchor the player; keep world->player_location (space_gen_offset) at
			// the origin so the next generation isn't double-shifted by the offset
			this->last_player_godot_pos = reanchor;
			if (this->player) this->player->hit_box->set_global_position(reanchor);
			if (this->world) this->world->player_location = Vector3(0, 0, 0);
			set_planet_background_sphere(
				result->planet_background_sphere,
				result->game_scale,
				this->space_gen_offset
			);

			// remember the new planet's godot geometry for the next swap
			if (result->planet_background_sphere) {
				auto& s = result->planet_background_sphere;
				this->prev_planet_center_godot = Vector3(s->center_x_ly, s->center_y_ly, s->center_z_ly) *
					result->game_scale + this->space_gen_offset;
				this->prev_planet_radius_godot = s->radius_godot;
			} else {
				this->prev_planet_center_godot = Vector3(0, 0, 0);
				this->prev_planet_radius_godot = 0.0;
			}

			// split the new blocks into sub-groups and swap them in a paced fashion
			start_paced_swap(result->blocks, result->game_scale);
		}
	}

	void clear_planet_background_sphere() {
		if (this->planet_background_sphere) {
			this->planet_background_sphere->queue_free();
			this->planet_background_sphere = nullptr;
		}
		if (this->planet_atmosphere_sphere) {
			this->planet_atmosphere_sphere->queue_free();
			this->planet_atmosphere_sphere = nullptr;
		}
	}

	void clear_terrain_min_height_plane() {
		if (this->terrain_min_height_plane) {
			this->terrain_min_height_plane->queue_free();
			this->terrain_min_height_plane = nullptr;
		}
		if (this->terrain_min_height_body) {
			this->terrain_min_height_body->queue_free();
			this->terrain_min_height_body = nullptr;
		}
	}

	void set_terrain_min_height_plane(const shared_ptr<SpaceGen::PlanetBackgroundSphere>& color_data) {
		clear_terrain_min_height_plane();
		if (!this->world || !this->world->cubePool || !this->world->cubePool->terrainZeroZero)
			return;

		constexpr double plane_size = 30000.0;
		shared_ptr<PositionDouble> min_height_position = std::make_shared<PositionDouble>(
			this->world->cubePool->terrainZeroZero->x,
			(double)Planet::minHeight,
			this->world->cubePool->terrainZeroZero->z
		);
		Vector3 plane_position = CoordinateConversion::convert(
			min_height_position,
			this->world->cubePool->terrainZeroZero,
			1.0
		);

		Color color(0.5, 0.5, 0.5);
		if (color_data)
			color = Color(color_data->r, color_data->g, color_data->b);

		Ref<PlaneMesh> plane_mesh;
		plane_mesh.instantiate();
		plane_mesh->set_size(Vector2(plane_size, plane_size));
		Ref<StandardMaterial3D> material;
		material.instantiate();
		material->set_albedo(color);
		plane_mesh->surface_set_material(0, material);

		this->terrain_min_height_plane = memnew(MeshInstance3D);
		this->terrain_min_height_plane->set_mesh(plane_mesh);
		this->terrain_min_height_plane->set_global_position(plane_position);
		add_child(this->terrain_min_height_plane);

		Ref<BoxShape3D> floor_shape;
		floor_shape.instantiate();
		floor_shape->set_size(Vector3(plane_size, 0.1, plane_size));
		CollisionShape3D* collision_shape = memnew(CollisionShape3D);
		collision_shape->set_shape(floor_shape);
		this->terrain_min_height_body = memnew(StaticBody3D);
		this->terrain_min_height_body->set_global_position(plane_position);
		this->terrain_min_height_body->add_child(collision_shape);
		add_child(this->terrain_min_height_body);
	}

	void set_planet_background_sphere(
		const shared_ptr<SpaceGen::PlanetBackgroundSphere>& data,
		double game_scale,
		Vector3 frame_offset
	) {
		clear_planet_background_sphere();
		if (!data || data->radius_godot <= 0.0) return;

		Ref<SphereMesh> sphere_mesh;
		sphere_mesh.instantiate();
		// Keep the coarse body behind the centers and faces of the voxel shell.
		double radius = data->radius_godot - data->block_size_godot * 0.5 - 0.01;
		if (radius <= 0.0) return;
		sphere_mesh->set_radius(radius);
		sphere_mesh->set_height(radius * 2.0);

		Ref<StandardMaterial3D> material;
		material.instantiate();
		material->set_albedo(Color(data->r, data->g, data->b));

		this->planet_background_sphere = memnew(MeshInstance3D);
		this->planet_background_sphere->set_mesh(sphere_mesh);
		this->planet_background_sphere->set_material_override(material);
		add_child(this->planet_background_sphere);
		this->planet_background_sphere->set_global_position(Vector3(
			data->center_x_ly * game_scale + frame_offset.x,
			data->center_y_ly * game_scale + frame_offset.y,
			data->center_z_ly * game_scale + frame_offset.z
		));

		if (data->atmosphere_strength <= 0.0) return;
		Ref<SphereMesh> atmosphere_mesh;
		atmosphere_mesh.instantiate();
		atmosphere_mesh->set_radius(radius * 1.015);
		atmosphere_mesh->set_height(radius * 2.03);
		this->planet_atmosphere_sphere = memnew(MeshInstance3D);
		this->planet_atmosphere_sphere->set_mesh(atmosphere_mesh);
		this->planet_atmosphere_sphere->set_material_override(
			MeshMaker::make_atmosphere_material(
				Color(data->atmosphere_r, data->atmosphere_g, data->atmosphere_b),
				(float)data->atmosphere_strength
			)
		);
		add_child(this->planet_atmosphere_sphere);
		this->planet_atmosphere_sphere->set_global_position(
			this->planet_background_sphere->get_global_position()
		);
	}

	// Split a freshly-generated set of space blocks into SPACE_SWAP_CHUNKS sub-groups
	// and begin swapping them in one sub-group per frame. Runs on the main thread.
	void start_paced_swap(vector<shared_ptr<Block>>& blocks, double game_scale) {
		this->space_swap_new_chunks.clear();
		this->space_swap_new_chunks.resize(this->SPACE_SWAP_CHUNKS);
		for (int i = 0; i < this->SPACE_SWAP_CHUNKS; ++i) {
			auto chunk = std::make_shared<vector<shared_ptr<Block>>>();
			int start = (int)((size_t)i * blocks.size() / this->SPACE_SWAP_CHUNKS);
			int end = (int)((size_t)(i + 1) * blocks.size() / this->SPACE_SWAP_CHUNKS);
			for (int b = start; b < end; ++b) chunk->push_back(blocks[b]);
			this->space_swap_new_chunks[i] = chunk;
		}

		this->space_swap_game_scale = game_scale;
		this->space_swap_offset = this->space_gen_offset;
		this->space_swap_cursor = 0;
		this->space_swap_active = true;

		// do the first sub-group now so there's no fully-empty frame
		paced_space_swap_tick();
	}

	// One step of the paced swap: despawn one old sub-group + spawn one new sub-group.
	// Called every frame from _process (and once from start_paced_swap).
	void paced_space_swap_tick() {
		if (!this->space_swap_active) return;

		int i = this->space_swap_cursor;
		// free one old sub-group first, then reuse its id for the new sub-group
		if (i < (int)this->space_swap_old_groups.size()) {
			this->space_cube_pool->despawn_blocks(this->space_swap_old_groups[i]);
		}
		if (i < (int)this->space_swap_new_chunks.size() && !this->space_swap_new_chunks[i]->empty()) {
			string gid = Util::format("%s%d", "space_group_", i);
			this->space_cube_pool->spawn_space_blocks(gid, *this->space_swap_new_chunks[i], this->space_swap_offset, this->space_swap_game_scale);
		}
		++this->space_swap_cursor;

		int total = std::max((int)this->space_swap_old_groups.size(), (int)this->space_swap_new_chunks.size());
		if (this->space_swap_cursor >= total) {
			// the just-spawned sub-groups are now the live ones for the next regen to despawn
			this->space_swap_old_groups.clear();
			for (int c = 0; c < (int)this->space_swap_new_chunks.size(); ++c) {
				if (!this->space_swap_new_chunks[c]->empty())
					this->space_swap_old_groups.push_back(Util::format("%s%d", "space_group_", c));
			}
			this->space_swap_new_chunks.clear();
			this->space_swap_active = false;
		}
	}

	// Space -> planet surface: init world/rootChunk, drop the player in, gravity on.
	void enter_planet(
		shared_ptr<Planet> planet,
		const shared_ptr<SpaceGen::PlanetBackgroundSphere>& color_data = nullptr
	) {
		if (!this->world || !planet) return;
		this->on_planet = true;

		// Reuse the terrain tree generated while rendering this planet from space.
		// Only build it here when the space path did not prepare one.
		if (planet->rootChunk) {
			this->world->rootChunk = static_pointer_cast<RootChunk>(planet->rootChunk);
			this->world->rootChunk->rootChunk = this->world->rootChunk;
		} else if (!this->world->rootChunk) {
			int64_t root_square = (int64_t)ceil(planet->worldSize / (double)Chunk::CHUNK_SIZES[Chunk::topTypeFor(planet->worldSize)]);
			if (root_square < 1) root_square = 1;
			this->world->rootChunk = std::make_shared<RootChunk>((int)root_square, 0, planet);
			this->world->rootChunk->rootChunk = this->world->rootChunk;
			planet->rootChunk = this->world->rootChunk;
			this->world->rootChunk->init(planet);
		}
		this->world->planet = planet;
		if (this->world->cubePool == nullptr) {
			this->world->init(planet);
		}

		this->world->genType = GenType::TERRAIN;
		this->world->TERRAIN_GEN_ON = true;


		// remember which way "up" is from the planet so we can climb back out
		{
			double ox, oy, oz;
			double pdist = SpaceGen::viewer_dist(this->current_stellar, planet->location, ox, oy, oz);
			double inv = 1.0 / max(pdist, 1e-12);
			this->landed_dir = Vector3(-ox * inv, -oy * inv, -oz * inv);
			// Keep the player's space view, but make planet-center direction point down.
			this->player->orient_camera_for_surface(-this->landed_dir);
		}

		// drop the player onto the surface at the direction they approached from
		shared_ptr<Position> surface_world = CoordinateConversion::sphere_location_to_world_position(this->landed_dir, planet);
		this->world->cubePool->terrainZeroZero = surface_world;
		auto* root = static_cast<RootChunk*>(planet->rootChunk.get());
		int idx = root->positionToIndex(surface_world, planet);
		double surface_height = 0.0;
		if (idx >= 0 && (size_t)idx < root->subChunks.size())
			surface_height = root->subChunks[idx]->position->y;
		double drop_height = planet->maxHeight - 3.0;

		shared_ptr<PositionDouble> surface_pos = std::make_shared<PositionDouble>(surface_world->x, drop_height, surface_world->z);
		Vector3 godot_pos = CoordinateConversion::convert(surface_pos, this->world->cubePool->terrainZeroZero, 1.0);
		this->player->init(this->world, planet, this->current_stellar, LocationType::TERRAIN);
		this->player->hit_box->set_global_position(godot_pos);
		this->last_player_godot_pos = godot_pos;

		// gravity + normal movement
		this->player->set_location_mode(LocationType::TERRAIN);
		set_terrain_min_height_plane(color_data);

		// start the worker thread once
		if (!this->world->running) {
			this->world->start();
		}
	}

	// Planet surface -> space: stop terrain gen, pace-despawn the surface, force a regen.
	void leave_planet() {
		if (!this->world || !this->world->planet) return;
		this->on_planet = false;
		clear_terrain_min_height_plane();

		this->world->TERRAIN_GEN_ON = false;
		this->world->genType = GenType::SPACE;
		this->world->queue_all_terrain_for_despawn();

		// no-gravity flight, normal space speed
		this->player->set_location_mode(LocationType::SPACE);

		// push the stellar coord up out of the atmosphere so the next regen
		// doesn't immediately re-land us (we're above max terrain height now)
		double height_m = 0.0;
		shared_ptr<Position> player_position = this->world->get_player_position();
		if (player_position)
			height_m = (double)player_position->y;
		double climb_m = max(Planet::maxHeight - height_m, 1.0);
		double climb_ly = climb_m * SpaceGen::LY_PER_M;
		this->current_stellar = SpaceGen::offset_coordinate(
			this->current_stellar,
			this->landed_dir.x * climb_ly,
			this->landed_dir.y * climb_ly,
			this->landed_dir.z * climb_ly
		);

		// force a fresh space regen (stellar coord didn't move while on the surface)
		this->last_regen_view = Vector3(0,0,0);
		this->regen_accumulator = this->space_regen_interval;
	}

	void space_transition_tick(double delta) {
		if (!this->player) return;

		// on the surface: watch for climbing out of the atmosphere
		if (this->on_planet) {
			double height_m = 0.0;
			if (this->world) {
				shared_ptr<Position> player_position = this->world->get_player_position();
				if (player_position)
					height_m = (double)player_position->y;
			}
			if (height_m > Planet::maxHeight) {
				leave_planet();
			}
			return;
		}

		// in space: periodic regen gated on movement + altitude
		this->regen_accumulator += delta;
		if (this->regen_accumulator < this->space_regen_interval) return;
		this->regen_accumulator = 0.0;

		update_stellar_from_movement();

		// determine if we've moved enough for a regen
		double ox, oy, oz;
		double moved_godot = (double) (this->last_regen_view - this->player->hit_box->get_position()).length();
		if (moved_godot < SPACE_REGEN_MOVED_GODOT) return;

		request_regen();
	}

	void _process(double delta) {
		
		if (!first_called) {
			first_called = true;
			first_frame();
		}

		this->skybox_accumulator += delta;

		// apply a finished space regen (skybox, re-anchor, landing) if one is ready
		apply_space_result();

		// step the paced space-block swap (one sub-group despawn/spawn per frame)
		paced_space_swap_tick();

		// space <-> planet transition loop
		space_transition_tick(delta);
	}


	

	void basic_test(LocationType location_type, shared_ptr<StellarCoordinate> world_location, shared_ptr<StellarCoordinate> player_location) {
		
		// INIT terrain gen
		Transform3D player_start_transform; 
		player_start_transform.origin = Vector3(0, 0, 0);

		//TODO recieve the quadrant seed here, add the position of the planet to get the seed
		int64_t seed = 56635234455L;

		shared_ptr<Planet> planet = std::make_shared<Planet>(seed, world_location);
		// XXX DELETE THIS this is just for testing chunk smoothness parameters easily
		string kv = KVSerializer::read_file("src/TEST/planet.kv");
		KVSerializer::deserialize(kv, *planet);
		stringstream ss;
		KVSerializer::serialize(ss, *planet);
		cout << ss.str() << endl;

		// DELETE THIS testing plants
		planet->hasLife = true;
		planet->precipitation = 50;
		
		int64_t root_square = 40;
		planet->worldSize = root_square * Chunk::CHUNK_SIZES[ChunkType::CHUNK_2097152];
		planet->radius = planet->worldSize / (2*M_PI);
		root_square = (int64_t)ceil(planet->worldSize / (double)Chunk::CHUNK_SIZES[Chunk::topTypeFor(planet->worldSize)]);
		if (root_square < 1) root_square = 1;
		this->world->rootChunk = std::make_shared<RootChunk>(root_square, 0, planet);
		this->world->rootChunk->rootChunk = this->world->rootChunk;
		planet->rootChunk = this->world->rootChunk;
		this->world->rootChunk->init(planet);
		this->world->init(planet);
		this->world->genType = location_type == LocationType::SPACE? GenType::SPACE : GenType::TERRAIN;
		if (location_type == LocationType::SPACE) {
			player_start_transform = this->world->init_for_space(player_location);
		}

		// SPAWN start surface (also testing group collider thing, will delete all of this someday)
		double godot_size = 40 * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE;
		Ref<Mesh> mesh = MeshMaker::make_cube(godot_size, "assets/textures/SAND.png", 1);
		// Ref<Mesh> mesh = MeshMaker::make_cube(10, "assets/textures/BDOG.png", 1);
		StaticBody3D* body = memnew(StaticBody3D);
		this->add_child(body);
		Array positions;
		Vector3 pos = Vector3(10, -30, 0);
		positions.append(pos);
		CollisionShape3D* shape = MeshMaker::make_group_collider(
			positions,
			pos,
			godot_size
		);
		body->add_child(shape);
		MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(mesh);
		body->add_child(mesh_instance);
		body->set_global_position(positions[0]);

		// MAKE player
		this->player = memnew(Player);
		this->player->init(this->world, this->world->planet, player_location, location_type);
		add_child(this->player);

		// set player tranforms
		this->player->hit_box->set_transform(player_start_transform);


		// start world gen
		this->world->start();

	}

	bool first_called = false;
	void first_frame() {

		if (world->cubePool != nullptr) {
			// spawn_test_block(
			// 	std::make_shared<PositionDouble>(0, 4, 0),
			// 	0.25
			// );
			// spawn_test_block(
			// 	std::make_shared<PositionDouble>(0, 4.25, 0),
			// 	0.25
			// );
			spawn_test_block(
				std::make_shared<PositionDouble>(0, 4, 0),
				1
			);
			spawn_test_block(
				std::make_shared<PositionDouble>(0, 2, 0),
				2
			);
		}
		
		DebugLine::draw_line(Vector3(-1, 0, 0), Vector3(1, 0, 0) , Color(1, 0, 0)); // x axis RED
		DebugLine::draw_line(Vector3(0, -1, 0), Vector3(0, 1, 0) , Color(0, 1, 0)); // y axis GREEN
		DebugLine::draw_line(Vector3(0, 0, -1), Vector3(0, 0, 1) , Color(0, 0, 1)); // z axis BLUE

		// x grid lines
		DebugLine::draw_line(Vector3(-0.5, 0, -0.1), Vector3(-0.5, 0, 0.1) , Color(1, 0, 0));
		DebugLine::draw_line(Vector3(0.5, 0, -0.1), Vector3(0.5, 0, 0.1) , Color(1, 0, 0));

		// y grid lines
		DebugLine::draw_line(Vector3(-0.1, -0.5, 0), Vector3(0.1, -0.5, 0) , Color(0, 1, 0));
		DebugLine::draw_line(Vector3(-0.1, 0.5, 0), Vector3(0.1, 0.5, 0) , Color(0, 1, 0));

		// z grid lines
		DebugLine::draw_line(Vector3(-0.1, 0, -0.5), Vector3(0.1, 0, -0.5) , Color(0, 0, 1));
		DebugLine::draw_line(Vector3(-0.1, 0, 0.5), Vector3(0.1, 0, 0.5) , Color(0, 0, 1));


		// shared_ptr<Block> block = std::make_shared<Block>();
		// block->material = BMaterial::BASALT;
		// block->size = (int)ChunkType::CHUNK_2097152;
		// vector<shared_ptr<Block>> bs;
		// bs.push_back(block);
		// this->world->cubePool->spawn_blocks(
		// 	"test",
		// 	bs,
		// 	Vector3(0,0,0),
		// 	SpaceGenLevel::MILLION_TO_BILLION
		// );


	}


	void spawn_player(shared_ptr<StellarCoordinate> player_location, LocationType locationType) {

		Transform3D player_start_transform; 
		player_start_transform.origin = Vector3(0, 0, 0);

		// if (location_type == LocationType::SPACE) {
		// 	player_start_transform = this->world->init_for_space(player_location);
		// }

		// MAKE player
		this->player = memnew(Player);
		this->player->init(this->world, this->world->planet, player_location, locationType);
		add_child(this->player);
		player->set_location_mode(locationType);

		// set player tranforms
		this->player->hit_box->set_transform(player_start_transform);

	}


	void spawn_test_block(shared_ptr<PositionDouble> position, double size) {
		// Ref<Mesh> mesh = MeshMaker::make_cube(
		// 	size * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE, 
		// 	"assets/textures/SAND.png", 
		// 	1
		// );
		// MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		// mesh_instance->set_mesh(mesh);

		// Vector3 start_pos = CoordinateConversion::convert_position_double_to_fVector(
		// 	position,
		// 	this->world->cubePool->terrainZeroZero,
		// 	this->world->cubePool->worldSize,
		// 	size
		// );
		// mesh_instance->set_global_position(start_pos);
		// this->add_child(mesh_instance);


		Ref<Mesh> selected_block = MeshMaker::make_cube(size * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE);
		Ref<Material> selected_block_material = MeshMaker::make_material("assets/textures/SAND.png");
		this->world->cubePool->place_block(position, size, selected_block, selected_block_material, true);

	}

	void mesh_test() {


        // Create the mesh data
		Ref<ArrayMesh> mesh;
		mesh.instantiate();

		PackedVector3Array vertices;
		PackedVector3Array normals;
		PackedInt32Array indices;

		float s = 0.2f; // half-size

		// Each face has 4 unique vertices (24 total)
		struct Face {
			Vector3 v0, v1, v2, v3;
			Vector3 normal;
		};

		// Define cube faces (counter-clockwise winding)
		Face faces[6] = {
			// Front (+Z)
			{{-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}, {0, 0, 1}},
			// Back (-Z)
			{{ s, -s, -s}, {-s, -s, -s}, {-s,  s, -s}, { s,  s, -s}, {0, 0, -1}},
			// Left (-X)
			{{-s, -s, -s}, {-s, -s,  s}, {-s,  s,  s}, {-s,  s, -s}, {-1, 0, 0}},
			// Right (+X)
			{{ s, -s,  s}, { s, -s, -s}, { s,  s, -s}, { s,  s,  s}, {1, 0, 0}},
			// Top (+Y)
			{{-s,  s,  s}, { s,  s,  s}, { s,  s, -s}, {-s,  s, -s}, {0, 1, 0}},
			// Bottom (-Y)
			{{-s, -s, -s}, { s, -s, -s}, { s, -s,  s}, {-s, -s,  s}, {0, -1, 0}}
		};

		for (int f = 0; f < 6; f++) {
			int base = vertices.size();
			vertices.push_back(faces[f].v0);
			vertices.push_back(faces[f].v1);
			vertices.push_back(faces[f].v2);
			vertices.push_back(faces[f].v3);

			for (int i = 0; i < 4; i++)
				normals.push_back(faces[f].normal);

			// Two triangles per face (quad)
			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 0);
			indices.push_back(base + 2);
			indices.push_back(base + 3);
		}

		// Pack into Array for Godot
		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = vertices;
		arrays[Mesh::ARRAY_NORMAL] = normals;
		arrays[Mesh::ARRAY_INDEX] = indices;

		// Add surface to mesh
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

		// Material
		Ref<StandardMaterial3D> material;
		material.instantiate();
		material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		material->set_albedo(Color(0.8, 0.8, 0)); 
		// mesh->surface_set_material(0, material);


		int grid_size = 100;
		cout << grid_size * grid_size * grid_size << endl;
		float space = 1.1;
		int i = 0;
		for (int x = 0; x < grid_size; x++) {
			for (int y = 0; y < grid_size; y++) {

				Ref<MultiMesh> multimesh;
				multimesh.instantiate();
				multimesh->set_transform_format(MultiMesh::TransformFormat::TRANSFORM_3D);
				multimesh->set_mesh(mesh);
				multimesh->set_instance_count(grid_size);

				for (int z = 0; z < grid_size; z++) {
					Transform3D t;
					t.origin = Vector3(x * space, y * space, z * space);
					multimesh->set_instance_transform(z, t);
				}
				MultiMeshInstance3D* mm_instance = memnew(MultiMeshInstance3D);
				mm_instance->set_multimesh(multimesh);
				mm_instance->set_material_override(material);
				add_child(mm_instance);
			}
		}



		cout << "hi" << endl;
	}

	void spawn_block(Transform3D transform, string material, double size) {

		Ref<Mesh> mesh = MeshMaker::make_cube(size * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE);
		Ref<Material> godot_material = MeshMaker::make_material(material);

		// make collider container
		StaticBody3D* collider_container = memnew(StaticBody3D);
		
		// spawn instance
		vector<Transform3D> transforms = {transform};
		MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
			this,
			godot_material,
			mesh,
			transforms,
			collider_container,
			size
		);

	}


	void random_test(shared_ptr<StellarCoordinate> player_location) {

		spawn_player(player_location, LocationType::SPACE);

		// spawn a ring of test blocks around the player, in the camera's field of view
		string material = BMaterial::SAND;
		double size = 0.5; // godot unit, LY-size → cube of size*game_scale (game_scale=1 → 0.5-unit cubes)
		auto blocks = std::make_shared<vector<shared_ptr<Block>>>();
		for (int i = 0; i < 10; i++) {
			double angle = i * (2 * M_PI / 10.0);
			double x = std::sin(angle) * 4.0;
			double z = std::cos(angle) * 4.0;
			blocks->push_back(SpaceGen::make_space_block(material, x, 1.5, z, size));
		}
		Vector3 offset = this->world ? this->world->player_location : Vector3();
		this->space_deferred->spawn_space_blocks("spawn_test", blocks, offset, 1.0);
	}
};



#endif
