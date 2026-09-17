// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "util/Position.h"
#include "util/PositionDouble.h"
#include <unordered_map>
#include <cmath>
#include "../blocks/Block.h"
#include <algorithm>
#include "Util.hpp"
#include "space/Planet.h"
#include "GodotUtil.hpp"


#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>


template<typename T>
using uptr = std::unique_ptr<T>;



using namespace std;
using namespace godot;


enum class LocationType {
	TERRAIN,
	SPACE
};

/**
 * How game distances are converted to godot distances in space gen rendering
 * 
 * This is how many kilometers the player is a from a celestial body for the 
 * different tier scaling.
 * 
 * Also in deep space instead of kilometers these can be used as light year distances instead
 * 
 * A light year is 9,460,730,472,580.8 km for reference. A galaxy can be up to 1 million lyr
 * across so that would be around 9,000,000,000,000,000,000 or 9 quintillian km across.
 * 
 * 9460730472580
 */
enum class SpaceGenLevel : int64_t {
	QUADRILLION_PLUS,
	TRILLION_TO_QUADRILLION,
	BILLION_TO_TRILLION,
	MILLION_TO_BILLION,
	THOUSAND_TO_MILLION,
	ZERO_TO_THOUSAND
};

/**
 * 
 */
class CoordinateConversion
{
public:



	// godot looses precision at +-10km 
	// on mars the elevation varies from about -7000km to 21000km (Hellas Planitia, Olympus Mons)
	// Veryovkina Cave in Abkhazia, Georgia reaches a depth of 2,212 meters
	// Asteriod 4 Vesta – Rheasilvia Central Peak Central peak: ~19–22 km high, Basin floor depth: ~13 km, Total relief (peak to floor): ~30–35 km
	// We want a max/min height of +-30,000 meters. 
	// So we'll just make 1 meter in Godot 4 meters in our game
	// so the max height of our game will be 10,000 * 4 = 40,000 m
	inline static double BLOCK_SCALE = 0.5; // Block 1 is 0.5 meters
	inline static double WORLD_SCALE = 0.25; // 1 m in godot is 4 m in our game

	CoordinateConversion() {};
	~CoordinateConversion() {};

	// offsets used during Position to Vector3 conversion to avoid block geometry faces from wigging out
	inline static unordered_map<int, float> block_size_to_micro_offset = {
		{ 1, 0.0001f },
		{ 4, 0.0002f },
		{ 16, 0.0003f }
	};





	static Vector3 convert_position_to_fVector(
		const shared_ptr<Position>& position, 
		shared_ptr<Position>& terrain_zero_zero,
		int world_size,
		int block_size
	) {
		return convert_position_double_to_fVector(
			std::make_shared<PositionDouble>(position->x, position->y, position->z),
			terrain_zero_zero,
			world_size,
			block_size
		);
	}

	static Vector3 convert_position_double_to_fVector(
		const shared_ptr<PositionDouble>& position, 
		shared_ptr<Position>& terrain_zero_zero,
		int world_size,
		double block_size
	) {
		shared_ptr<PositionDouble> wrappedPosition = CoordinateConversion::find_position_closest_to_player(
			position, 
			std::make_shared<PositionDouble>(terrain_zero_zero->x, terrain_zero_zero->y, terrain_zero_zero->z), 
			world_size
		);
		return CoordinateConversion::convert(
			wrappedPosition, 
			terrain_zero_zero, 
			block_size
		);
		// FTransform transform = FTransform(
		//     randomRotation(block->position, block->position->x).Quaternion(),
		//     fVector
		// );
		// return FTransform(fVector);
	}



	static shared_ptr<Position> convert_fVector_to_position(Vector3 position, shared_ptr<Position>& terrain_zero_zero, int64_t world_size, int block_size)
	{
		shared_ptr<PositionDouble> position_double = convert_fVector_to_position_double(
			position,
			terrain_zero_zero,
			world_size,
			block_size
		);

		return std::make_shared<Position>(floor(position_double->x), ceil(position_double->y), floor(position_double->z));
	}



