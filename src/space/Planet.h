// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "util/Random.h"
#include "util/BMaterial.hpp"
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include "util/serialization/KVSerializer.hpp"
#include "server/Interface.hpp"
#include "util/StellarCoordinate.hpp"

#include "util/serialization/KVSerializer.hpp"
#include "economies/Economies.hpp"


template<typename T>
using uptr = std::unique_ptr<T>;



using namespace std;

struct Star; // Forward declaration
struct SolarSystem; // Forward declaration
class Chunk; // Forward declaration
struct City; // Forward declaration
enum class ChunkType : int;

/**
 * 
 */
class Planet : public Economy {
public:
	Planet(int64_t seed, shared_ptr<StellarCoordinate> location);
	~Planet();

	shared_ptr<StellarCoordinate> location;

	std::weak_ptr<SolarSystem> parent; // owning solar system


	int64_t seed = 45654234455L;//1112342341234455L;
	int64_t worldSize; // (circumferance) earth is 40,075 km around, so that'd be: 40,075 * 1,000 = 40,075,000 meters. 
	double radius;
	// So the world size for earth would be 80,150,000 
	// our 2097152 chunks are 1,048,576 meters across, so earth would be ~38 2097152 chunks across

	// unreal looses precision at 20km https://forums.unrealengine.com/t/general-precision-guidelines-how-far-up-or-down-in-world-units-before-problems-arise/47347
	// 21 km, but there is a beta for large worlds https://docs.unrealengine.com/5.0/en-US/large-world-coordinates-in-unreal-engine-5/
	// olympus mons is 22 km above surrounding plains
	// Veryovkina Cave in Abkhazia, Georgia reaches a depth of 2,212 meters
	// beta is 88 million km
	// We want a max height of 60,000 meters. Our 0.5 meter block is 0.25 meters in unreal units
	// so the max height unreal can do in our units is 20,000 * 4 = -80,000 m to 80,000 m
	
	inline static int64_t minHeight = -15000;
	inline static int64_t maxHeight = 15000;
	// inline static int64_t minHeight = -30000;
	// inline static int64_t maxHeight = 30000;

	// 0 complete desert, 4 normal desert, 25 normal temperate, 50 rain forest, 100 constant hurricane
	int64_t precipitation = 20;

	// this is the sea level above the min height of the 2097152 chunks
	int64_t seaLevel = -10000;

	double gravity = 9.8;
	shared_ptr<Star> star;
	int64_t solarIntensity;
	int64_t magneticFieldStrenth; // more northern lights with atmosphere, strong sun, strong mag field
	int64_t dayLength;
	int64_t axis_tilt = 0; // 0 - 90 degrees
	int64_t rotation_speed; // km/h
	bool rotateClockwise;
	int64_t temperature; // -270 C to 1000 C (average temperature of the planet)

	// atmosphere
	int64_t dustLevel = 0; // brown or desaturate colors

	// normal atmospheres
	int64_t o2 = 0; // blue
	int64_t n2 = 0; // blue
	int64_t methane = 0; // blue green
	int64_t h2 = 0; // clear
	int64_t he = 0; // clear
	int64_t ammonia = 0; // white brown red
	int64_t h2o = 0; // blue
	int64_t co2 = 0; // clear
	int64_t sulfericAcid = 0; // white or yellow color
	int64_t sulfer = 0; // so2/h2s yellow or orange
	int64_t phosphorus = 0; // white or red
	int64_t chlorine = 0; // yellow green
	int64_t bromine = 0; // deep orange or red
	int64_t idoine = 0; // violet or purple

	// super hot atmospheres
	int64_t sodium_potassium = 0; // orange or yellow
	int64_t iron = 0; // orange or yellow
	int64_t titanium = 0; // gray brown but metallic
	int64_t silicate = 0; // white or gray



	int64_t windSpeeds;
	int64_t atmosphericPressure = 0; // 0-100, just arbitrary numbers, but 100 would be like super intense like jupiter or something. Earth would be like 2


	// life
	bool hasLife = false;
	int64_t evolutionStage; // 0-10, 10 has more iterations on plants and animals?
	vector<WoodMaterialType> wood_materials;


    double max_134217728 = 99.99;
    double max_16777216 = 99.99;
    double max_2097152 = 99.99;
    double max_131072 = 99.99;
    double max_8192 = 99.99;
    double max_512 = 99.999;
    double max_64 = 100;
    double max_16 = 100;
    double max_4 = 100;
    double max_1 = 100;

    double min_134217728 = 95;
    double min_16777216 = 95;
    double min_2097152 = 95;
    double min_131072 = 97;
    double min_8192 = 98;
    double min_512 = 99.81;
    double min_64 = 99.9999;
    double min_16 = 100;
    double min_4 = 100;
    double min_1 = 100;

    DECLARE_NAMED_FIELDS(
        FIELD("max_134217728", &Planet::max_134217728),
        FIELD("max_16777216", &Planet::max_16777216),
        FIELD("max_2097152", &Planet::max_2097152),
        FIELD("max_131072", &Planet::max_131072),
        FIELD("max_8192", &Planet::max_8192),
        FIELD("max_512", &Planet::max_512),
        FIELD("max_64", &Planet::max_64),
        FIELD("max_16", &Planet::max_16),
        FIELD("max_4", &Planet::max_4),
        FIELD("max_1", &Planet::max_1),

        FIELD("min_134217728", &Planet::min_134217728),
        FIELD("min_16777216", &Planet::min_16777216),
        FIELD("min_2097152", &Planet::min_2097152),
        FIELD("min_131072", &Planet::min_131072),
        FIELD("min_8192", &Planet::min_8192),
        FIELD("min_512", &Planet::min_512),
        FIELD("min_64", &Planet::min_64),
        FIELD("min_16", &Planet::min_16),
        FIELD("min_4", &Planet::min_4),
        FIELD("min_1", &Planet::min_1)
    );

	shared_ptr<Chunk> rootChunk;

	vector<shared_ptr<City>> cities;

    vector<shared_ptr<Economy>> get_child_economies() override;

};

