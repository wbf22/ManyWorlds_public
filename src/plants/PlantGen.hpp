// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "../blocks/WorldBlockPool.hpp"
#include "space/Planet.h"
#include "Plant.hpp"
#include "../blocks/Block.h"
#include "../blocks/Block.h"
#include "../chunks/Chunk.h"
#include <vector>
#include <deque>




template<typename T>
using uptr = std::unique_ptr<T>;





class WorldBlockPool;


using namespace std;

struct PlantInstance {
	int species;
	shared_ptr<Position> position;
	PlantType type;
	bool high_res;
};


/**
 * 
 */
class PlantGen
{
public:

	inline static const int ALL_TREES_RENDER_DISTANCE = 512;//Chunk::RENDER_DISTANCES[2];
	inline static const int FULL_DEFINTION_RENDER_DISTANCE = 128;
	inline static const int TICKS_BETWWEN_SPANWS = 32;

	// gen
	vector< unordered_map<PlantType, shared_ptr<Plant>> > plants;
	shared_ptr<Planet> planet;
	int max_height = 0;
	
	// spawning
	deque<pair<string, PlantInstance>> plants_to_spawn; // position tag -> tree_index
	unordered_map<string, PlantInstance> plants_in_world; // position tag -> tree_index
	unordered_map<string, PlantInstance> plants_to_despawn; // position tag -> tree_index
	unordered_set<shared_ptr<Chunk>> chunk_64_to_gen;


	// leaves
	vector<Block> pine = {
		{"pine", std::make_shared<Position>(0,0,0), nullptr, 1},

		{"pine", std::make_shared<Position>(1,0,1), nullptr, 1},
		{"pine", std::make_shared<Position>(0,0,1), nullptr, 1},
		{"pine", std::make_shared<Position>(-1,0,1), nullptr, 1},

		{"pine", std::make_shared<Position>(-2,0,2), nullptr, 1},
		{"pine", std::make_shared<Position>(-1,0,2), nullptr, 1},
		{"pine", std::make_shared<Position>(0,0,2), nullptr, 1},
		{"pine", std::make_shared<Position>(1,0,2), nullptr, 1},
		{"pine", std::make_shared<Position>(2,0,2), nullptr, 1},

		{"pine", std::make_shared<Position>(2,0,3), nullptr, 1},
	};
	unordered_map<string, vector<Block>> leaves = {
		{"pine", this->pine}
	};


	PlantGen(shared_ptr<Planet>& planet)
	{
		this->planet = planet;


		if (planet->hasLife) {

			// generate plants for this planet
			int num_species = planet->worldSize * 0.0000012476;
			if (num_species < 50) num_species = 50;

			// int species = 35;
			for (int species = 0; species < num_species; species++)
			{
				unordered_map<PlantType, shared_ptr<Plant>> species_plants;
				species_plants[PlantType::BABY] = make_shared<Plant>(species, PlantType::BABY, planet);
				species_plants[PlantType::FIELD] = make_shared<Plant>(species, PlantType::FIELD, planet);
				species_plants[PlantType::FOREST] = make_shared<Plant>(species, PlantType::FOREST, planet);

				plants.push_back(species_plants);

				int plant_height = species_plants[PlantType::FOREST]->trunk->height;
				if (plant_height > this->max_height) {
					this->max_height = plant_height;
				}
			}

		}

	}

	PlantGen() {}

	~PlantGen() {}


	

};
