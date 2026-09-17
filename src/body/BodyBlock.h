// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/classes/base_material3d.hpp>

#include "util/Position.h"
#include "util/PositionDouble.h"




using namespace std;
using namespace godot;


/**
 * 
 */
class BodyBlock
{
public:
	inline static double SMALLEST_BLOCK_SIZE = 0.25;

	Ref<Material> material;
	shared_ptr<PositionDouble> position;
	Quaternion rotation;
	double size;

	shared_ptr<Position> rel_pos_in_blocks;



	BodyBlock() = default;
	BodyBlock(Ref<Material> material, shared_ptr<PositionDouble> position, Quaternion rotation, double size) {
		this->material = material;
		this->position = position;
		this->rotation = rotation;
		this->size = size;
	}
	BodyBlock(Ref<Material> material, shared_ptr<PositionDouble> position, double size) {
		this->material = material;
		this->position = position;
		this->rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		this->size = size;
	}
	


	~BodyBlock() = default;


	shared_ptr<Position> calc_relative_position_in_size_blocks() {
		return std::make_shared<Position>(
			round((this->position->x + parent_start->x) / this->size), 
			round((this->position->y + parent_start->y) / this->size), 
			round((this->position->z + parent_start->z) / this->size)
		);
	}


	string tag(int64_t offset_x, int64_t offset_y, int64_t offset_z) {
		shared_ptr<Position> pos = get_relative_position_in_size_blocks();
		char buf[96]; // enough for four 64-bit ints + spaces
		int len = snprintf(buf, sizeof(buf), "%d %" PRId64 " %" PRId64 " %" PRId64, this->size, this->rel_pos_in_blocks->x, this->rel_pos_in_blocks->y, this->rel_pos_in_blocks->z);
		return string(buf, len);
	}


	string tag() {
		return this->tag(0,0,0);
	}




};
