// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Position.h"
#include "space/Planet.h"
#include "util/Random.h"
#include "util/Offset.h"
#include <array>
#include <stack>
#include "DownBlock.h"
#include "../blocks/Block.h"
#include "util/BMaterial.hpp"
#include "WaterContract.h"
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include "WaterBlock.h"
#include <cmath>
#include "util/CoordinateConversion.hpp"



template<typename T>
using uptr = std::unique_ptr<T>;



template<typename T, typename U>
using omap = std::map<T, U>;


using namespace std;

class Planet; // Forward declaration
class GenResult;

enum class PlantType {
    BABY,
    FOREST,
    FIELD
};

struct PlantPlacement {
    int species = 0;
    shared_ptr<Position> position;
    PlantType type = PlantType::BABY;
};


enum class ChunkType : int {
	NONE = -2,
	CHUNK_1 = 1,
	CHUNK_4 = 4,
	CHUNK_16 = 16,
	CHUNK_64 = 64,
	CHUNK_512 = 512,
	CHUNK_8192 = 8192,
	CHUNK_131072 = 131072,
	CHUNK_2097152 = 2097152,
	CHUNK_16777216 = 16777216,
	CHUNK_134217728 = 134217728,
	ROOT = -1
};

ChunkType smaller(ChunkType type);
ChunkType larger(ChunkType type);
int level(ChunkType type);


/**
 *
 */
class Chunk : public Block
{

public:


	static unordered_map<ChunkType, int> RENDER_DISTANCES;
	static unordered_map<ChunkType, int> SPACE_RENDER_DISTANCES;
	static unordered_map<ChunkType, int> CHUNK_SIZES;
	static unordered_map<ChunkType, int> LEVEL_SUB_CHUNKS;

	static unordered_map<ChunkType, double> CHUNK_SMOOTHNESS_MIN;
	static unordered_map<ChunkType, double> CHUNK_SMOOTHNESS_MAX;
	static int MAX_SMOOTHNESS;
	inline static ChunkType EROSION_LEVEL = ChunkType::CHUNK_8192;

	static BMaterial MATERIALS;

	weak_ptr<Chunk> rootChunk; // non-owning link to the root ancestor (breaks the parent->child / child->root cycle)
	int64_t world_size;

	// root's direct-child (top data) tier, adaptive to planet size
	ChunkType subChunkType = ChunkType::NONE;
	ChunkType topType();
	static ChunkType topTypeFor(int64_t worldSize);


	/*
	* chunk stats
	*/
	ChunkType type;
	double smoothness = 0;
	vector<shared_ptr<Chunk>> subChunks = {};
	//int SWCornerHeight = std::numeric_limits<int>().min(); // height of SW corner, this should give chunks a less square like nature

	unordered_map<int, int> inFlows; // (index of subchunk, flow amount) inflow heights are always the height of the chunk
	int outFlow = -1; // out flow heights are always the height of the parent chunk
	int flowSide = -1;
	int totalFlow = 0;
	unordered_map< int, shared_ptr<WaterContract> > externalFlowBlockers; // (index, contract) if a subchunk is on the edge of it's parent, the neighbors subchunk has to be loaded first to determine where the river goes
	double lakiness = 0.9; //0.0-1.0 determines how prone to lakes an area is
	vector<shared_ptr<WaterBlock>> waterBlocks;
	int lakeLevel = 0; // the level of the lake, if there is one
	

	int64_t precipitation = -1; // ( 4 desert, 15 semi arid, 35 normal temperate, 80 rain forest, 100 constant hurricane ) * chunk area
	int vegetationLevel = 0; // XXX maybe use this to affect erosion

	vector<shared_ptr<DownBlock>> downBlocks;

	// rock layers
	double rock_slope;
	double angle; // 0-180 of which way the rock layer slope goes
	int y_offset_rock_layers;

	// substrates
	double slope;
	int64_t highest_neighbor_height;
	string substrate_material;
	int substrate_depth;

	// (only in root chunk)
	vector<int> rockLayerHeights;
	vector<string> rockLayerTypes;

	/*
	* generation values
	*/
	bool initialized = false; // this flag implies the chunk has been generated (height, rock layers, etc) but children may or may not have been generated
	bool subChunksInitialized = false; // this flag implies this chunk has intialized it's sub chunks heights, meaning each of those chunks is ready to load
	unordered_map<int, int64_t> pending_subchunk_heights;
	struct PendingSubchunkContract {
		unordered_map<int, int> inFlows;
		int outFlow = -1;
		int flowSide = -1;
		int totalFlow = 0;
		bool hasOutFlow = false;
	};
	unordered_map<int, PendingSubchunkContract> pending_subchunk_contracts;