	static shared_ptr<PositionDouble> convert_fVector_to_position_double(Vector3 position, shared_ptr<Position>& terrain_zero_zero, int64_t world_size, double block_size)
	{
		shared_ptr<PositionDouble> convert_position = CoordinateConversion::convert(
			position, 
			terrain_zero_zero, 
			block_size
		);

    	CoordinateConversion::wrap(
			convert_position->x,
			convert_position->z, 
			world_size
		);

		return convert_position;
	}


	static Vector3 convert(shared_ptr<PositionDouble> position, shared_ptr<Position>& terrain_zero_zero, const double& block_size)
	{
		
		//TODO convert these before hand instead of every method call. Also decide on a different factor and scale blocks for that. 
		// The most extreme elevations on mars are 21km and -7.5km
		// 60 km is probably our goal size, so we'd want to stack 120k of our smallest blocks to get there.
		// that'd be -15,000 to 15,000. 30,000 / 120,000 = 0.25 or 4 blocks per meter

		// factor in terrain offset from zero zero
		double manyworlds_x = position->x - terrain_zero_zero->x; // XXX: this should be scaled probably
		double manyworlds_y = position->y;
		double manyworlds_z = position->z - terrain_zero_zero->z;

		
		// apply micro offsets so geometry faces don't wig out
		double size_div_2 = block_size / 2.0f;
		size_div_2 += CoordinateConversion::block_size_to_micro_offset[block_size];

		double ue5_x = (manyworlds_x + size_div_2) * WORLD_SCALE * BLOCK_SCALE;
		double ue5_y = (manyworlds_y - size_div_2) * WORLD_SCALE * BLOCK_SCALE;
		double ue5_z = (manyworlds_z + size_div_2) * WORLD_SCALE * BLOCK_SCALE;

		
		// x:lr y:fb z:ud
		Vector3 fVector = Vector3(ue5_x, ue5_y, ue5_z);

		// flip x as ue5 does opposite x from us
		fVector.x = -fVector.x;

		return fVector;
	}


	inline static bool VERBOSE = false;
	static shared_ptr<PositionDouble> convert(Vector3 position, shared_ptr<Position>& terrain_zero_zero, double block_size)
	{

		// flip x as ue5 does opposite x from us
		position.x = -position.x;
		
		double size_div_2 = block_size / 2.0f;
		double manyworlds_x = (position.x / BLOCK_SCALE / WORLD_SCALE) - size_div_2;
		double manyworlds_y = (position.y / BLOCK_SCALE / WORLD_SCALE) + size_div_2;
		double manyworlds_z = (position.z / BLOCK_SCALE / WORLD_SCALE) - size_div_2;

		shared_ptr<PositionDouble> pos = std::make_shared<PositionDouble>(
			manyworlds_x, 
			manyworlds_y, 
			manyworlds_z
		);

		// if (VERBOSE) {
		// 	cout << " manyworlds_x " << manyworlds_x << " manyworlds_y " << manyworlds_y << " manyworlds_z " << manyworlds_z << endl;
		// 	cout << " ciel_x " << ceil(manyworlds_x) << " ciel_y " << ceil(manyworlds_y) << " floor_z " << floor(manyworlds_z) << endl;
		// 	cout << pos->toString() << endl;
		// }


		pos->x += terrain_zero_zero->x; // XXX: this should be scaled probably
		pos->z += terrain_zero_zero->z;


		return pos;
	}


	static shared_ptr<Position> wrap(shared_ptr<Position> position, int64_t worldSizeQuaterSizeBlocks)
	{
		wrap(position->x, position->z, worldSizeQuaterSizeBlocks);
		return position;
	}

	
	static void wrap(int64_t& x, int64_t& z, const int64_t& world_size)
	{
		x = wrap(x, world_size);
		z = wrap(z, world_size);
	}


	static int64_t wrap(int64_t v, int64_t world_size) {
		while (v < 0) v += world_size;
		if (v >= world_size) v %= world_size;
		return v;
	}


	static void wrap(double& x, double& z, const double& world_size)
	{
		while (x < 0) x += world_size;
		if (x >= world_size) x = fmod(x, world_size);
		
		while (z < 0) z += world_size;
		if (z >= world_size) z = fmod(z, world_size);
	}


