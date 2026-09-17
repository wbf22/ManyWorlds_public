// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Random.h"
#include <string>
#include "space/Planet.h"
#include "WoodSlice.h"
#include <unordered_map>
#include "../blocks/Block.h"
#include <cmath>
#include "util/Offset.h"
#include "util/TrigApprox.h"
#include <algorithm>
#include "util/Position.h"
#include "Rot.h"
#include "util/PositionDouble.h"
#include <tuple>
#include "util/BMaterial.hpp"
#include "util/Pool.h"
#include "util/PositionMap.h"
#include "Leaf.hpp"



template<typename T>
using uptr = std::unique_ptr<T>;





using namespace std;




/**
 * Trunk of a plant. This part only extends from the ground to the point where major branches start coming off.
 */
class Trunk
{
public:
	Trunk() = default;
	
	/**
	 *
	 * @param seed (some kind of seed probably based off the seed and the nth tree you're generating)
	*/
	Trunk(int64_t seed, double gravity, double how_advanced)
	{
		this->seed = seed;

		this->stoutness = Random::randInt( seed, 30, 60, 90, 70);
		this->pointiness = (this->stoutness < 50)? Random::randInt( seed + 16, 0, 0, 100, 90) : Random::randInt( seed + 16, 0, 50, 100, 90);

		int minHeight = 1;
		int maxHeight = (1.0 / gravity) * (how_advanced / 5.0)  * 800;

		this->height = Random::randInt( seed + seed, minHeight, maxHeight);

		// TODO determine this later
		this->barkTexture = BMaterial().DEFAULT;

		this->hardness = Random::randInt( seed + 1, 0, 10);

		this->straightness = Random::randDouble( seed + 2, 0, 0.2, 10);
		this->grainTwist = Random::randInt( seed + 3, 0, 10);

		this->branchiness = Random::randInt( seed + 5, 0, 100);
		this->splitTrunk = Random::randInt( seed + 7, 0, 2) == 0;
		this->branchDirection = Random::randDouble( seed + 9, 0.5, 1, 2.6);
		this->canopyHeightRatio = Random::randInt( seed + 18, 0, 100);
		this->branchSplittingType = Random::randInt(seed + 17, 0, 5);

		this->rootVisibility = Random::randInt( seed + 13, 0, 10);
		this->hangingRootHeight = Random::randInt( seed + 14, 0, this->height);

		this->layered = Random::randInt(seed + 15, 0, 100) < 50;
	}

	~Trunk() { }


    // for random functions
	int64_t seed;

	// basic features
	int height; // height of the main trunk (the canopy can extend higher)

	// main trunk
	int stoutness; // 0 is a inverted cone, 50 is straight pole, 100 is a cone
	int pointiness; // 0 topwidth is determine by height and stoutness, 10 topwidth is zero
    double straightness; // 0 is straight, 100 is windy
	int grainTwist; // 0 is twisty, 10 is straight grain
	vector<shared_ptr<WoodSlice>> woodSlices; // list of wood slices from height 0 to trunk height
	unordered_map<string, shared_ptr<Block>> blocks; // map of position string (offset from tree origin) to block

	// branches
	int branchiness; // 0 is no branches, 100 is a bramble
	bool splitTrunk; // trunk splits at every node, or branches split off main trunk (like a pine or conifer)
	double branchDirection; // slope of the line that branches come off of the trunk (-3 to 3)
	int canopyHeightRatio; // 1 is disk on top of tree, 100 is the entire height of the tree (fir tree)
	int branchSplittingType; // 0 split vertical, 1 split horizontal, 2 split random, 3 split for light, 4 split angle x

	// roots
	int rootVisibility; // 0 is roots not visible, 10 is roots visible above ground to the height of the tree
	int hangingRootHeight = 0; 	

	// textures
	string barkTexture;
	int hardness; // 0 is pith or foam, 10 is crystal
	bool layered; // layered is like a palm tree with repeating segments.



	// get blocks features
	// shared_ptr<Rot> rotation;


};