	// Corner heights for the coarse terrain surface outside detailed terrain.
	int surface_sw_height = 0;
	int surface_nw_height = 0;
	int surface_ne_height = 0;
	int surface_se_height = 0;


	/*
	* plants
	*/
	// Species weights are generated at every chunk level and drive coarse views.
	unordered_map<int, double> ratios_plant_types;
	vector<PlantPlacement> plant_placements;
	bool plant_placements_generated = false;
	// Legacy placement index retained for serialized callers.
	unordered_map<shared_ptr<Position>, int> plantPositions;


	// river and lakes stuff, only used for 512 chunks
	 unordered_map<int, vector<int>> river_starts__to_rivers;

	// space stuff
	vector<Vector3> space_positions;
	vector<string> space_group_ids;



	Chunk(int parentSmoothness, ChunkType type, shared_ptr<Position> position, int64_t seed, weak_ptr<Chunk> rootChunk, int precipitation);

	~Chunk() {};

	shared_ptr<Chunk> newSubChunk(int parentSmoothness, shared_ptr<Position> subChunkPos, int64_t seed);
	double vegetation_amount();
	double getNeighborPrecipitationInterpolation(const vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Position>& subChunkPos, int worldSize);
	void generate_plant_ratios(const shared_ptr<Planet>& planet, int species_count, int64_t interp_precipitation);
	void generate_plant_placements(const shared_ptr<Planet>& planet, int species_count);
	// partition this chunk's placements into its subchunks (512 -> 64 -> 16)
	void distribute_plant_placements();
	// species pool for a chunk of this type (scales with the chunk size, min 8)
	int plant_species_count(const shared_ptr<Planet>& planet);

	shared_ptr<Chunk> genSubChunks(
		const shared_ptr<Position>& playerPosition,
		const shared_ptr<Planet>& planet
	);

	void makeEmptySubChunks(int square, int numSubChunks, const shared_ptr<Planet>& planet);
	void setSubChunkHeight(int index, int64_t height);
	void setPendingSubchunkInflow(int childIndex, int inflowIndex, int flowAmount);
	void setPendingSubchunkOutflow(int childIndex, int outflowIndex, int flowSide, int flowAmount);

	void initRockLayers(Chunk* parentChunk, vector<shared_ptr<Chunk>> parentNeighborChunks, const shared_ptr<Planet>& planet);

	void initCornerHeight(const shared_ptr<Planet>& planet);

    vector<shared_ptr<DownBlock>> determineDownBlocks(vector<shared_ptr<Chunk>>& loaded_in_neighbors, const shared_ptr<Planet>& planet);

	void addDownBlocks( 
		int chunk_size,
		int min,
		const shared_ptr<Planet>& planet 
	);

    shared_ptr<DownBlock> makeDownBlock(int height, string& material_string, int this_chunk_size, const shared_ptr<Planet>& planet);


    // TERRAIN GEN

	int makeTerrain(const shared_ptr<Planet>& planet);

	void makeIndicePaths(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, const shared_ptr<Planet>& planet);

	void initEdgeHeights(int sideStart, int sideFinish, int step, int neighborNesestToStartHeight, int neighborNearestToFinishHeight, const shared_ptr<Planet>& planet);

	void gradeChunk(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, const shared_ptr<Planet>& planet);

	void interpolate_with_cliffs(vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Planet>& planet);

	void erodeFlowPathsAndFill(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, int erosionResistance, const shared_ptr<Planet>& planet);

	void makeLakes(unordered_map<int, int>& indexToNextIndex, const shared_ptr<Planet>& planet);



	// TERRAIN HELPERS

	pair<int, int> getNearestFlowsToStartAndFinishOnSide(int sideStart, int sideFinish, int square);

	int interpolate(int index, int first, int second, int firstHeight, int secondHeight, int square);

	int interpolateWithIndices(int index, vector<int> interpolateBetween, unordered_map<int, int> dists, int square);

	int getNeighborChunkInterpolation(const vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Position>& subChunkPos, int worldSize);

	static int interpolate(vector<shared_ptr<Position>> positions, const shared_ptr<Position>& target_position, int worldSize);

	int getEdgesAndMiddleInterpolation(int index, int square);

	void makeWaterContracts(int index, int nextIndex, int flowAmount, int square, int childNumSubChunks, int childSquare, const shared_ptr<Planet>& planet);

	void makeWaterContractsCrossParent(int index, int side, bool isOutFlow, int flowAmount, shared_ptr<Chunk>& parentNeighbor, int childNumSubChunks, int childSquare);