	/*
		checks if a value is between two potentially wrapped values
	*/
	static bool is_between(double val, double start, double end, double error_margin=0.001) {
		
		// handle exact matches that wouldn't pass because of double precision errors
		if (val == start || val == end || abs(val-start) <= error_margin || abs(val-end) <= error_margin) {
			return true;
		}
		// if end isnt' wrapped compared to start
		else if (start < end) {
			return val > start && val < end;
		}
		// if end is wrapped compared to start
		else if (start > end) {
			return val > start || val < end;
		}

		return false; // not possible to get here
	}

	/*
	* Takes the max of the x and z dists.
	*
	* ( 2 sub, 2 abs )
	*/
	static int cheapMaxDistance(int x, int z, int otherX, int otherZ)
	{
		int xdist = abs(x - otherX);
		int zdist = abs(z - otherZ);
		return std::max({ xdist, zdist });
	}

	/**
	 * Converts a wrapped position to the closest position to the unwrapped 'playerPosition'.
	 */
	static shared_ptr<PositionDouble> find_position_closest_to_player(shared_ptr<PositionDouble> pos, const shared_ptr<PositionDouble> playerPosition, const int worldSize)
	{
		shared_ptr<PositionDouble> wrapX = std::make_shared<PositionDouble>(pos->x - worldSize, pos->y, pos->z);
		shared_ptr<PositionDouble> wrapZ = std::make_shared<PositionDouble>(pos->x, pos->y, pos->z - worldSize);
		shared_ptr<PositionDouble> wrapBoth = std::make_shared<PositionDouble>(pos->x - worldSize, pos->y, pos->z - worldSize);

		// TODO could speed this up with a function without wrapping as cheapMaxDistance does
		double distNone = cheapMaxDistance(playerPosition->x, playerPosition->z, pos->x, pos->z);
		double distWrapX = cheapMaxDistance(playerPosition->x, playerPosition->z, wrapX->x, wrapX->z);
		double distWrapZ = cheapMaxDistance(playerPosition->x, playerPosition->z, wrapZ->x, wrapZ->z);
		double distWrapBoth = cheapMaxDistance(playerPosition->x, playerPosition->z, wrapBoth->x, wrapBoth->z);

		double min = std::min({ distNone, distWrapX, distWrapZ, distWrapBoth });

		if (distNone == min) {
			return pos;
		}
		else if (distWrapX == min) {
			return wrapX;
		}
		else if (distWrapZ == min) {
			return wrapZ;
		}
		else if (distWrapBoth == min) {
			return wrapBoth;
		}

		return pos;
	}

	/**
	 * Wraps the coordinates to find the closest possible distance between the two points.
	 * 
	 * Excepts coordinates to be 0 - wrapValue
	 */
	
	static int cheapWrapMaxDistance(int x, int z, int otherX, int otherZ, int wrapValue)
	{

		/**
		 * 
		 * Player 0, 0
		 * - 0, 0 = 0
		 * - 0, 10 = 10
		 * - 0, max = 0
		 * - max, 0 = 0
		 * - max, max = 0
		 * - max - 10, max - 10 = 10
		 * 
		 * 
		 * Player 0, max
		 * - 0, 0 = 0
		 * - 0, 10 = 10
		 * - 0, max = 0
		 * - max, 0 = 0
		 * - max, max = 0
		 * - max - 10, max - 10 = 10
		 * 
		 * 
		 * Player max, 0
		 * - 0, 0 = 0
		 * - 0, 10 = 10
		 * - 0, max = 0
		 * - max, 0 = 0
		 * - max, max = 0
		 * - max - 10, max - 10 = 10
		 *
		 * 
		 * Player max, max
		 * - 0, 0 = 0
		 * - 0, 10 = 10
		 * - 0, max = 0
		 * - max, 0 = 0
		 * - max, max = 0
		 * - max - 10, max - 10 = 10
		 * 
		 */

		int xdist = std::min({ abs(x - otherX), abs(x - wrapValue - otherX), abs(x + wrapValue - otherX) });
		int zdist = std::min({ abs(z - otherZ), abs(z - wrapValue - otherZ), abs(z + wrapValue - otherZ) });
		return std::max({ xdist, zdist });

		// int xdist = std::min({ abs(x - otherX), abs(x - wrapValue - otherX) });
		// int zdist = std::min({ abs(z - otherZ), abs(z - wrapValue - otherZ) });
		// return std::max({ xdist, zdist });
	}


