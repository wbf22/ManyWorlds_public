// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Position.h"
#include "util/PositionDouble.h"
#include <sstream>
#include <iomanip> 
#include <cinttypes>
#include "util/serialization/KVSerializer.hpp"





using namespace std;


/**
 * 
 */
class Block
{
public:

	Block(string material, shared_ptr<Position> position, shared_ptr<PositionDouble> position_double, double size);
	Block();
	~Block() = default;


	string material;
	shared_ptr<Position> position;
	shared_ptr<PositionDouble> position_double;
	double size = 0.0;        // physical size in the block's source coordinate system
	double render_size = 0.0; // optional Godot-space size for space rendering
	bool is_surface_patch = false; // terrain surface mesh rather than a cube
	


	int getSize();


	
	int64_t getId();
	
	string stringTag();

	static string stringTag(int size, int64_t x, int64_t y, int64_t z);

	static string stringTag_double(double size, double x, double y, double z);

	static void parseStringTagDouble(const string &tag, double &size, double &x, double &y, double &z);

	static void parseStringTag(const string& tag, int64_t& size, int64_t& x, int64_t& y, int64_t& z);
	
	string stringTagNoY();

	static string stringTagNoY(int size, int64_t x, int64_t z);

	static void parseStringTagNoY(const string& tag, int& size, int64_t& x, int64_t& z);

	shared_ptr<Block> duplicate();



    DECLARE_NAMED_FIELDS(
        FIELD("material", &Block::material),
        FIELD("position", &Block::position),
        FIELD("position_double", &Block::position_double),
        FIELD("size", &Block::size)
    );
};