	void matchBlockers(int subOutFlow, shared_ptr<Chunk>& currentChunk, shared_ptr<Chunk>& nextChunk, int subNeighborInflow);

	static int flowDir(int index, int nextIndex, int square);

	static int matchFlowToFlow(int flowIndex, int side, int numSubchunks, int square);

	static int getSubOutFlow(int index, int side, int childNumSubChunks, int childSquare);

	static int sideOfEdgeIndex(int index, int numSubChunks, int square);

	static int getFlowSide(int64_t x, int64_t z, int64_t other_x, int64_t other_z, int64_t size, int64_t world_size);


	static int weightInterpolationAndRandom(int interpolation, int random, double smoothness, int64_t seed, bool allow_anomalies);

	static int calculate_flow_amount(ChunkType type, int precipitation);

	static int calculate_added_flow_amount(int dist_from_start, int square, int precipitation);


	// GENERAL USE HELPERS

	vector<pair<int, int>> getNeighborHeightsInChunk(int index, int numSubChunks, int square);

	static vector<int> getNeighbors(int currentIndex, int numSubChunks, int square);

	vector<int> getAll9Neighbors(int currentIndex, int numSubChunks, int square);

	vector<int> getNeighborsWithNE(int currentIndex, int numSubChunks, int square);

	int getMaterialHardness();

	static bool isEdgeOfChunk(int candidate, int numSubChunks, int square);

	static int cheapMaxDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ);

	static int cheapMaxDistance(int index, int otherIndex, int square);

	shared_ptr<Position> convertIndex(int index);

	static void convertIndex(int index, int arraySqrtRoot, int& x, int& z);

	static int convertToIndex(int64_t x, int64_t z, int arraySqrtRoot);

	static int manhattenDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ);

	static int wrapManhattenDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ, int64_t wrapValue);

	int64_t getId();

	shared_ptr<Chunk> getParent();


	// DEBUG

	string toString(string file_path="src/TEST/output/chunkOut.txt");

	string parentToString(const shared_ptr<Planet>& planet);

	static void dump_to_string(int x, int z, const shared_ptr<Planet>& planet);

	int getIndexInParent(shared_ptr<Chunk> parent, const shared_ptr<Planet>& planet);



	// Other

	virtual int getSquareOfSub();

	int getLevel();

	ChunkType getType();

	shared_ptr<Chunk> getSubChunk(Offset& offset);

	static void get_coordinates_by_chunk_type(ChunkType type, int64_t& x, int64_t& z);

	static string chunk16_group_id(double x, double z);

	static string type_group_id(ChunkType type, double x, double z);

	static int64_t render_distance_for_position(int64_t x, int64_t z, int64_t player_x, int64_t player_z, shared_ptr<Planet>& planet);


	vector<shared_ptr<Chunk>> getChunkNeighbors_N_E_S_W_NE(ChunkType type, shared_ptr<Position>& pos);

	static vector<Offset> getPath(ChunkType type, int64_t x, int64_t z, ChunkType startType);

	shared_ptr<Chunk> getChunk(ChunkType type, int64_t x, int64_t z);
	
	vector<shared_ptr<Chunk>> getNESWNeighborsInitialized(ChunkType type, shared_ptr<Position>& pos);

	shared_ptr<Chunk> getNeighborInitialized(shared_ptr<Chunk> chunk, int64_t side);

	shared_ptr<Chunk> getChunkOrClosestAncestorInitialized(ChunkType type, int64_t x, int64_t z);

	vector<shared_ptr<Chunk>> getNeighborsNESWOrAncestorsInWorld(ChunkType type, shared_ptr<Position>& pos, const unordered_map<string, shared_ptr<Block>>& blocks_in_world);

	shared_ptr<Chunk> getChunkOrClosestAncestorInWorld(ChunkType type, int64_t x, int64_t z, const unordered_map<string, shared_ptr<Block>>& blocks_in_world);

	vector<shared_ptr<Chunk>> getChunksAtPosition(int64_t x, int64_t z);

	vector<shared_ptr<Chunk>> getParents(ChunkType type, int64_t x, int64_t z);

	vector<shared_ptr<Chunk>> getChunksInRadius(ChunkType type, int64_t x, int64_t z, int64_t radius);	

	shared_ptr<Chunk> get_neighbor(ChunkType type, int64_t side, shared_ptr<Chunk> chunk);

	static PrecipitationType get_precipitationType(int precipitation);

	void get_relative_slope_and_highest_neighbor(ChunkType type, vector<shared_ptr<Chunk>> parentNeighborChunks, double& slope_result, int64_t& highest_neighbor_result);

	shared_ptr<Position> center_pos();

};