	/**
	 * Determines the offset 'other_val' is from val taking wrapping into account.
	 * 
	 * 'val' and 'other_val' should be positive.
	 * 
	 * The result will either be positive or negative
	 */
	static double wrap_offset(double val, double other_val, double wrap_value) {
		
		double offset = val - other_val;
		double offset_up = val + wrap_value - other_val;
		double offset_down = val - wrap_value - other_val;

		if (abs(offset_up) < abs(offset)) {
			offset = offset_up;
		}

		if (abs(offset_down) < abs(offset)) {
			offset = offset_down;
		}


		return offset;
	}

	// static shared_ptr<Position> convert_fVector_to_position(
	// 	Vector3& vector, 
	// 	shared_ptr<Position>& player_position, 
	// 	shared_ptr<Position>& terrain_zero_zero, 
	// 	int block_size,
	// 	int world_size
	// ) {
	// 	shared_ptr<Position> position = CoordinateConversion::convert(vector, terrain_zero_zero, block_size);
	// 	return CoordinateConversion::findPositionClosestToPlayer(position, player_position, world_size);
	// }


	static Vector3 determine_block_pos(Vector3& hit_pos, double block_godot_size) {

		/**
		 * 
		 * If we hit top
		 * 	- it should be really close to y position
		 * 
		 * If we hit side
		 * 	- y shouldn't be close
		 * 	- if close to front, right, left, back; then that's the side
		 * 
		 * If we hit bottom
		 * - y should be close to y posiiton - 1
		 * 
		 * 
		 */

		/*
		     -Z (North)
				^
				|
		-X(W) <-+-> +X(E)
				|
				v
			  +Z(S)

		*/

		double half_block = block_godot_size / 2;

		double west_x = Util::round_to_precision(hit_pos.x, block_godot_size) - half_block;
		double east_x = Util::round_to_precision(hit_pos.x, block_godot_size) + half_block;
		double up_y = Util::round_to_precision(hit_pos.y, block_godot_size);
		double down_y = Util::round_to_precision(hit_pos.y, block_godot_size) - block_godot_size;
		double north_z = Util::round_to_precision(hit_pos.z, block_godot_size) + half_block;
		double south_z = Util::round_to_precision(hit_pos.z, block_godot_size) - half_block;
		

		double west_diff = abs(west_x - hit_pos.x);
		double east_diff = abs(east_x - hit_pos.x);
		double up_diff = abs(up_y - hit_pos.y);
		double down_diff = abs(down_y - hit_pos.y);
		double north_diff = abs(north_z - hit_pos.z);
		double south_diff = abs(south_z - hit_pos.z);

		double min = Util::min(west_diff, east_diff, up_diff, down_diff, north_diff, south_diff);

		if (min == west_diff) {
			return Vector3(
				Util::floor_to_precision(hit_pos.x, block_godot_size) + half_block,
				Util::ceil_to_precision(hit_pos.y, block_godot_size),
				Util::floor_to_precision(hit_pos.z, block_godot_size)
			);
		}
		else if (min == east_diff) {
			return Vector3(
				Util::floor_to_precision(hit_pos.x, block_godot_size) - half_block,
				Util::ceil_to_precision(hit_pos.y, block_godot_size),
				Util::floor_to_precision(hit_pos.z, block_godot_size)
			);
		}
		else if (min == up_diff) {
			return Vector3(
				Util::floor_to_precision(hit_pos.x, block_godot_size),
				Util::ceil_to_precision(hit_pos.y, block_godot_size) - block_godot_size,
				Util::floor_to_precision(hit_pos.z, block_godot_size)
			);
		}
		else if (min == down_diff) {
			return Vector3(
				Util::floor_to_precision(hit_pos.x, block_godot_size),
				Util::ceil_to_precision(hit_pos.y, block_godot_size) + block_godot_size,
				Util::floor_to_precision(hit_pos.z, block_godot_size)
			);
		}
		else if (min == north_diff) {
				return Vector3(
					Util::floor_to_precision(hit_pos.x, block_godot_size),
					Util::ceil_to_precision(hit_pos.y, block_godot_size),
					Util::floor_to_precision(hit_pos.z, block_godot_size) + half_block
				);
		}
		else if (min == south_diff) {
			return Vector3(
				Util::floor_to_precision(hit_pos.x, block_godot_size),
				Util::ceil_to_precision(hit_pos.y, block_godot_size),
				Util::floor_to_precision(hit_pos.z, block_godot_size) - half_block
			);
		}

		return Vector3(-1,-1,-1);
	}


	static double space_scale(SpaceGenLevel level, double game_engine_value) {

		if (level == SpaceGenLevel::QUADRILLION_PLUS) {
			return game_engine_value * 1'000'000'000'000'000;
		}
		else if (level == SpaceGenLevel::TRILLION_TO_QUADRILLION) {
			return game_engine_value * 1'000'000'000'000;
		}
		else if (level == SpaceGenLevel::BILLION_TO_TRILLION) {
			return game_engine_value * 1'000'000'000;
		}
		else if (level == SpaceGenLevel::MILLION_TO_BILLION) {
			return game_engine_value * 1'000'000;
		}
		else if (level == SpaceGenLevel::THOUSAND_TO_MILLION) {
			return game_engine_value * 1'000;
		}

		return game_engine_value;
	}


	static double game_scale(SpaceGenLevel level, double space_value) {
		if (level == SpaceGenLevel::QUADRILLION_PLUS) {
			return space_value / 1'000'000'000'000'000;
		}
		else if (level == SpaceGenLevel::TRILLION_TO_QUADRILLION) {
			return space_value / 1'000'000'000'000;
		}
		else if (level == SpaceGenLevel::BILLION_TO_TRILLION) {
			return space_value / 1'000'000'000;
		}
		else if (level == SpaceGenLevel::MILLION_TO_BILLION) {
			return space_value / 1'000'000;
		}
		else if (level == SpaceGenLevel::THOUSAND_TO_MILLION) {
			return space_value / 1'000;
		}

		return space_value;
	}

	static SpaceGenLevel space_gen_level(double distance) {

		if (distance >= 1'000'000'000'000'000) {
			return SpaceGenLevel::QUADRILLION_PLUS;
		}
		else if (distance >= 1'000'000'000'000) {
			return SpaceGenLevel::TRILLION_TO_QUADRILLION;
		}
		else if (distance >= 1'000'000'000) {
			return SpaceGenLevel::BILLION_TO_TRILLION;
		}
		else if (distance >= 1'000'000) {
			return SpaceGenLevel::MILLION_TO_BILLION;
		}
		else if (distance >= 1'000) {
			return SpaceGenLevel::THOUSAND_TO_MILLION;
		}
		else {
			return SpaceGenLevel::ZERO_TO_THOUSAND;
		}
	}

	
	static Transform3D convert_position_to_space_transform(
		shared_ptr<Block> block,
		Vector3 position, 
		Vector3 planet_center,
		shared_ptr<Planet> planet,
		SpaceGenLevel level
	) {
		Transform3D transform;

		// modify position for chunk height and set
		double position_radius = planet->radius - block->size/2 + block->position->y;
		position_radius = CoordinateConversion::game_scale(level, position_radius);
		transform.origin = planet_center + position * position_radius;


		// get look rotation to center of planet
		Vector3 up = (position).normalized();
		Vector3 pole = Vector3(0,1,0);
		Vector3 right = pole - up * up.dot(pole);
		if (right.length() < 0.001) {
			right = Vector3(1,0,0).cross(up);
		}
		Vector3 forward = up.cross(right);
		transform.basis = Basis(right.normalized(), up.normalized(), -forward.normalized());
		// Quaternion rotToLookAtPlayer = Basis().looking_at(to_center.normalized(), GodotUtil::forward_v).get_quaternion();
		// transform.basis = Basis(rotToLookAtPlayer);

		// transform.basis = Basis::looking_at(to_center, GodotUtil::forward_v);

		return transform;
	} 


	static shared_ptr<Position> sphere_location_to_world_position(Vector3 sphere_location, shared_ptr<Planet> planet) {
		

		/*

			determine quadrant based on closest poles

			weigth between poles and 

			
		*/
		double EPSILON = 0.0000001;

		double dist_front_pole = sphere_location.distance_to(Vector3(0, 0, 1));
		double dist_back_pole = sphere_location.distance_to(Vector3(0, 0, -1));

		Vector2 cross_view = Vector2(sphere_location.x, sphere_location.y).normalized();
		double dist_left_pole = cross_view.distance_to(Vector2(-1, 0));
		double dist_right_pole = cross_view.distance_to(Vector2(1, 0));
		double dist_top_pole = cross_view.distance_to(Vector2(0, 1));
		double dist_bottom_pole = cross_view.distance_to(Vector2(0, -1));

		Vector2 angle;
		double magnitude;

		// DETERMINE ANGLE FROM RELATIVE DISTANCES TO SIDE POLES
		int64_t half_world_size = planet->worldSize / 2;

		// bottom left
		if (dist_left_pole <= dist_right_pole && dist_bottom_pole <= dist_top_pole) {
			Vector2 bottom = Vector2(0, -1);
			Vector2 left = Vector2(-1, 0);
			angle = bottom / (dist_bottom_pole + EPSILON);
			angle += left / (dist_left_pole + EPSILON);
		}
		// top left
		else if (dist_left_pole <= dist_right_pole && dist_top_pole <= dist_bottom_pole) {
			Vector2 top = Vector2(0, 1);
			Vector2 left = Vector2(-1, 0);
			angle = top / (dist_top_pole + EPSILON);
			angle += left / (dist_left_pole + EPSILON);
		}
		// top right
		else if (dist_right_pole <= dist_left_pole && dist_top_pole <= dist_bottom_pole) {
			Vector2 top = Vector2(0, 1);
			Vector2 right = Vector2(1, 0);
			angle = top / (dist_top_pole + EPSILON);
			angle += right / (dist_right_pole + EPSILON);
		}
		// bottom right
		else if (dist_right_pole <= dist_left_pole && dist_bottom_pole <= dist_top_pole) {
			Vector2 bottom = Vector2(0, -1);
			Vector2 right = Vector2(1, 0);
			angle = bottom / (dist_bottom_pole + EPSILON);
			angle += right / (dist_right_pole + EPSILON);
		}


		// convert angle to point along the edge of the square map
		if (abs(angle.x) > abs(angle.y)) {
			angle.x = angle.x == 0? angle.x + EPSILON : angle.x; // avoid divide by zero
			angle.y = half_world_size * angle.y / abs(angle.x); // find smaller dim length if large dim is half world size
			angle.x = half_world_size * angle.x / abs(angle.x); // set large dim to half world size preserving sign
		}
		else {
			angle.y = angle.y == 0? angle.y + EPSILON : angle.y; // avoid divide by zero
			angle.x = half_world_size * angle.x / abs(angle.y); // find smaller dim length if large dim is half world size
			angle.y = half_world_size * angle.y / abs(angle.y); // set large dim to half world size preserving sign
		}
			

		// DETERMINE SCALE FROM REALTIVE DISTANCE TO FRONT AND BACK POLES
		double scale = dist_front_pole / (dist_front_pole + dist_back_pole);
		int64_t x = angle.x * scale;
		int64_t z = angle.y * scale;
		x += half_world_size;
		z += half_world_size;


		
		return std::make_shared<Position>(x, 0, z);
	}

	static Vector3 world_position_to_sphere_location(shared_ptr<Position> position, shared_ptr<Planet> planet) {
		double current_dist = planet->worldSize + planet->worldSize;
		double step = 0.1;
		Vector3 closest;
		Vector3 location;
		for (double a_x = 0; a_x < 2*M_PI; a_x+=step) {
			location.x = sin(a_x);
			for (double a_y = 0; a_y < 2*M_PI; a_y+=step) {
				location.y = sin(a_y);
				for (double a_z = 0; a_z < 2*M_PI; a_z+=step) {
					location.z = sin(a_z);
					location.normalize();
					shared_ptr<Position> test_world_position = sphere_location_to_world_position(location, planet);
					double test_dist = position->manhatten(test_world_position);
					if (test_dist < current_dist) {
						current_dist = test_dist;
						closest = location;
					}
				}
			}

		}

		return closest.normalized();
	}

	static Vector3 world_position_to_sphere_location_old(shared_ptr<Position> position, shared_ptr<Planet> planet) {
		

		/*
			determine closesness to corners, midpoint, sides, top, bottom

			weigth those vectors accordingly and average

			normalize
		*/
		Position front = Position(planet->worldSize*0.5, 0, planet->worldSize*0.5);
		Position left_pole = Position(planet->worldSize*0.25, 0, planet->worldSize*0.5);
		Position right_pole = Position(planet->worldSize*0.75, 0, planet->worldSize*0.5);
		Position top_pole = Position(planet->worldSize*0.5, 0, planet->worldSize*0.75);
		Position bottom_pole = Position(planet->worldSize*0.5, 0, planet->worldSize*0.25);
		Position top = Position(position->x, 0, planet->worldSize);
		Position bottom = Position(position->x, 0, 0);
		Position left = Position(0, 0, position->z);
		Position right = Position(planet->worldSize, 0, position->z);

		double weight_front = front.manhatten(position);
		double weight_left_pole = left_pole.manhatten(position);
		double weight_right_pole = right_pole.manhatten(position);
		double weight_top_pole = top_pole.manhatten(position);
		double weight_bottom_pole = bottom_pole.manhatten(position);
		double weight_top = top.manhatten(position);
		double weight_bottom = bottom.manhatten(position);
		double weight_left = left.manhatten(position);
		double weight_right = right.manhatten(position);
		// double back_weight = Util::min(weight_top, weight_bottom, weight_left, weight_right);

		Vector3 front_v = Vector3(0, 0, 1) / (weight_front + 0.0001);
		Vector3 left_v = Vector3(-1, 0, 0) / (weight_left_pole + 0.0001);
		Vector3 right_v = Vector3(1, 0, 0) / (weight_right_pole + 0.0001);
		Vector3 top_v = Vector3(0, 1, 0) / (weight_top_pole + 0.0001);
		Vector3 bottom_v = Vector3(0, -1, 0) / (weight_bottom_pole + 0.0001);
		Vector3 sphere_location = front_v + left_v + right_v + top_v + bottom_v;

		sphere_location += Vector3(0, 0, -1) / (weight_top + 0.0001) / 5;
		sphere_location += Vector3(0, 0, -1) / (weight_bottom + 0.0001) / 5;
		sphere_location += Vector3(0, 0, -1) / (weight_top + 0.0001) / 5;
		sphere_location += Vector3(0, 0, -1) / (weight_left + 0.0001) / 5;
		sphere_location += Vector3(0, 0, -1) / (weight_right + 0.0001) / 5;
		// Vector3 back_v = Vector3(0, 0, -1) / (back_weight + 0.0001);
		// Vector3 sphere_location = front_v + left_v + right_v + top_v + bottom_v + back_v;
		sphere_location = sphere_location.normalized();

		return sphere_location;
	}

	
	static pair<double, double> get_lat_and_long(Vector3 offset_from_sphere_center) {
		offset_from_sphere_center = offset_from_sphere_center.normalized();
		double lat = asin(offset_from_sphere_center.y);
		double longi = atan(offset_from_sphere_center.z/Util::no_zero(offset_from_sphere_center.x));

		return std::pair<double, double>(lat, longi);
	}




	static Quaternion convert_pointing_rotation_to_fquat(
		const shared_ptr<PositionDouble>& position, 
		const PositionDouble& pointing_direction
	) {
		// FVector up = FVector(0, 0, 1).GetSafeNormal();
		// FVector pointing_dir = FVector(pointing_direction.x - position->x, pointing_direction.z - position->z, pointing_direction.y - position->y).GetSafeNormal();

		// float dot = FVector::DotProduct(up, pointing_dir);
		// FVector cross = FVector::CrossProduct(up, pointing_dir);
		// float w = dot + FMath::Sqrt(up.SizeSquared() * pointing_dir.SizeSquared());
		// FQuat q(cross.X, cross.Y, cross.Z, w);

		// return q.GetNormalized();

		// World up vector
		Vector3 up(0, 0, 1);
		up = up.normalized();

		// Direction from position to target
		Vector3 dir = Vector3(pointing_direction.x - position->x, pointing_direction.z - position->z, pointing_direction.y - position->y).normalized();

		// Dot and cross product
		float dot = up.dot(dir);
		Vector3 cross = up.cross(dir);

		// w component of quaternion
		float w = dot + Math::sqrt(up.length_squared() * dir.length_squared());

		// Construct quaternion
		Quaternion q(cross.x, cross.y, cross.z, w);

		return q.normalized();
	}



};
