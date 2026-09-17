// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"

#include <algorithm>


// Original values
//int Chunk::RENDER_DISTANCES[10] = { 32, 48, 64, 96, 256, 768, 2048, 16384, 262144, 4194304 };
//int Chunk::CHUNK_SMOOTHNESS_MIN[11] = { 0, 64, 80, 80, 99, 100, 100, 100, 100, 100, 100 };
//int Chunk::CHUNK_SMOOTHNESS_MAX[11] = { 0, 80, 98, 100, 100, 100, 100, 100, 100, 100, 100 };


// currently we're only doing chunks 2097152, 131072, 8192, 512, 64, 16, 4, 1


ChunkType smaller(ChunkType type) {
	switch (type) {
		case ChunkType::CHUNK_1:
			return ChunkType::NONE;
		case ChunkType::CHUNK_4:
			return ChunkType::CHUNK_1;
		case ChunkType::CHUNK_16:
			return ChunkType::CHUNK_4;
		case ChunkType::CHUNK_64:
			return ChunkType::CHUNK_16;
		case ChunkType::CHUNK_512:
			return ChunkType::CHUNK_64;
		case ChunkType::CHUNK_8192:
			return ChunkType::CHUNK_512;
		case ChunkType::CHUNK_131072:
			return ChunkType::CHUNK_8192;
		case ChunkType::CHUNK_2097152:
			return ChunkType::CHUNK_131072;
		case ChunkType::CHUNK_16777216:
			return ChunkType::CHUNK_2097152;
		case ChunkType::CHUNK_134217728:
			return ChunkType::CHUNK_16777216;
		case ChunkType::ROOT:
			return ChunkType::CHUNK_134217728;
		case ChunkType::NONE:
			return ChunkType::ROOT;
		default:
			return ChunkType::NONE;
	}
}

ChunkType larger(ChunkType type) {
	switch (type) {
		case ChunkType::NONE:
			return ChunkType::CHUNK_1;
		case ChunkType::CHUNK_1:
			return ChunkType::CHUNK_4;
		case ChunkType::CHUNK_4:
			return ChunkType::CHUNK_16;
		case ChunkType::CHUNK_16:
			return ChunkType::CHUNK_64;
		case ChunkType::CHUNK_64:
			return ChunkType::CHUNK_512;
		case ChunkType::CHUNK_512:
			return ChunkType::CHUNK_8192;
		case ChunkType::CHUNK_8192:
			return ChunkType::CHUNK_131072;
		case ChunkType::CHUNK_131072:
			return ChunkType::CHUNK_2097152;
		case ChunkType::CHUNK_2097152:
			return ChunkType::CHUNK_16777216;
		case ChunkType::CHUNK_16777216:
			return ChunkType::CHUNK_134217728;
		case ChunkType::CHUNK_134217728:
			return ChunkType::ROOT;
		case ChunkType::ROOT:
			return ChunkType::NONE;
		default:
			return ChunkType::NONE;
	}
}

int level(ChunkType type) {
	switch (type) {
		case ChunkType::CHUNK_1:
			return 10;
		case ChunkType::CHUNK_4:
			return 9;
		case ChunkType::CHUNK_16:
			return 8;
		case ChunkType::CHUNK_64:
			return 7;
		case ChunkType::CHUNK_512:
			return 6;
		case ChunkType::CHUNK_8192:
			return 5;
		case ChunkType::CHUNK_131072:
			return 4;
		case ChunkType::CHUNK_2097152:
			return 3;
		case ChunkType::CHUNK_16777216:
			return 2;
		case ChunkType::CHUNK_134217728:
			return 1;
		case ChunkType::ROOT:
			return 0;
		case ChunkType::NONE:
			return -1;
		default:
			return -2;
	}
}


//                                   1   4  16   64   512   8192   131072  2097152
//int Chunk::RENDER_DISTANCES[8] = { 48, 96, 128, 768, 2048, 16384, 262144, 4194304 };
// int Chunk::RENDER_DISTANCES[8] = { 48, 128, 512, 1024, 4096, 16384, 262144, 4194304 };
unordered_map<ChunkType, int> Chunk::RENDER_DISTANCES = {
    { ChunkType::CHUNK_1, 128 }, // 128
    { ChunkType::CHUNK_4, 192 }, // 192
    { ChunkType::CHUNK_16, 512 }, // 512
    { ChunkType::CHUNK_64, 1024 },
    { ChunkType::CHUNK_512, 4096 },
    { ChunkType::CHUNK_8192, 16384 },
    { ChunkType::CHUNK_131072, 262144 },
    { ChunkType::CHUNK_2097152, 4194304 },
    { ChunkType::CHUNK_16777216, 8388608 },
    { ChunkType::CHUNK_134217728, 16777216 }
};
unordered_map<ChunkType, int> Chunk::SPACE_RENDER_DISTANCES = {
    { ChunkType::CHUNK_16, (int)ChunkType::CHUNK_16 * 64 },
    { ChunkType::CHUNK_64, (int)ChunkType::CHUNK_64 * 32 },
    { ChunkType::CHUNK_512, (int)ChunkType::CHUNK_512 * 32 },
    { ChunkType::CHUNK_8192, (int)ChunkType::CHUNK_8192 * 64 },
    { ChunkType::CHUNK_131072, (int)ChunkType::CHUNK_131072 * 32 },
    { ChunkType::CHUNK_2097152, (int)ChunkType::CHUNK_2097152 * 32 },
    { ChunkType::CHUNK_16777216, (int)ChunkType::CHUNK_16777216 * 8 },
    { ChunkType::CHUNK_134217728, (int)ChunkType::CHUNK_134217728 * 4 }
};
 // { 2097152, 131072, 8192, 512, 64, 16, 4, 1 };
unordered_map<ChunkType, int> Chunk::CHUNK_SIZES = {
    { ChunkType::CHUNK_1, 1 },
    { ChunkType::CHUNK_4, 4 },
    { ChunkType::CHUNK_16, 16 },
    { ChunkType::CHUNK_64, 64 },
    { ChunkType::CHUNK_512, 512 },
    { ChunkType::CHUNK_8192, 8192 },
    { ChunkType::CHUNK_131072, 131072 },
    { ChunkType::CHUNK_2097152, 2097152 },
    { ChunkType::CHUNK_16777216, 16777216 },
    { ChunkType::CHUNK_134217728, 134217728 }
};
// int Chunk::LEVEL_SUB_CHUNKS[8] = { 256, 256, 256, 64, 16, 16, 16, 1 }; // { 256, 256, 256, 64, 16, 16, 16, 1 }
unordered_map<ChunkType, int> Chunk::LEVEL_SUB_CHUNKS = {
    { ChunkType::CHUNK_1, 1 },
    { ChunkType::CHUNK_4, 16 },
    { ChunkType::CHUNK_16, 16 },
    { ChunkType::CHUNK_64, 16 },
    { ChunkType::CHUNK_512, 64 },
    { ChunkType::CHUNK_8192, 256 },
    { ChunkType::CHUNK_131072, 256 },
    { ChunkType::CHUNK_2097152, 256 },
    { ChunkType::CHUNK_16777216, 64 },
    { ChunkType::CHUNK_134217728, 64 }
};
int Chunk::MAX_SMOOTHNESS = 100000;

BMaterial Chunk::MATERIALS = BMaterial();
// total: 64x64 + 16x16x12 + 8x8x20 + 4x4x28 + 4x4x20 + 1x1x28 + ?x?x?
// up to 512 = 19,324
// horizon on earth is about 5km away.
// in real life I can see payson to lone peak, and that is about 79km. I bet you can see up to 90km
// total: 64x64 + 16x16x12 + 8x8x20 + 4x4x28 + 4x4x20 + 1x1x28 + 15*15-1 = 19,548


/*
    PUBLIC FUNCTIONS
*/

// original
//int Chunk::CHUNK_SMOOTHNESS_MIN[11] = { 0, 64, 80, 80, 99, 100, 100, 100, 100, 100, 100 };
//int Chunk::CHUNK_SMOOTHNESS_MAX[11] = { 0, 80, 98, 100, 100, 100, 100, 100, 100, 100, 100 };

//                                   2097152, 131072, 8192, 512,   64,   16,  4,   1
//                                                      512
unordered_map<ChunkType, double> Chunk::CHUNK_SMOOTHNESS_MIN = {
    { ChunkType::CHUNK_134217728, 95 },
    { ChunkType::CHUNK_16777216, 95 },
    { ChunkType::CHUNK_2097152, 95 },
    { ChunkType::CHUNK_131072, 97 },
    { ChunkType::CHUNK_8192, 98 },
    { ChunkType::CHUNK_512, 99.81 },
    { ChunkType::CHUNK_64, 99.9999 },
    { ChunkType::CHUNK_16, 100 },
    { ChunkType::CHUNK_4, 100 },
    { ChunkType::CHUNK_1, 100 }
};
unordered_map<ChunkType, double> Chunk::CHUNK_SMOOTHNESS_MAX = {
    { ChunkType::CHUNK_134217728, 99.99 },
    { ChunkType::CHUNK_16777216, 99.99 },
    { ChunkType::CHUNK_2097152, 99.99 },
    { ChunkType::CHUNK_131072, 99.99 },
    { ChunkType::CHUNK_8192, 99.99 },
    { ChunkType::CHUNK_512, 99.999 },
    { ChunkType::CHUNK_64, 100 },
    { ChunkType::CHUNK_16, 100 },
    { ChunkType::CHUNK_4, 100 },
    { ChunkType::CHUNK_1, 100 }
};
Chunk::Chunk(int parentSmoothness, ChunkType type, shared_ptr<Position> position, int64_t seed, weak_ptr<Chunk> rootChunk, int precipitation)
{
    this->rootChunk = rootChunk;
    this->position = position;
    this->precipitation = precipitation;

    // determine smoothness for the height map
    double minAddition = CHUNK_SMOOTHNESS_MIN[type] - parentSmoothness;
    double maxAddition = CHUNK_SMOOTHNESS_MAX[type] - parentSmoothness;

    double addedSmoothness = maxAddition;
    if (minAddition < maxAddition)
        addedSmoothness = Random::randDouble(seed+this->position->makeSpike(), minAddition, maxAddition);

    this->smoothness = parentSmoothness + addedSmoothness;


    if (type == ChunkType::CHUNK_1) {
        this->size = 1;
        this->type = ChunkType::CHUNK_1;
    }
    else if (type == ChunkType::CHUNK_4) {
        this->size = 4;
        this->type = ChunkType::CHUNK_4;
    }
    else if (type == ChunkType::CHUNK_16) {
        this->size = 16;
        this->type = ChunkType::CHUNK_16;
    }
    else if (type == ChunkType::CHUNK_64) {
        this->size = 64;
        this->type = ChunkType::CHUNK_64;
    }
    else if (type == ChunkType::CHUNK_512) {
        this->size = 512;
        this->type = ChunkType::CHUNK_512;
    }
    else if (type == ChunkType::CHUNK_8192) {
        this->size = 8192;
        this->type = ChunkType::CHUNK_8192;
    }
    else if (type == ChunkType::CHUNK_131072) {
        this->size = 131072;
        this->type = ChunkType::CHUNK_131072;
    }
    else if (type == ChunkType::CHUNK_2097152) {
        this->size = 2097152;
        this->type = ChunkType::CHUNK_2097152;
    }
    else if (type == ChunkType::CHUNK_16777216) {
        this->size = 16777216;
        this->type = ChunkType::CHUNK_16777216;
    }
    else if (type == ChunkType::CHUNK_134217728) {
        this->size = 134217728;
        this->type = ChunkType::CHUNK_134217728;
    }
}

double Chunk::vegetation_amount() {
    int square = this->getSquareOfSub();
    double numSubChunks = square * square;

    double precipitation_amount = clamp((double)this->precipitation / 100.0, 0.0, 1.0);
    double flow_amount = clamp(this->totalFlow / numSubChunks, 0.0, 1.0);
    return max(precipitation_amount, flow_amount);
}

void Chunk::generate_plant_ratios(const shared_ptr<Planet>& planet, int species_count, int64_t interp_precipitation) {

    if (!planet || !planet->hasLife || species_count <= 0) {
        this->ratios_plant_types.clear();
        return;
    }

    // use the parent+neighbor-smoothed precipitation so coverage doesn't jump at chunk borders
    this->precipitation = interp_precipitation;

    this->ratios_plant_types.clear();
    double vegetation = this->vegetation_amount();
    double total = 0.0;
    for (int species = 0; species < species_count; ++species) {
        double weight = Random::randDouble(
            planet->seed ^ this->position->makeSpike() ^ species,
            0.25, 1.0
        );
        // Wetter chunks support more species, while dry chunks retain a sparse tail.
        weight *= 0.1 + vegetation;
        this->ratios_plant_types[species] = weight;
        total += weight;
    }
    if (total > 0.0) {
        for (auto& [species, weight] : this->ratios_plant_types)
            weight /= total;
    }
}

int Chunk::plant_species_count(const shared_ptr<Planet>& planet) {
    if (!planet) return 0;
    int base_species = (int)(planet->worldSize * 0.0000012476);
    if (base_species < 50) base_species = 50;
    // finer chunks show a smaller subset of the regional species pool
    int species_count = std::max(8, (int)std::round(
        base_species * (double)CHUNK_SIZES[this->getType()] / (double)CHUNK_SIZES[ChunkType::CHUNK_8192]));
    return std::min(species_count, base_species);
}

void Chunk::generate_plant_placements(const shared_ptr<Planet>& planet, int species_count) {
    if (this->plant_placements_generated || !planet || !planet->hasLife ||
        this->type != ChunkType::CHUNK_512 || species_count <= 0) return;

    this->plant_placements.clear();

    double vegetation = this->vegetation_amount();
    int count = (int)round(4.0 * vegetation);
    int64_t seed = planet->seed ^ this->position->makeSpike();
    for (int i = 0; i < count; ++i) {
        int species = Random::randInt(seed + i * 3, 0, species_count - 1);
        shared_ptr<Position> position = make_shared<Position>(
            Random::randInt(seed + i * 3 + 1, this->position->x, this->position->x + this->getSize() - 1),
            0,
            Random::randInt(seed + i * 3 + 2, this->position->z, this->position->z + this->getSize() - 1)
        );
        PlantType type = Random::randBool(seed + position->makeSpike())
            ? PlantType::BABY : PlantType::FOREST;
        if (i < 2) type = PlantType::FIELD;
        this->plant_placements.push_back({species, position, type});
    }
    this->plant_placements_generated = true;
}

void Chunk::distribute_plant_placements() {
    if (this->plant_placements.empty() || this->subChunks.empty()) return;

    int square = this->getSquareOfSub();
    int child_size = CHUNK_SIZES[this->subChunks[0]->getType()];
    if (child_size <= 0) return;

    for (const PlantPlacement& placement : this->plant_placements) {
        if (!placement.position) continue;
        int cx = (int)((placement.position->x - this->position->x) / child_size);
        int cz = (int)((placement.position->z - this->position->z) / child_size);
        cx = std::clamp(cx, 0, square - 1);
        cz = std::clamp(cz, 0, square - 1);
        int index = this->convertToIndex(cx, cz, square);
        this->subChunks[index]->plant_placements.push_back(placement);
    }
}


shared_ptr<Chunk> Chunk::newSubChunk(int parentSmoothness, shared_ptr<Position> subChunkPos, int64_t seed)
{
    if (this->type == ChunkType::CHUNK_1) {
        return nullptr;
    }

    // precipitation
    int precip = this->precipitation;
    if (this->getLevel() < 6) {
        precip *= Random::randInt(seed + subChunkPos->makeSpike(), 80, 120);
        precip *= 0.01;
        precip = (precip > 100) ? 100 : precip;
    }

    // make sub chunk (root subdivides into its adaptive top tier)
    ChunkType childType = (this->subChunkType != ChunkType::NONE) ? this->subChunkType : smaller(this->getType());
    return std::make_shared<Chunk>(parentSmoothness, childType, subChunkPos, seed, this->rootChunk, precip);
}



/*
* 'parentChunks' is all the adjecent chunks to this chunk
*/
shared_ptr<Chunk> Chunk::genSubChunks(

    const shared_ptr<Position>& playerPosition,
    const shared_ptr<Planet>& planet
) {
    int level = this->getLevel();
    int y = this->position->y;


    // if this chunk isn't initialized return it's parent or ancestor
    if (!this->initialized) {
        shared_ptr<Chunk> ancestor = this->getParent();
        shared_ptr<Chunk> chunk_to_do_first = ancestor;
        while (!ancestor->subChunksInitialized) {
            chunk_to_do_first = ancestor;
            ancestor = chunk_to_do_first->getParent();
        }
        return chunk_to_do_first;
    }

    // make sure nieghbors are loaded in enough to do rivers and erosion etc..
    // all parent neighbors should be loaded in in order to do rivers and erosion
    if (this->rootChunk.lock()->topType() > this->type) {
        vector<shared_ptr<Chunk>> parentNeighborChunks = this->rootChunk.lock()->getChunkNeighbors_N_E_S_W_NE(
            larger(this->getType()), 
            this->position
        );
        for (shared_ptr<Chunk> parentNeighbor : parentNeighborChunks) {
            if(!parentNeighbor->subChunksInitialized) {
                shared_ptr<Chunk> chunk_to_do_first = parentNeighbor;
                shared_ptr<Chunk> ancestor = chunk_to_do_first->getParent();
                while (!ancestor->subChunksInitialized) {
                    chunk_to_do_first = ancestor;
                    ancestor = chunk_to_do_first->getParent();
                }
                return chunk_to_do_first;
            }
        }

    }
        
    // if not 'loaded', init sublevel chunks, and spawn all
    if (!this->subChunksInitialized) {
        //string thisString = this->toString();
        this->makeTerrain(planet);
    }

    return nullptr;
}

void Chunk::makeEmptySubChunks(int square, int numSubChunks, const shared_ptr<Planet>& planet) {

    this->subChunks.resize(numSubChunks);


    // init sub chunk positions etc..
    for (int i = 0; i < numSubChunks; i++) {
        shared_ptr<Position> subChunkPos = this->convertIndex(i);
        shared_ptr<Chunk> subChunk = this->newSubChunk(this->smoothness, subChunkPos, planet->seed);

        auto pending_height = this->pending_subchunk_heights.find(i);
        if (pending_height != this->pending_subchunk_heights.end())
            subChunk->position->y = pending_height->second;

		auto pending_contract = this->pending_subchunk_contracts.find(i);
		if (pending_contract != this->pending_subchunk_contracts.end()) {
			subChunk->inFlows = pending_contract->second.inFlows;
			if (pending_contract->second.hasOutFlow) {
				subChunk->outFlow = pending_contract->second.outFlow;
				subChunk->flowSide = pending_contract->second.flowSide;
				subChunk->totalFlow = pending_contract->second.totalFlow;
			}
		}

        // subChunk->indexInParent = i;
        this->subChunks[i] = subChunk;
    }

	this->pending_subchunk_heights.clear();
	this->pending_subchunk_contracts.clear();



}

void Chunk::setSubChunkHeight(int index, int64_t height)
{
	if (index < 0) return;
	if (!this->subChunks.empty()) {
		if (index < (int)this->subChunks.size() && this->subChunks[index])
			this->subChunks[index]->position->y = height;
		return;
	}
	this->pending_subchunk_heights[index] = height;
}

void Chunk::setPendingSubchunkInflow(int childIndex, int inflowIndex, int flowAmount)
{
	if (childIndex < 0 || inflowIndex < 0) return;
	if (!this->subChunks.empty()) {
		if (childIndex < (int)this->subChunks.size() && this->subChunks[childIndex])
			this->subChunks[childIndex]->inFlows[inflowIndex] = flowAmount;
		return;
	}
	this->pending_subchunk_contracts[childIndex].inFlows[inflowIndex] = flowAmount;
}

void Chunk::setPendingSubchunkOutflow(int childIndex, int outflowIndex, int flowSide, int flowAmount)
{
	if (childIndex < 0 || outflowIndex < 0) return;
	if (!this->subChunks.empty()) {
		if (childIndex < (int)this->subChunks.size() && this->subChunks[childIndex]) {
			this->subChunks[childIndex]->outFlow = outflowIndex;
			this->subChunks[childIndex]->flowSide = flowSide;
			this->subChunks[childIndex]->totalFlow = flowAmount;
		}
		return;
	}
	PendingSubchunkContract& contract = this->pending_subchunk_contracts[childIndex];
	contract.outFlow = outflowIndex;
	contract.flowSide = flowSide;
	contract.totalFlow = flowAmount;
	contract.hasOutFlow = true;
}

void Chunk::initRockLayers(Chunk* parentChunk, vector<shared_ptr<Chunk>> parentNeighborChunks, const shared_ptr<Planet>& planet) {

    int pos_spike = this->position->makeSpike();
    int pos_seed_spike = planet->seed + pos_spike;

    // determine rock_slope and angle (based on geologic activity)
    shared_ptr<Chunk> parent = this->rootChunk.lock()->getChunk(larger(this->getType()), this->position->x, this->position->z);
    int this_index = this->getIndexInParent(parent, planet);
    vector<int> neighbor_indices = Chunk::getNeighbors(this_index, parentChunk->subChunks.size(), parentChunk->getSquareOfSub());
    vector<shared_ptr<Chunk>> neighbors;
    for (int neighbor_index : neighbor_indices) {
        if (neighbor_index >= 0)
            neighbors.push_back(parentChunk->subChunks[neighbor_index]);
    }
    
    double height_to_neighbor_heights_ratio = 0.0;
    int cnt = 0;
    for (shared_ptr<Chunk> neighbor : neighbors) { // these are neighbors in the current parent chunk so they should be already loaded in
        // if (neighbor == nullptr) continue;
        double ratio = (double)this->position->y / (double)neighbor->position->y;
        height_to_neighbor_heights_ratio += ratio;
        ++cnt;
    }
    height_to_neighbor_heights_ratio /= cnt;

    bool range_to_change_slope = this->getLevel() > 5 && this->getLevel() < 9; // and only applied to chunk sizes 512 to 16
    if ( range_to_change_slope && height_to_neighbor_heights_ratio > 1.0) { // if this chunk is higher than it's neighbors or if there is a significant difference between it and it's neighbors
        this->rock_slope = (
            Random::randDouble(planet->seed+pos_spike, 0.0, 0.0, 5.0) 
            + parentChunk->rock_slope
        ) / 2.0;
        this->angle = (
            Random::randDouble(planet->seed+pos_spike, 0.0, 0.0, 0.0)
            // Random::randDouble(planet->seed+pos_spike, 0.0, 0.0, 3.14159)
            + parentChunk->angle
        ) / 2.0;
    }
    else {
        this->rock_slope = parentChunk->rock_slope;
        this->angle = parentChunk->angle;
    }

    // determine offset of this chunks layers according to parent layer slope
    double x_step = cos(parentChunk->angle);
    double z_step = sin(parentChunk->angle);
    double y_step = parentChunk->rock_slope;

    int parent_size = parentChunk->getSize() / 2;
    int x_start = parent_size * x_step;
    int z_start = parent_size * z_step;

    int x_end = parent_size * -x_step;
    int z_end = parent_size * -z_step;

    double end_y_offset = manhattenDistance(x_start, z_start, x_end, z_end) * y_step;
    int parent_center_of_chunk_x = parentChunk->position->x + parent_size; // parent position is the bottom left corner of the chunk
    int parent_center_of_chunk_z = parentChunk->position->z + parent_size;
    double this_dist_to_start = manhattenDistance(this->position->x, this->position->z, parent_center_of_chunk_x + x_start, parent_center_of_chunk_z + z_start);
    double this_dist_to_end = manhattenDistance(this->position->x, this->position->z, parent_center_of_chunk_x + x_end, parent_center_of_chunk_z + z_end);
    
    this->y_offset_rock_layers = end_y_offset * (this_dist_to_start / (this_dist_to_start + this_dist_to_end));

    // set material
    vector<int>& layer_heights = planet->rootChunk->rockLayerHeights;
    int layer_index = 0;
    for (int i = 0; i < layer_heights.size(); i++) {
        if (this->position->y + this->y_offset_rock_layers > layer_heights[i]) {
            layer_index = i;
            break;
        }
    }
    this->material = planet->rootChunk->rockLayerTypes[layer_index];
}

// make sure that inflow and outflow heights are all set before calling this
void Chunk::initCornerHeight(const shared_ptr<Planet>& planet)
{
    // get highest inflow
    /*int highestInflow = this->position->y;
    for (pair<int, int> inflow : this->inFlows) {
        int inflowHeight = this->subChunks[inflow.first]->position->y;
        if (inflowHeight > highestInflow)
            highestInflow = inflowHeight;
    }

    int random = Random::randInt(planet->seed+this->position->makeSpike(), highestInflow, planet->maxHeight);

    int randomWeight = 100 - this->smoothness;
    int height = (highestInflow * this->smoothness) + (random * randomWeight);
    height /= 100;*/

    //this->SWCornerHeight = height;
    //this->subChunks[0]->position->y = height;
}

// determines downblocks for chunks currently in world. Returned in descending order
vector<shared_ptr<DownBlock>> Chunk::determineDownBlocks(vector<shared_ptr<Chunk>>& loaded_in_neighbors, const shared_ptr<Planet>& planet) {

    // if no neighbors are loaded we don't need any downblocks
    if (loaded_in_neighbors.size() == 0) return this->downBlocks;

    // 108388046022

    int THIS_CHUNK_SIZE = CHUNK_SIZES[this->getType()];

    // get lowest neighbor so we know how far down the blocks should go
    int min = this->position->y; // N
    for (const shared_ptr<Chunk>& neighborChunk : loaded_in_neighbors) { // E S W since N is already checked above
        if (neighborChunk == nullptr || neighborChunk->position->y < planet->minHeight || neighborChunk->getType() > ChunkType::CHUNK_16) continue;

        int height = neighborChunk->position->y - CHUNK_SIZES[neighborChunk->getType()];
        if (height < min) {
            min = height;
        }
    }
    // check neighbors of this parent chunk to see if they have caves that would expose an opening (view underneath terrain) this subchunk needs to block
    for (const shared_ptr<Chunk>& neighborChunk : loaded_in_neighbors) {
        if (neighborChunk == nullptr) continue;
        for (const shared_ptr<DownBlock>& downBlock : neighborChunk->downBlocks) {
            if (downBlock->isCave) {
                int height = downBlock->position->y - THIS_CHUNK_SIZE;
                if (height < min) {
                    min = height;
                }
            }
        }
    }

    // make downBlocks until there's no gap
    this->addDownBlocks(THIS_CHUNK_SIZE, min, planet);
    
    return this->downBlocks;
}

void Chunk::addDownBlocks(
    int chunk_size,
    int min,
    const shared_ptr<Planet>& planet 
) {
    // clear the downblocks as we're recalculating them
    this->downBlocks.clear();

    shared_ptr<Chunk> parent = this->getParent();

    // determine rock layer
    vector<int>& layer_heights = planet->rootChunk->rockLayerHeights;
    int layer_index = 0;
    for (int i = 0; i < layer_heights.size(); i++) {
        if (this->position->y + this->y_offset_rock_layers > layer_heights[i]) {
            layer_index = i;
            break;
        }
    }
    int layer_height = layer_heights[layer_index];

    // make downBlocks until there's no gap
    int height = this->position->y;
    while (height > min) {
        while(height + this->y_offset_rock_layers < layer_height) {
            ++layer_index;
            layer_height = planet->rootChunk->rockLayerHeights[layer_index];
        }
        string material_string = planet->rootChunk->rockLayerTypes[layer_index];

        this->makeDownBlock(height, material_string, chunk_size, planet);

        height -= chunk_size;
    }
}


shared_ptr<DownBlock> Chunk::makeDownBlock(int height, string& material_string, int this_chunk_size, const shared_ptr<Planet>& planet)
{

    shared_ptr<Position> dBlockPos = make_shared<Position>(this->position->x, height, this->position->z);

    bool caveBlock = false;

    // determine if this is a cave
    int index = (this->position->y - height) / this_chunk_size;
    if (index > 1 && this->downBlocks.size() > index) {
        shared_ptr<DownBlock> previous_block = this->downBlocks[index];
        shared_ptr<DownBlock> pre_previous_block = this->downBlocks[index - 1];
        // avoid floating blocks, if cave is above previoius and previous was solid, can't be cave underneath
        if (previous_block->isCave || (!pre_previous_block->isCave && !previous_block->isCave)) {
            // XXX: eventually do caves and overhangs
            // caveBlock = Random::randInt(planet->seed+dBlockPos->makeSpike(), 0, 100) > 15;
        }
    }

    shared_ptr<DownBlock> block = make_shared<DownBlock>(material_string, dBlockPos, nullptr, caveBlock, this_chunk_size);
    this->downBlocks.push_back(block);

    return block;
}




// TERRAIN GEN

/*
* Method calculates (HEIGHT, WATER, CRATERS, CAVES, TEXTURE)
*/
int Chunk::makeTerrain(const shared_ptr<Planet>& planet) {

    if (this->totalFlow > 0) {
        // cout << this->getSize() << " flow " << this->totalFlow << endl;
    }

    int level = this->getLevel();

    /*
    * Outflow == -1
    * - parent hasn't made outflow contract, and this is child of parent outflow
    * 
    * Neighbor isn't loaded
    * 
    * 
    */
    

    int yp = this->position->y;
    int square = this->getSquareOfSub();
    int numSubChunks = square * square;

    // init subchunks and precipitation
    if (this->getType() > ChunkType::CHUNK_1 && this->subChunks.empty())
        this->makeEmptySubChunks(square, numSubChunks, planet);

    int childSquare = this->subChunks[0]->getSquareOfSub();
    int childNumSubChunks = childSquare * childSquare;

    // if a 512 has no inflows
    vector<shared_ptr<Chunk>> neighborChunks = this->rootChunk.lock()->getChunkNeighbors_N_E_S_W_NE(this->getType(), this->position);
    // XXX maybe do a random choice here to do erosion down to 64x64 chunks based on how soft the stone is (canyonlands)
    // bool could_be_a_mountain = this->inFlows.empty() && this->getType() == Chunk::EROSION_LEVEL;
    if (this->getType() >= Chunk::EROSION_LEVEL) { // 8192, so 512s are being generated with rivers

        // CONTRACTS
        // ** try to make contracts that aren't made yet **

        // outflow match
        if (this->outFlow != -1) { // all but lake chunks
            int nInFlow = this->matchFlowToFlow(this->outFlow, this->flowSide, numSubChunks, square);
            if (neighborChunks[this->flowSide]->inFlows.contains(nInFlow)) {
                this->makeWaterContractsCrossParent(this->outFlow, this->flowSide, true, this->totalFlow, neighborChunks[this->flowSide], childNumSubChunks, childSquare);
            }
        }
        // inFlow matching
        if (!this->inFlows.empty()) {
            for (int i : { 0, 1, 2, 3 }) {
                int nOutFlowSide = neighborChunks[i]->flowSide;
                int nOutFlowSideIfMatch = (i + 2) % 4;
                if (nOutFlowSide == nOutFlowSideIfMatch) {
                    int inFlowIndex = this->matchFlowToFlow(neighborChunks[i]->outFlow, nOutFlowSide, numSubChunks, square);
                    if (this->inFlows.contains(inFlowIndex)) {
                        this->makeWaterContractsCrossParent(inFlowIndex, i, false, neighborChunks[i]->totalFlow, neighborChunks[i], childNumSubChunks, childSquare);
                        this->subChunks[inFlowIndex]->position->y = neighborChunks[i]->position->y; // neighbor height is the height of it's outflow
                    }
                    else {
                        int64_t id_this = this->getId();
                        int64_t neighbor_id = neighborChunks[i]->getId();
                        cout << "RIVER MISMATCH! probably going to crash" << endl;
                        return 1;
                    }
                }
            }
        }

        // if a sea bottom, make outflow in the middle of the chunk
        if (this->outFlow == -1) {
            this->outFlow = numSubChunks * 0.6;
        }

        this->subChunks[this->outFlow]->position->y = this->position->y; // the outflow position is the height of the chunk

        // PLAN RIVERS
        unordered_map<int, int> indexToNextIndex;
        unordered_set<int> riverStarts;
        this->makeIndicePaths(indexToNextIndex, riverStarts, planet);
        
        // HEIGHTS
        // edge heights
        for (int side : {0, 1, 2, 3}) { // NESW
            int sideStart = 0;
            int sideFinish = 0;
            int step = 0;
            int64_t startHeight;
            int64_t finishHeight;

            if (side == 0) {
                sideStart = numSubChunks - square;
                sideFinish = numSubChunks - 1;
                step = 1;
            }
            else if (side == 1) {
                sideStart = numSubChunks - 1;
                sideFinish = square - 1;
                step = -square;
            }
            else if (side == 2) {
                sideStart = square;
                sideFinish = 0;
                step = -1;
            }
            else if (side == 3) {
                sideStart = 0;
                sideFinish = numSubChunks - square;
                step = square;
            }

            // make sure start and finish height are at least higher than the neighbors' outflow
            startHeight = this->getNeighborChunkInterpolation(neighborChunks, this->subChunks[sideStart]->position, planet->worldSize);
            finishHeight = this->getNeighborChunkInterpolation(neighborChunks, this->subChunks[sideFinish]->position, planet->worldSize);
            startHeight = std::max(startHeight, neighborChunks[side]->position->y);
            finishHeight = std::max(finishHeight, neighborChunks[side]->position->y);

            this->initEdgeHeights(sideStart, sideFinish, step, startHeight, finishHeight, planet);
        }

        // grade chunk between edges
        this->gradeChunk(indexToNextIndex, riverStarts, planet);


        // MAKE RIVERS
        int erosionResistance = this->getMaterialHardness(); // TODO add vegetation quantity
        this->erodeFlowPathsAndFill(indexToNextIndex, riverStarts, erosionResistance, planet);

        // MAKE LAKES
        this->makeLakes(indexToNextIndex, planet);


    }
    else {// purely interpolate with smallest chunks (4x4 and smaller)
        this->interpolate_with_cliffs(neighborChunks, planet);

        // FILL LAKES
        int lakeHeight = this->lakeLevel;
        for (int i = 0; i < numSubChunks; i++) {
            // lake level
            this->subChunks[i]->lakeLevel = lakeHeight;

            // init subchunks
            // if (this->getType() > ChunkType::CHUNK_4 && this->subChunks[i]->subChunks.size() == 0)
            //     this->subChunks[i]->makeEmptySubChunks(childSquare, childNumSubChunks, planet);
        }
    }


    // init rock layers and substrate
    int64_t this_spike = this->position->makeSpike();
    for (shared_ptr<Chunk>& subChunk : this->subChunks) {

        // rock layer
        subChunk->initRockLayers(this, neighborChunks, planet);
        subChunk->initialized = true;


        // set chunk slope and highest neighbor relative other chunks in parent
        subChunk->get_relative_slope_and_highest_neighbor(subChunk->getType(), neighborChunks, subChunk->slope, subChunk->highest_neighbor_height);

        // substrate depth
        int64_t sub_spike = subChunk->position->makeSpike();
        bool low_spot = subChunk->slope  <= 1.0;
        int min_substrate = min(1.6/Util::no_zero(subChunk->slope ), 20.0);
        int max_substrate = min(2.8/Util::no_zero(subChunk->slope ), 20.0);
        subChunk->substrate_depth = Random::randInt(planet->seed + sub_spike, min_substrate, max_substrate);

        // substrate type
        int likelyhood_to_be_like_parent = low_spot? 90: 82; // low spots have less variation
        likelyhood_to_be_like_parent += level; // more likely to be like parent if small (chunk1 = 8 vs chunk2097152 = 1)
        if (Random::randBool(planet->seed + sub_spike, likelyhood_to_be_like_parent) && this->substrate_material != "") {
            subChunk->substrate_material = this->substrate_material;
        }
        else {
            // or just get a random substrate
            subChunk->substrate_material = BMaterial::get_random_substrate(
                Chunk::get_precipitationType(this->precipitation), 
                planet->seed ^ sub_spike,
                low_spot,
                planet->hasLife
            );
        }

        // RANDOM chance to blend with neighbor type for both substrates and rock layers
        vector<float> dists_weights;
        for (int i = 0; i < neighborChunks.size()-1; ++i) {
            shared_ptr<Position> center_pos = neighborChunks[i]->center_pos();
            float dist = CoordinateConversion::cheapWrapMaxDistance(center_pos->x, center_pos->z, subChunk->position->x, subChunk->position->z, planet->worldSize);
            dist -= this->size / 2;
            dists_weights.push_back(1.0 - dist / this->size);
        }
        int pn_index = Random::weighted_choice_irregular(dists_weights, planet->seed+this_spike+sub_spike);
        float chance = dists_weights[pn_index] * 50;
        if (Random::randBool(planet->seed+this_spike+sub_spike+1, chance)) {
            subChunk->substrate_material = neighborChunks[pn_index]->substrate_material;
            subChunk->material =  neighborChunks[pn_index]->material;
            subChunk->rock_slope =  neighborChunks[pn_index]->rock_slope;
            subChunk->angle =  neighborChunks[pn_index]->angle;
            subChunk->y_offset_rock_layers =  neighborChunks[pn_index]->y_offset_rock_layers;
        }

    }

    // init plants (species tracked only down to 512x512 chunks)
    ChunkType childType = this->subChunks[0]->getType();
    if (childType >= ChunkType::CHUNK_512) {
        for (shared_ptr<Chunk>& subChunk : this->subChunks) {
            int species_count = subChunk->plant_species_count(planet);
            int64_t interp_precip = (int64_t)this->getNeighborPrecipitationInterpolation(
                neighborChunks, subChunk->position, planet->worldSize);
            subChunk->generate_plant_ratios(planet, species_count, interp_precip);
        }
    }

    // placements are made once on the 512, then partitioned down through 64 to 16
    if (this->type == ChunkType::CHUNK_512) {
        this->generate_plant_placements(planet, this->plant_species_count(planet));
    }
    if (childType == ChunkType::CHUNK_64 || childType == ChunkType::CHUNK_16) {
        this->distribute_plant_placements();
    }

    this->subChunksInitialized = true;


    // CRATER AND VOLCANOS (only on certain levels) 

    if (level < 3) return 0; // chunks that don't spawn in don't need to do the stuff below



    int SUB_CHUNK_SIZE = CHUNK_SIZES[this->subChunks[0]->getType()];

    // WATERBLOCKS 
    for (int index = 0; index < numSubChunks; index++) {
        shared_ptr<Chunk>& subChunk = this->subChunks[index];
        shared_ptr<Position> subPos = subChunk->position;


        // lake blocks
        if (subChunk->lakeLevel > subPos->y) {
            // make blocks from water level to the chunks level
            int waterHeight = subChunk->lakeLevel;
            for (int y = waterHeight; y > subPos->y; y -= SUB_CHUNK_SIZE) {
                shared_ptr<WaterBlock> waterBlock = std::make_shared<WaterBlock>();
                shared_ptr<Position> pos = Position::build(subPos->x, y, subPos->z);
                waterBlock->position = pos;
                waterBlock->size = SUB_CHUNK_SIZE;
                waterBlock->material = MATERIALS.WATER;
                subChunk->waterBlocks.push_back(waterBlock);
            }
            // make one more to fill the gap, overlapping other water block maybe
            shared_ptr<WaterBlock> waterBlock = std::make_shared<WaterBlock>();
            shared_ptr<Position> pos = Position::build(subPos->x, subPos->y + SUB_CHUNK_SIZE, subPos->z);
            waterBlock->position = pos;
            waterBlock->size = SUB_CHUNK_SIZE;
            waterBlock->material = MATERIALS.WATER;
            subChunk->waterBlocks.push_back(waterBlock);
        }
    }
    

    return 0;
}


void Chunk::makeIndicePaths(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, const shared_ptr<Planet>& planet) {

    int square = this->getSquareOfSub();
    int numSubChunks = this->subChunks.size();

    int outX = this->outFlow & (square - 1); // this '&' is a cheap % operation that only works for powers of 2
    int outZ = this->outFlow / square;


    // TODO replace with lookup table
    // get dists from edge 
    unordered_map<int, int> distsToOutflow;
    for (int index = 0; index < numSubChunks; index++) {

        int x = index & (square - 1); // this '&' is a cheap % operation that only works for powers of 2
        int z = index / square;

        distsToOutflow[index] = Chunk::manhattenDistance(x, z, outX, outZ);
    }

    int spike = this->position->makeSpike();

    // get neighbors
    unordered_map<int, vector<int>> indexToValidNeighbors;
    for (int index = 0; index < numSubChunks; index++) {
        vector<int> neighbors = this->getNeighbors(index, numSubChunks, square);
        vector<int> valids;
        for (int n : neighbors) 
            if (n >= 0) 
                valids.push_back(n);

        indexToValidNeighbors[index] = valids;
    }
    

    indexToNextIndex[this->outFlow] = -1;

    // make the default flows to outflow
    for (int index = 0; index < numSubChunks; index++) {
        if (index == this->outFlow) continue;

        int64_t sub_chunk_spike = this->subChunks[index]->position->makeSpike();
        spike += sub_chunk_spike;
        bool random = Random::randBool(planet->seed+spike);

        int nextIndex = -1;
        int bestDist = -1;
        int currentDist = distsToOutflow[index];
        bool isEdge = false;
        vector<int> neighbors = indexToValidNeighbors[index];
        for (int n : neighbors) {
            if (indexToNextIndex.contains(n) && indexToNextIndex[n] == index) continue; // no loops

            // if at outflow break
            if (n == this->outFlow) {
                nextIndex = n;
                break;
            }
            // if no next index, set it
            else if (nextIndex == -1) {
                nextIndex = n;
                bestDist = distsToOutflow[n];
            }
            // if bestDist is worse than currentDist, take anything that's better
            else if (bestDist > currentDist && distsToOutflow[n] < bestDist) {
                nextIndex = n;
                bestDist = distsToOutflow[n];
            }
            // otherwise randomly take one
            else if (distsToOutflow[n] <= bestDist && random) {
                nextIndex = n;
                bestDist = distsToOutflow[n];
            }
        }

        indexToNextIndex[index] = nextIndex;
    }


    // modify default flows 
    if (Chunk::isEdgeOfChunk(this->outFlow, numSubChunks, square)) {
        deque<int> closeAndMiddleRowOrColumns;
        int side = Chunk::sideOfEdgeIndex(this->outFlow, numSubChunks, square);
        if (side == 0) {
            for (int index = square / 2; index < numSubChunks; index += square)
                closeAndMiddleRowOrColumns.push_front(index);
        }
        else if (side == 1) {
            for (int index = numSubChunks / 2; index < numSubChunks / 2 + square; index += square)
                closeAndMiddleRowOrColumns.push_front(index);
        }
        else if (side == 2) {
            for (int index = square / 2; index < numSubChunks; index += square)
                closeAndMiddleRowOrColumns.push_back(index);
        }
        else {
            for (int index = numSubChunks / 2; index < numSubChunks / 2 + square; index += square)
                closeAndMiddleRowOrColumns.push_back(index);
        }

        for (int index : closeAndMiddleRowOrColumns) {
            int thisDist = distsToOutflow[index] + 1;

            int64_t sub_chunk_spike = this->subChunks[index]->position->makeSpike();

            bool direction = Random::randInt(planet->seed + spike + sub_chunk_spike, 0, 2) != 0;

            int step = (direction) ? 1 : -1;
            int start = (direction) ? 0 : indexToValidNeighbors[index].size() - 1;
            int end = (direction) ? indexToValidNeighbors[index].size() : -1;

            // get new neighbor
            for (int n = start; n != end; n += step) {
                int neighbor = indexToValidNeighbors[index][n];
                bool shouldReplace = index != this->outFlow && indexToNextIndex[index] != neighbor && indexToNextIndex[neighbor] != index;
                shouldReplace = shouldReplace && thisDist >= distsToOutflow[neighbor];
                if (shouldReplace) indexToNextIndex[index] = neighbor;
            }
        }
    }

    // find river starts
    set<int> notRiverStarts = { this->outFlow };
    for (int i = 0; i < numSubChunks; i++) {
        if (!notRiverStarts.contains(i)) riverStarts.insert(i);

        int nextIndex = indexToNextIndex[i];
        notRiverStarts.insert(nextIndex);
        riverStarts.erase(nextIndex);

    }
}

/**
* To be called on this chunk for S W sides, and on neighbors for N E sides
*/
void Chunk::initEdgeHeights(int sideStart, int sideFinish, int step, int heightOfNeighborClosestToStart, int heightOfNeighborClosestToFinish, const shared_ptr<Planet>& planet) {
    
    int square = this->getSquareOfSub();

    int min = std::min(heightOfNeighborClosestToFinish, heightOfNeighborClosestToStart);
    if(min < this->position->y) min = this->position->y; // make sure we're higher than outflow

    // init edges all higher than outflow 
    for (int index = sideStart; index != sideFinish; index += step) {
        if (index == this->outFlow || this->inFlows.contains(index)) continue;

        shared_ptr<Chunk> subChunk = this->subChunks[index];

        // get interpolation between start and finish
        int spike = subChunk->position->makeSpike();
        int interpolation = this->interpolate(index, sideStart, sideFinish, heightOfNeighborClosestToStart, heightOfNeighborClosestToFinish, square);
        if (interpolation < this->position->y) interpolation = this->position->y; // make sure we're higher than outflow
        int random = Random::randInt(planet->seed + spike, min, planet->maxHeight);

        int height = Chunk::weightInterpolationAndRandom(interpolation, random, this->smoothness, planet->seed+spike, false);

        subChunk->position->y = height;
    }


}


void Chunk::gradeChunk(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, const shared_ptr<Planet>& planet) {
    int numSubChunks = this->subChunks.size();
    int square = this->getSquareOfSub();
    int spike = this->position->makeSpike();

    // sort river starts (to have deterministic terrain gen)
    std::vector<int> riverStartSorted(riverStarts.begin(), riverStarts.end());
    std::sort(riverStartSorted.begin(), riverStartSorted.end());

    // instantiate river starts as higher points
    for (int index : riverStartSorted) {
        spike += index;

        if (!this->inFlows.contains(index) && this->outFlow != index && !this->isEdgeOfChunk(index, numSubChunks, square)) {
            int interpolation = this->getEdgesAndMiddleInterpolation(index, square);
            if (interpolation < this->position->y) interpolation = this->position->y; // make sure we're higher than outflow
            int random = Random::randInt(planet->seed + spike, interpolation, planet->maxHeight);

            int height = Chunk::weightInterpolationAndRandom(interpolation, random, this->smoothness, planet->seed+spike, true);

            this->subChunks[index]->position->y = height;
        }
    }

    // grade heights
    set<int> visited;
    int minHeight = this->position->y;// outlfow height
    for (int start : riverStartSorted) {
        int index = start;
        spike += index;
        visited.insert(index);

        // interpolate the rest
        int lastHeight = this->subChunks[index]->position->y;
        index = indexToNextIndex[index];
        while (index != this->outFlow) {
            int currentHeight = this->subChunks[index]->position->y;
            if (visited.contains(index) && currentHeight <= lastHeight) break; // if already set and valid, skip

            visited.insert(index);

            // if it's an inflow we'll just pretend it goes up
            if (!this->inFlows.contains(index)) {

                // otherwise set, or override with lower height
                int interpolation = this->getEdgesAndMiddleInterpolation(index, square);
                int min = (interpolation < lastHeight) ? interpolation : lastHeight - 1;
                // int min = (interpolation < lastHeight) ? interpolation : lastHeight - max( (lastHeight - minHeight) / 1.3, 1.0 );
                // int height = (min < lastHeight)? Random::randInt(planet->seed+spike, min, lastHeight) : lastHeight;

                this->subChunks[index]->position->y = max(min, minHeight);
            }

            // step
            lastHeight = this->subChunks[index]->position->y;
            index = indexToNextIndex[index];
        }
    }

}

// used only for the smallest chunks
void Chunk::interpolate_with_cliffs(vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Planet>& planet)
{
        /*
            Angles
            + dry sand 20-30
            + rocks 35-40
            + soil 45-60
            + fine dust low gravity 50-70


            maybe just add 10-gravity to these angles
        */

        int numSubChunks = this->subChunks.size();
        int square = this->getSquareOfSub();
        for (int index = 0; index < numSubChunks; index++) {
            int spike = this->subChunks[index]->position->makeSpike();

            // get biggest y diff of neighbors
            vector<shared_ptr<Position>> positions;
            positions.push_back(this->position);
            positions.push_back(neighborChunks[0]->position);
            positions.push_back(neighborChunks[4]->position);
            positions.push_back(neighborChunks[1]->position);
            int y_min = positions[0]->y;
            int y_max = positions[0]->y;
            for (shared_ptr<Position> n_pos: positions) {
                if (n_pos->y < y_min) y_min = n_pos->y;
                if (n_pos->y > y_max) y_max = n_pos->y;
            }
            double slope = y_max - y_min;

            int rand_min_or_max = Random::randBool(planet->seed+spike)? y_max : y_min;


            // weight average with slope
            float avg = 0;
            float avg_div = 0;
            for (shared_ptr<Position> n_pos: positions) {

                // determine weight
                double dist = CoordinateConversion::cheapWrapMaxDistance(
                    this->subChunks[index]->position->x, 
                    this->subChunks[index]->position->z, 
                    n_pos->x, 
                    n_pos->z, 
                    planet->worldSize
                ) + 1;
                
                // if is minimum neighbor, give higher weight based on slope and world gravity
                if (n_pos->y == rand_min_or_max) {
                    slope = min(slope, 10.0) / 10.0; // make it a value 0-1
                    double mult = 1 - Random::randDouble(planet->seed+spike, 0.0, 0.5) * slope * planet->gravity / 10.0;
                    dist *= mult;
                }

                avg += n_pos->y / (dist * dist);
                avg_div += 1 / (dist * dist);
            }
            int interpolation = std::round(avg / avg_div);


            // add randomness and set height
            int random = (interpolation < planet->maxHeight)? Random::randInt(planet->seed + spike, interpolation, planet->maxHeight) : planet->maxHeight;
            int height = Chunk::weightInterpolationAndRandom(interpolation, random, this->smoothness, planet->seed+spike, true);
            this->subChunks[index]->position->y = height;

        }
    
}


void Chunk::erodeFlowPathsAndFill(unordered_map<int, int>& indexToNextIndex, unordered_set<int>& riverStarts, int erosionResistance, const shared_ptr<Planet>& planet) {
    
    // amazon river has a flow rate of 224k m3/s, missippi 21k, nile 2k


    // XXX: use material hardness (erosionResistance) here

    int level = this->getLevel();
    int childSquare = this->subChunks[0]->getSquareOfSub();
    int childNumSubChunks = childSquare * childSquare;
    int square = this->getSquareOfSub();

    
    // determine flow amounts, make contracts
    for (int index : riverStarts) {
        int start = index;


        int flowAmount = Chunk::calculate_flow_amount(this->getType(), this->precipitation);
        if (this->inFlows.contains(index)) {
            flowAmount = this->inFlows[index] + flowAmount; // more for each level
            //flowAmount = this->inFlows[index] + level * this->precipitation * 0.01; // apply parent river level
        }

        int dist_from_start = 0;
        while (index != this->outFlow) {

            // set flow amount
            this->subChunks[index]->totalFlow = std::max(flowAmount, this->subChunks[index]->totalFlow);

            int nextIndex = indexToNextIndex[index];

            // make contracts
            if (level < 7) // 2x2 chunks don't need to make contracts between 1x1 chunks
                this->makeWaterContracts(index, nextIndex, this->subChunks[index]->totalFlow, square, childNumSubChunks, childSquare, planet);


            // step
            index = nextIndex;
            
            flowAmount += Chunk::calculate_added_flow_amount(dist_from_start, this->getSquareOfSub(), this->precipitation);
            ++dist_from_start;
        }
    }
}


void Chunk::makeLakes(unordered_map<int, int>& indexToNextIndex, const shared_ptr<Planet>& planet) {

    int square = this->getSquareOfSub();
    int numSubChunks = square * square;
    int y = this->position->y;

    // parent lake level
    // TODO we might have to manually dig out some blocks to have the parent lake level applied
    if (this->lakeLevel != 0) {
        for (int i = 0; i < this->subChunks.size(); i++) {
            this->subChunks[i]->lakeLevel = lakeLevel;
        }

        return;
    }

    if (this->getLevel() <= 8) // after 16x16 chunks we aren't going to make more lakes
        return;

    // otherwise make lakes
    set<int> lakeIndices;
    int numAttempts = this->lakiness * numSubChunks;
    int lastLakeStart = numAttempts;
    double materialHardness = this->getMaterialHardness(); // set this with rock hardness for this chunk 0.0-1.0
    for (int i = 0; i < numAttempts; i++) {

        int index = Random::cheapRandom(lastLakeStart, square, numSubChunks - square - 2);

        if (lakeIndices.contains(index)) continue;

        lastLakeStart = index;

        set<int> lake;

        int newLakeLevel = this->subChunks[index]->position->y;

        vector<int> exploreQueue;
        vector<int> neighbors = this->getNeighbors(index, numSubChunks, square);
        for (int neighbor : neighbors) {
            if (indexToNextIndex[neighbor] == index && this->subChunks[neighbor]->position->y >= newLakeLevel && !this->isEdgeOfChunk(neighbor, numSubChunks, square)) {
                exploreQueue.push_back(neighbor);
            }
        }

        while (lake.size() < square && !exploreQueue.empty()) {

            index = exploreQueue.back();
            exploreQueue.pop_back();

            // collect neighbors and determine if valid
            neighbors = this->getNeighbors(index, numSubChunks, square);
            bool valid = true;
            vector<int> validNeighbors;
            for (int neighbor : neighbors) {
                if (indexToNextIndex[neighbor] == index && this->subChunks[neighbor]->position->y >= newLakeLevel && !this->isEdgeOfChunk(neighbor, numSubChunks, square)) {
                    validNeighbors.push_back(neighbor);
                }
                else if (this->subChunks[neighbor]->position->y < newLakeLevel || this->subChunks[neighbor]->lakeLevel != 0) {
                    valid = false;
                }
            }

            if (valid) {
                lake.insert(index);
                for (int neighbor : validNeighbors)
                    exploreQueue.push_back(neighbor);
            }


        }


        int maxDepth = this->subChunks[0]->position->y - this->subChunks[this->outFlow]->position->y; // basically random
        int lakeMin = this->subChunks[lastLakeStart]->position->y - maxDepth;
        for (int indicy : lake) {
            this->subChunks[indicy]->position->y = (lakeMin < newLakeLevel)? Random::randInt(planet->seed + indicy, lakeMin, newLakeLevel) : lakeMin;
            this->subChunks[indicy]->lakeLevel = newLakeLevel;
        }

    }

}




// TERRAIN HELPERS


// (nearestToStart, nearestToFinish)
pair<int, int> Chunk::getNearestFlowsToStartAndFinishOnSide(int sideStart, int sideFinish, int square) {
    if (this->lakeLevel > 0)
        return pair<int, int>(-1, -1);

    int nearestToStart = this->outFlow;
    int nearestToStartDist = this->cheapMaxDistance(sideStart, this->outFlow, square);

    int nearestToFinish = this->outFlow;
    int nearestToFinishDist = this->cheapMaxDistance(sideFinish, this->outFlow, square);

    for (pair<int, int> inflow : this->inFlows) {
        int startDist = this->cheapMaxDistance(sideStart, inflow.first, square);
        if (startDist < nearestToStartDist) {
            nearestToStartDist = startDist;
            nearestToStart = inflow.first;
        }
        int finishDist = this->cheapMaxDistance(sideFinish, inflow.first, square);
        if (finishDist < nearestToFinishDist) {
            nearestToFinishDist = finishDist;
            nearestToFinish = inflow.first;
        }
    }

    if (nearestToStartDist > this->cheapMaxDistance(sideStart, 0, square))
        nearestToStart = 0;
    if (nearestToFinishDist > this->cheapMaxDistance(sideFinish, 0, square))
        nearestToFinish = 0;

    return pair<int, int>(nearestToStart, nearestToFinish);
}


int Chunk::interpolate(int index, int first, int second, int firstHeight, int secondHeight, int square) {

    float firstDist = Chunk::cheapMaxDistance(index, first, square) + 1; // +1 to not divide by zero
    float secondDist = Chunk::cheapMaxDistance(index, second, square) + 1; // +1 to not divide by zero

    firstDist *= firstDist;
    secondDist *= secondDist;

    float avg = firstHeight / firstDist + secondHeight / secondDist;
    avg /= (1 / firstDist + 1 / secondDist);

    return std::round(avg);
}


/*
* interpolates between indices and the index to determine the interpolation height for the index.
* Dists are a map that provides distances if previously calculated. Can be empty and the distances will
* be calculated in the method.
*/
int Chunk::interpolateWithIndices(int index, vector<int> indicesInterpBetween, unordered_map<int, int> dists, int square) {
    float avg = 0;
    float div = 0;
    for (int i : indicesInterpBetween) {
        float dist = (dists.contains(i)) ? dists[i] : Chunk::cheapMaxDistance(index, i, square);
        dist++; // +1 to not divide by zero
        dist *= dist;

        avg += this->subChunks[i]->position->y / dist;
        div += 1 / dist;
    }

    return std::round(avg / div);
}



int Chunk::getNeighborChunkInterpolation(const vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Position>& subChunkPos, int worldSize) {

    // determine distance to nieghboring parent chunks 
    // float parentDist = CoordinateConversion::cheapWrapMaxDistance(subChunkPos->x, subChunkPos->z, this->position->x, this->position->z, worldSize) + 1; // +1 to not divide by zero
    // float northDist = CoordinateConversion::cheapWrapMaxDistance(subChunkPos->x, subChunkPos->z, neighborChunks[0]->position->x, neighborChunks[0]->position->z, worldSize) + 1;
    // float northEastDist = CoordinateConversion::cheapWrapMaxDistance(subChunkPos->x, subChunkPos->z, neighborChunks[4]->position->x, neighborChunks[4]->position->z, worldSize) + 1;
    // float eastDist = CoordinateConversion::cheapWrapMaxDistance(subChunkPos->x, subChunkPos->z, neighborChunks[1]->position->x, neighborChunks[1]->position->z, worldSize) + 1;
    // parentDist *= parentDist;
    // northDist *= northDist;
    // northEastDist *= northEastDist;
    // eastDist *= eastDist;

    // float parentHeight = this->position->y;
    // float northHeight = neighborChunks[0]->position->y;
    // float northEastHeight = neighborChunks[4]->position->y;
    // float eastHeight = neighborChunks[1]->position->y;

    // float avg = parentHeight / parentDist + northHeight / northDist + northEastHeight / northEastDist + eastHeight / eastDist;
    // avg /= (1 / parentDist + 1 / northDist + 1 / northEastDist + 1 / eastDist);

    // return std::round(avg);


    vector<shared_ptr<Position>> positions;
    positions.push_back(this->position);
    positions.push_back(neighborChunks[0]->position);
    positions.push_back(neighborChunks[4]->position);
    positions.push_back(neighborChunks[1]->position);

    return Chunk::interpolate(positions, subChunkPos, worldSize);
}


int Chunk::interpolate(vector<shared_ptr<Position>> positions, const shared_ptr<Position>& target_position, int worldSize) {

    float avg = 0;
    float avg_div = 0;
    for (shared_ptr<Position> position: positions) {
        float dist = CoordinateConversion::cheapWrapMaxDistance(target_position->x, target_position->z, position->x, position->z, worldSize) + 1; // +1 to not divide by zero
        float height = position->y;
        avg += height / (dist * dist);
        avg_div += 1 / (dist * dist);
    } 

    return std::round(avg / avg_div);
}


double Chunk::getNeighborPrecipitationInterpolation(const vector<shared_ptr<Chunk>>& neighborChunks, const shared_ptr<Position>& subChunkPos, int worldSize) {

    // same inverse-square weighting as height: parent + N + NE + E neighbors
    double avg = 0.0, avg_div = 0.0;
    double parent_precip = (double)this->precipitation;
    double parent_dist = CoordinateConversion::cheapWrapMaxDistance(
        subChunkPos->x, subChunkPos->z, this->position->x, this->position->z, worldSize) + 1;
    avg += parent_precip / (parent_dist * parent_dist);
    avg_div += 1.0 / (parent_dist * parent_dist);

    for (int i : {0, 4, 1}) { // N, NE, E
        const shared_ptr<Chunk>& neighbor = neighborChunks[i];
        if (!neighbor) continue;
        double dist = CoordinateConversion::cheapWrapMaxDistance(
            subChunkPos->x, subChunkPos->z, neighbor->position->x, neighbor->position->z, worldSize) + 1;
        double value = (double)neighbor->precipitation;
        avg += value / (dist * dist);
        avg_div += 1.0 / (dist * dist);
    }

    return avg / avg_div;
}


int Chunk::getEdgesAndMiddleInterpolation(int index, int square) {
    int numSubChunks = this->subChunks.size();
    int halfOfSquare = square / 2;


    // get nearest edge
    int x = index & (square - 1); // eq. index % square
    int z = index / square;

    int nearestSide = -1;
    int edgeDist = std::numeric_limits<int>().max();
    vector<int> dists = { square - z, square - x, z, x }; // dists N E S W
    for (int i = 0; i < dists.size(); i++) {
        if (dists[i] < edgeDist) {
            edgeDist = dists[i];
            if (i == 0) // N
                nearestSide = numSubChunks - square + x;
            else if (i == 1) // E
                nearestSide = z * square + square - 1;
            else if (i == 2) // S
                nearestSide = x;
            else // W
                nearestSide = z * square;
        }
    }




    // get nearest inflow
    int nearestInflow = -1;
    int inflowDist = std::numeric_limits<int>().max();
    for (pair<int, int> inFlow : this->inFlows) {
        int dist = Chunk::cheapMaxDistance(index, inFlow.first, square);
        if (dist < inflowDist) {
            nearestInflow = inFlow.first;
            inflowDist = dist;
        }
    }


    // get dist to ouflow
    int outFlowDist = Chunk::cheapMaxDistance(index, this->outFlow, square);


    // interp between edge, inflow, and outflow
    if (this->inFlows.size() > 0) {
        return this->interpolateWithIndices(index, { nearestSide, nearestInflow, outFlow }, { {nearestSide, edgeDist}, {nearestInflow, inflowDist}, {outFlow, outFlowDist} }, square);
    }
    else {
        return this->interpolateWithIndices(index, { nearestSide, outFlow }, { {nearestSide, edgeDist}, {outFlow, outFlowDist} }, square);
    }
}


void Chunk::makeWaterContracts(int index, int nextIndex, int flowAmount, int square, int childNumSubChunks, int childSquare, const shared_ptr<Planet>& planet) {

    shared_ptr<Chunk> currentSubChunk = this->subChunks[index];

    int side = this->flowDir(index, nextIndex, square);
    int subOutFlow = this->getSubOutFlow(index, side, childNumSubChunks, childSquare);

    int nextChunkInflowIndex = this->matchFlowToFlow(subOutFlow, side, childNumSubChunks, childSquare);


    this->matchBlockers(subOutFlow, currentSubChunk, this->subChunks[nextIndex], nextChunkInflowIndex);

    // make outflows and inflows
    this->subChunks[nextIndex]->inFlows[nextChunkInflowIndex] = flowAmount;
    currentSubChunk->outFlow = subOutFlow;
    currentSubChunk->flowSide = side;
    currentSubChunk->totalFlow = flowAmount;

    // Defer endpoint heights until child chunks are actually needed.
    this->subChunks[nextIndex]->setSubChunkHeight(nextChunkInflowIndex, currentSubChunk->position->y);
    currentSubChunk->setSubChunkHeight(subOutFlow, currentSubChunk->position->y);
}


/**
 * Makes water contracts between parents. Basically just sets the flow amount in the inflow and outflow subchunks of the matching parents. 
 * The height isn't set here, but for the outflow it is set to the parent's height. The inflow is just calculated to something less than the neighbor's ouflow.
 */
void Chunk::makeWaterContractsCrossParent(int index, int side, bool isOutFlow, int flowAmount, shared_ptr<Chunk>& parentNeighbor, int childNumSubChunks, int childSquare) {

    if (this->subChunks[index]->outFlow != -1) return; //if already made just return

    int thisChunkSubIndex = this->getSubOutFlow(index, side, childNumSubChunks, childSquare);
    int otherChunkIndex = this->matchFlowToFlow(index, side, this->subChunks.size(), this->getSquareOfSub());
    int otherChunkSubIndex = this->matchFlowToFlow(thisChunkSubIndex, side, childNumSubChunks, childSquare);

    if (isOutFlow) {
        // make outflows and inflows
        this->subChunks[index]->outFlow = thisChunkSubIndex;
        this->subChunks[index]->flowSide = side;
        this->subChunks[index]->totalFlow = flowAmount;
		parentNeighbor->setPendingSubchunkInflow(otherChunkIndex, otherChunkSubIndex, flowAmount);

		if (!this->externalFlowBlockers.contains(thisChunkSubIndex)) {
			auto blocker = make_shared<WaterContract>();
			blocker->index = thisChunkSubIndex;
			blocker->isInFlow = false;
			this->externalFlowBlockers[thisChunkSubIndex] = blocker;
		}
		if (!parentNeighbor->externalFlowBlockers.contains(otherChunkSubIndex)) {
			auto blocker = make_shared<WaterContract>();
			blocker->index = otherChunkSubIndex;
			blocker->isInFlow = true;
			parentNeighbor->externalFlowBlockers[otherChunkSubIndex] = blocker;
		}
    }
    else {
        // make outflows and inflows
        this->subChunks[index]->inFlows[thisChunkSubIndex] = flowAmount;
		parentNeighbor->setPendingSubchunkOutflow(otherChunkIndex, otherChunkSubIndex, (side + 2) % 4, flowAmount);

		if (!parentNeighbor->externalFlowBlockers.contains(otherChunkSubIndex)) {
			auto blocker = make_shared<WaterContract>();
			blocker->index = otherChunkSubIndex;
			blocker->isInFlow = false;
			parentNeighbor->externalFlowBlockers[otherChunkSubIndex] = blocker;
		}
		if (!this->externalFlowBlockers.contains(thisChunkSubIndex)) {
			auto blocker = make_shared<WaterContract>();
			blocker->index = thisChunkSubIndex;
			blocker->isInFlow = true;
			this->externalFlowBlockers[thisChunkSubIndex] = blocker;
		}
    }
    
}


void Chunk::matchBlockers(int subOutFlow, shared_ptr<Chunk>& currentChunk, shared_ptr<Chunk>& nextChunk, int subNeighborInflow) {

    if (this->externalFlowBlockers.contains(subOutFlow)) return; // if already made just return

    // make blocker for currentSubChunk sub outflow index
    shared_ptr<WaterContract> blocker = std::make_shared<WaterContract>();
    blocker->index = subOutFlow;
    blocker->isInFlow = false;
    currentChunk->externalFlowBlockers.emplace(
        subOutFlow,
        blocker
    );

    // make blocker for nextSubChunk sub inflow index
    shared_ptr<WaterContract> nextBlocker = std::make_shared<WaterContract>();
    nextBlocker->index = subNeighborInflow;
    nextBlocker->isInFlow = true;
    nextChunk->externalFlowBlockers.emplace(
        subNeighborInflow,
        nextBlocker
    );
}

/**
* Returns inflow direction 0123 for NESW
*/
int Chunk::flowDir(int index, int nextIndex, int square) {

    if (nextIndex == index + square) { // N
        return 0;
    }
    if (nextIndex == index + 1) { // E
        return 1;
    }
    if (nextIndex == index - square) { // S
        return 2;
    }
    if (nextIndex == index - 1) { // W
        return 3;
    }


    //wrap for root chunk
    if (nextIndex == index % square) { // N
        return 0;
    }
    if (nextIndex == index + 1 - square) { // E
        return 1;
    }
    if (nextIndex % square == index) { // S
        return 2;
    }
    if (nextIndex == index + square - 1) { // W
        return 3;
    }

    return -1;
}

/*
* Matches edge index to index in other chunk.
*
* Will break on corner indices probably.
*/
int Chunk::matchFlowToFlow(int flowIndex, int side, int numSubchunks, int square) {

    // otherwise
    if (side == 2) { // south
        return flowIndex + numSubchunks - square;
    }
    else if (side == 0) { // North
        return flowIndex - numSubchunks + square;
    }
    else if (side == 3) { // West
        return flowIndex + square - 1;
    }
    else if (side == 1) { // East
        return flowIndex - square + 1;
    }
    return -1;
}


// TODO replace this with a look up table maybe
int Chunk::getSubOutFlow(int index, int side, int childNumSubChunks, int childSquare) {
    // 4 2
    int subOutFlow = -1;
    if (side == 0) { // North
        subOutFlow = childNumSubChunks - childSquare / 2;
    }
    else if (side == 1) { // East
        subOutFlow = childNumSubChunks / 2 + childSquare - 1;
    }
    else if (side == 2) { // South
        subOutFlow = childSquare / 2;
    }
    else if (side == 3) { // West
        subOutFlow = childNumSubChunks / 2;
    }

    return subOutFlow;
}

/**
* WARNING this function doesn't work when numSubChunks is 2x2
* 
* Returns inflow direction 0123 for NESW
*/
int Chunk::sideOfEdgeIndex(int index, int numSubChunks, int square) {

    // otherwise
    if (index < square) { // south
        return 2;
    }
    else if (index >= numSubChunks - square) { // North
        return 0;
    }
    else if (index % square == 0) { // West
        return 3;
    }
    else if ((index + 1) % square == 0) { // East
        return 1;
    }
    return -1;
}


/**
 * Get's the side of block the other block is positioned. Takes into account wrapping.
 * 
 * Expects
 * - both blocks are the same size
 * - the blocks are next to each other and share a side.
 * 
 * Returns flowside direction 0123 for NESW
 */
int Chunk::getFlowSide(int64_t x, int64_t z, int64_t other_x, int64_t other_z, int64_t size, int64_t world_size)
{

    int64_t x_diff = x - other_x;
    int64_t z_diff = z - other_z;
    

    // handle wrapping
    if (x > other_x && x + size > world_size) { // x is world_size-size and other_x is 0
        x -= world_size;
        x_diff = x - other_x;
    }
    else if (x < other_x && other_x + size > world_size) { // other_x is world_size-size and x is 0
        other_x -= world_size;
        x_diff = x - other_x;
    }

    if (z > other_z && z + size > world_size) { // z is world_size-size and other_z is 0
        z -= world_size;
        z_diff = z - other_z;
    }
    else if (z < other_z && other_z + size > world_size) { // other_z is world_size-size and z is 0
        other_z -= world_size;
        z_diff = z - other_z;
    }


    if (z_diff < 0) { // N
        return 0;
    }
    else if (x_diff < 0) { // E
        return 1;
    }
    else if (z_diff > 0) { // S
        return 2;
    }
    else if (x_diff > 0) { // W
        return 3;
    }

    return -1;
}

int Chunk::weightInterpolationAndRandom(int interpolation, int random, double smoothness, int64_t seed, bool allow_anomalies)
{
    // double smoothness_variability = (100 - smoothness) * 60;
    // double randomized_smoothness = smoothness == 100? smoothness : Random::randDouble(seed+spike, smoothness - smoothness_variability, smoothness, 100, 1.7);
    // // randomized_smoothness = (allow_anomalies) ? randomized_smoothness : smoothness;
    // randomized_smoothness = smoothness;
    
    double randomWeight = 100 - smoothness;
    int height = (interpolation * smoothness) + (random * randomWeight);
    height /= 100;

    return height;
}

int Chunk::calculate_flow_amount(ChunkType type, int precipitation) {
    int chunk_flow_amount = pow((int) type, 0.3) * precipitation / 100.0;
    return chunk_flow_amount;
}

int Chunk::calculate_added_flow_amount(int dist_from_start, int square, int precipitation) {
    return (precipitation / 100.0) * dist_from_start  / square;
}


// GENERAL USE HELPERS

/**
* Returns vector of (neighborIndex, height)
*
* at indices 0 1 2 3 for N E S W
*
* This function replace N and E heights with NE height when applicable
*
* for neighbor indices out of bonds it returns -1, -2, -3, -4 for NESW parent neighbors
*/
vector<pair<int, int>> Chunk::getNeighborHeightsInChunk(int index, int numSubChunks, int square) {

    vector<pair<int, int>> heights;
    vector<int> neighbors = this->getNeighborsWithNE(index, numSubChunks, square);

    const int MIN_INT = std::numeric_limits<int>().min();

    int north = neighbors[0];
    int northEast = neighbors[1];
    int east = neighbors[2];
    int northEastHeight = (northEast > 0) ? this->subChunks[northEast]->position->y : MIN_INT;

    for (int neighbor : neighbors) {
        if (neighbor == northEast) continue; // Skip NE because we can't directly flow to it

        if (neighbor < 0) { // out of bounds neighbors (edge of chunk) and skip neighbors already visited.  
            neighbor += (neighbor < -1) ? 1 : 0; // adjust for taking out NE
            heights.push_back(pair<int, int>(neighbor, 0));
            continue;
        }

        int height = this->subChunks[neighbor]->position->y;

        // Because of chunk interpolation, if NE is lower than N or E than it should replace their current height
        if (north == neighbor && northEastHeight > MIN_INT && height > northEastHeight)
            height = northEastHeight;
        if (east == neighbor && northEastHeight > MIN_INT && height > northEastHeight)
            height = northEastHeight;


        heights.push_back(pair<int, int>(neighbor, height));
    }

    return heights;
}

/**
* Returns indices for  N E S W:   0 1 2 3
*
* returns neighbor indices or -1, -2, -3, -4 for NESW parent neighbors if the chunk isn't in the same parent
*/
vector<int> Chunk::getNeighbors(int currentIndex, int numSubChunks, int square) {

    int north = currentIndex + square;
    int east = currentIndex + 1;
    int south = currentIndex - square;
    int west = currentIndex - 1;

    // -1, -2, -3, -4 for NESW parent neighbors
    vector<int> neighbors;
    if (north >= numSubChunks)
        neighbors.push_back(-1);
    else
        neighbors.push_back(north);

    if (east % square == 0)
        neighbors.push_back(-2);
    else
        neighbors.push_back(east);

    if (south < 0)
        neighbors.push_back(-3);
    else
        neighbors.push_back(south);

    if ((west + 1) % square == 0)
        neighbors.push_back(-4);
    else
        neighbors.push_back(west);

    return neighbors;
}

/**
* Returns indices for  N NE E SE S SW W NW:   0 1 2 3 4 5 6 7 8
*
* returns neighbor indices or -1, -2, -3, -4, -5, -6, -7, -8  N NE E SE S SW W NW parent neighbors
*/
vector<int> Chunk::getAll9Neighbors(int currentIndex, int numSubChunks, int square) {

    int north = currentIndex + square;
    int northEast = north + 1;
    int east = currentIndex + 1;
    int southEast = east - square;
    int south = currentIndex - square;
    int southWest = south - 1;
    int west = currentIndex - 1;
    int northWest = west + square;

    // -1, -2, -3, -4 for NESW parent neighbors
    vector<int> neighbors;
    if (north >= numSubChunks)
        neighbors.push_back(-1);
    else
        neighbors.push_back(north);

    if (northEast >= numSubChunks)
        neighbors.push_back(-2);
    else
        neighbors.push_back(northEast);

    if (east % square == 0)
        neighbors.push_back(-3);
    else
        neighbors.push_back(east);

    if (southEast < 0)
        neighbors.push_back(-4);
    else
        neighbors.push_back(southEast);

    if (south < 0)
        neighbors.push_back(-5);
    else
        neighbors.push_back(south);

    if (southWest < 0)
        neighbors.push_back(-6);
    else
        neighbors.push_back(southWest);

    if ((west + 1) % square == 0)
        neighbors.push_back(-7);
    else
        neighbors.push_back(west);

    if (northWest >= numSubChunks)
        neighbors.push_back(-8);
    else
        neighbors.push_back(northWest);

    return neighbors;
}

/**
* Returns indices for  N NE E S W:   0 1 2 3 4
*
* returns neighbor indices or -1, -2, -3, -4, -5 for NNEESW parent neighbors
*/
vector<int> Chunk::getNeighborsWithNE(int currentIndex, int numSubChunks, int square) {

    int north = currentIndex + square;
    int northEast = currentIndex + square + 1;
    int east = currentIndex + 1;
    int south = currentIndex - square;
    int west = currentIndex - 1;

    // -1, -2, -3, -4 for NESW parent neighbors
    vector<int> neighbors;
    if (north >= numSubChunks)
        neighbors.push_back(-1);
    else
        neighbors.push_back(north);

    if (northEast >= numSubChunks || northEast % square == 0)
        neighbors.push_back(-2);
    else
        neighbors.push_back(northEast);

    if (east % square == 0)
        neighbors.push_back(-3);
    else
        neighbors.push_back(east);

    if (south < 0)
        neighbors.push_back(-4);
    else
        neighbors.push_back(south);

    if ((west + 1) % square == 0)
        neighbors.push_back(-5);
    else
        neighbors.push_back(west);

    return neighbors;
}


/**
 * Returns the hardness of the rock from 0-10, 10 being the hardest
 */
int Chunk::getMaterialHardness()
{
    return MATERIALS.getHardness(this->material);
}


bool Chunk::isEdgeOfChunk(int candidate, int numSubChunks, int square) {
    if (candidate < square) { // south
        return true;
    }
    else if (candidate >= numSubChunks - square) { // North
        return true;
    }
    else if (candidate % square == 0) { // West
        return true;
    }
    else if ((candidate + 1) % square == 0) { // East
        return true;
    }

    return false;
}


/*
* Takes the max of the x and z dists.
*
* ( 2 sub, 2 abs )
*/
int Chunk::cheapMaxDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ)
{
    int xdist = abs(x - otherX);
    int zdist = abs(z - otherZ);
    return std::max({ xdist, zdist });
}


/*
* Takes the max of the x and z dists. 
* 
* ( 6 sub, 4 and, 2 abs )
* 
* Only works for squares that are powers of 2
*/
int Chunk::cheapMaxDistance(int index, int otherIndex, int square)
{
    
    int x = index & (square - 1); // this '&' is a cheap % operation that only works for powers of 2
    int z = index / square;
    int otherX = otherIndex & (square - 1);
    int otherZ = otherIndex / square;


    int xdist = abs(x - otherX);
    int zdist = abs(z - otherZ);
    return std::max({ xdist, zdist });
}


shared_ptr<Position> Chunk::convertIndex(int index)
{
    int size = Chunk::CHUNK_SIZES[
        (this->subChunkType != ChunkType::NONE) ? this->subChunkType : smaller(this->type)
    ];

    int o_x, o_z;
    convertIndex(index, this->getSquareOfSub(), o_x, o_z);
    int x = this->position->x + o_x * size;
    int z = this->position->z + o_z * size;

    return std::make_shared<Position>(x, std::numeric_limits<int>().min(), z);
}


void Chunk::convertIndex(int index, int arraySqrtRoot, int& x, int& z) {
    z = index / arraySqrtRoot;
    x = index - z * arraySqrtRoot;
}


int Chunk::convertToIndex(int64_t x, int64_t z, int arraySqrtRoot) {
    return z * arraySqrtRoot + x;
}


int Chunk::manhattenDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ) {

    int64_t xdist = abs(x - otherX);
    int64_t zdist = abs(z - otherZ);

    return xdist + zdist;
}

int Chunk::wrapManhattenDistance(int64_t x, int64_t z, int64_t otherX, int64_t otherZ, int64_t wrapValue)
{
	double xdist = std::min({ abs(x - otherX), abs(x - wrapValue - otherX), abs(x + wrapValue - otherX) });
	double zdist = std::min({ abs(z - otherZ), abs(z - wrapValue - otherZ), abs(z + wrapValue - otherZ) });
	return xdist + zdist;
}

int64_t Chunk::getId() {
    string str = std::to_string(Chunk::CHUNK_SIZES[this->getType()]).substr(0, 3) + "0" + std::to_string(this->position->x) + "0" + std::to_string(this->position->z);
    str = str.substr(0, 18);
    return std::stoll(str);
}

shared_ptr<Chunk> Chunk::getParent() {
    return this->rootChunk.lock()->getChunk(larger(this->getType()), this->position->x, this->position->z);
}


// DEBUG

string Chunk::toString(string file_path)
{
    /*

                ------------------
               |(1)     /\        |
               |        9b        |
               | < 5Th  La  9Th > |
               |        4T        |
               |12,900 w\/        |
                ------------------

    */


    int square = this->getSquareOfSub();
    int numSubChunks = square * square;
    int childSquare = this->subChunks[0]->getSquareOfSub();
    int childNumSubChunks = childSquare * childSquare;

    vector<pair< string, unordered_map<int, int> >> chunkInfos; // height, outflows:(index, amount)

    // collect info for each chunk to render
    for (int i = 0; i < this->subChunks.size(); i++) {

        shared_ptr<Chunk> subChunk = this->subChunks[i];
        unordered_map<int, int> flowDirs; //(dir, amount)
        if (subChunk->outFlow != -1) {
            int dir = subChunk->flowSide;
            flowDirs[dir] += subChunk->totalFlow;
        }
        else if (i == this->outFlow) {
            int dir = this->flowSide;
            flowDirs[dir] += this->totalFlow;
        }

        string height = std::to_string( std::max(subChunk->position->y, (int64_t) subChunk->lakeLevel) );
        if (height.length() == 5) {
            height.insert(2, ",");
        }
        else if (height.length() == 4) {
            height.insert(1, ",");
        }
        

        chunkInfos.push_back(
            pair<string, unordered_map<int, int>>(height, flowDirs)
        );
    }


    // render
    std::stringstream ss;
    for (int y = square - 1; y >= 0; y--) {
        for (int x = 0; x < square; x++) {
            ss << " ------------------ ";
        }
        ss << "\n";
        for (int x = 0; x < square; x++) {
            ss << "|(";
            int index = y * square + x;
            string label = std::to_string(index);
            ss << label;
            ss << ")";
            for (int i = 0; i < 6 - label.length(); i++) ss << " ";

            if (chunkInfos[index].second.contains(0)) {
                ss << "/\\";
            }
            else {
                ss << "  ";
            }
            ss << "         ";
        }
        ss << "|\n";
        for (int x = 0; x < square; x++) {
            ss << "|        ";
            int index = y * square + x;
            if (chunkInfos[index].second.contains(0)) {
                string flow = std::to_string(chunkInfos[index].second[0]);
                flow = flow.substr(0, 5);
                ss << flow;
                int end = 5 - flow.length();
                for (int i = 0; i < end; i++) {
                    ss << " ";
                }
            }
            else {
                ss << "     ";
            }
            ss << "      ";
        }
        ss << "|\n";
        for (int x = 0; x < square; x++) {
            ss << "|";
            int index = y * square + x;
            if (chunkInfos[index].second.contains(3)) {
                string flow = std::to_string(chunkInfos[index].second[3]);
                flow = flow.substr(0, 5);
                ss << " <" << flow;
                int end = 5 - flow.length();
                for (int i = 0; i < end; i++) ss << " ";
            }
            else {
                ss << "       ";
            }

            if (this->subChunks[index]->lakeLevel > this->subChunks[index]->position->y)
                ss << " La  ";
            else
                ss << "     ";

            if (chunkInfos[index].second.contains(1)) {
                string flow = std::to_string(chunkInfos[index].second[1]);
                flow = flow.substr(0, 5);
                int end = 5 - flow.length();
                for (int i = 0; i < end; i++) ss << " ";
                ss << flow << "> ";
            }
            else {
                ss << "       ";
            }
        }
        ss << "|\n";
        for (int x = 0; x < square; x++) {
            ss << "|        ";
            int index = y * square + x;
            if (chunkInfos[index].second.contains(2)) {
                string flow = std::to_string(chunkInfos[index].second[2]);
                flow = flow.substr(0, 5);
                ss << flow;
                int end = 5 - flow.length();
                for (int i = 0; i < end; i++) ss << " ";
            }
            else {
                ss << "     ";
            }
            ss << "      ";
        }
        ss << "|\n";
        for (int x = 0; x < square; x++) {
            ss << "|";

            int index = y * square + x;
            string height = chunkInfos[index].first;
            height = height.substr(0, 8);
            ss << height;
            int end = 8 - height.length();
            for (int i = 0; i < end; i++) ss << " ";

            if (chunkInfos[index].second.contains(2)) {
                ss << "\\/";
            }
            else {
                ss << "  ";
            }
            ss << "         ";
        }
        ss << "|\n";
    }

    for (int x = 0; x < square; x++) {
        ss << " ------------------ ";
    }
    ss << "\n";

    ss << this->position->toString() << "\n";
    ss << this->stringTag() << "\n";
    ss << this->getId() << "\n";
    ss << "precipitation " << this->precipitation << "\n";
    ss << "ouflow " << this->outFlow << "\n";
    ss << "totalFlow " << this->totalFlow << "\n";
    for (pair<int, int> inflow : this->inFlows) {
        ss << "inflow " << inflow.first << " of " << inflow.second << " units" << "\n";  

    }

    string string = ss.str();

    std::ofstream outputFile(file_path);

    // Check if the file is successfully opened
    if (outputFile.is_open()) {
        // Write data to the file
        outputFile << string;

        outputFile.close();
    }
    else {
        std::cerr << "Error opening the file." << std::endl;
    }

    return string;
}


string Chunk::parentToString(const shared_ptr<Planet>& planet)
{
    shared_ptr<Chunk> parent = this->rootChunk.lock()->getChunk(larger(this->getType()), this->position->x, this->position->z);

    return parent->toString();
}


void Chunk::dump_to_string(int x, int z, const shared_ptr<Planet>& planet) {

    shared_ptr<Chunk> spawn_2097152 = planet->rootChunk->getChunk(ChunkType::CHUNK_2097152, x, z);
    if (spawn_2097152 != nullptr && spawn_2097152->initialized)
        spawn_2097152->toString("src/TEST/output/CHUNK_2097152");
    shared_ptr<Chunk> spawn_131072 = planet->rootChunk->getChunk(ChunkType::CHUNK_131072, x, z);
    if (spawn_131072 != nullptr && spawn_131072->initialized)
        spawn_131072->toString("src/TEST/output/CHUNK_131072");
    shared_ptr<Chunk> spawn_8192 = planet->rootChunk->getChunk(ChunkType::CHUNK_8192, x, z);
    if (spawn_8192 != nullptr && spawn_8192->initialized)
        spawn_8192->toString("src/TEST/output/CHUNK_8192");
    shared_ptr<Chunk> spawn_512 = planet->rootChunk->getChunk(ChunkType::CHUNK_512, x, z);
    if (spawn_512 != nullptr && spawn_512->initialized)
        spawn_512->toString("src/TEST/output/CHUNK_512");
    shared_ptr<Chunk> spawn_64 = planet->rootChunk->getChunk(ChunkType::CHUNK_64, x, z);
    if (spawn_64 != nullptr && spawn_64->initialized)
        spawn_64->toString("src/TEST/output/CHUNK_64");
    shared_ptr<Chunk> spawn_16 = planet->rootChunk->getChunk(ChunkType::CHUNK_16, x, z);
    if (spawn_16 != nullptr && spawn_16->initialized)
        spawn_16->toString("src/TEST/output/CHUNK_16");
    shared_ptr<Chunk> spawn_4 = planet->rootChunk->getChunk(ChunkType::CHUNK_4, x, z);
    if (spawn_4 != nullptr && spawn_4->initialized)
        spawn_4->toString("src/TEST/output/CHUNK_4");
}




int Chunk::getIndexInParent(shared_ptr<Chunk> parent, const shared_ptr<Planet>& planet) {
    int xOff = this->position->x - parent->position->x;
    int zOff = this->position->z - parent->position->z;
    int thisSize = Chunk::CHUNK_SIZES[this->getType()];
    xOff = xOff / thisSize;
    zOff = zOff / thisSize;

    return zOff * parent->getSquareOfSub() + xOff;
}



// Other

int Chunk::getSquareOfSub()
{
    if (this->type == ChunkType::CHUNK_1) {
        return 1;
    }
    else if (this->type == ChunkType::CHUNK_4) {
        return 4;
    }
    else if (this->type == ChunkType::CHUNK_16) {
        return 4;
    }
    else if (this->type == ChunkType::CHUNK_64) {
        return 4;
    }
    else if (this->type == ChunkType::CHUNK_512) {
        return 8;
    }
    else if (this->type == ChunkType::CHUNK_8192) {
        return 16;
    }
    else if (this->type == ChunkType::CHUNK_131072) {
        return 16;
    }
    else if (this->type == ChunkType::CHUNK_2097152) {
        return 16;
    }
    else if (this->type == ChunkType::CHUNK_16777216) {
        return 8;
    }
    else if (this->type == ChunkType::CHUNK_134217728) {
        return 8;
    }

    return 1;
}

int Chunk::getLevel()
{
	return level(this->type);
}

ChunkType Chunk::topType()
{
    shared_ptr<Chunk> root = this->rootChunk.lock();
    if (root && root->subChunkType != ChunkType::NONE)
        return root->subChunkType;
    return ChunkType::CHUNK_2097152;
}

ChunkType Chunk::topTypeFor(int64_t worldSize)
{
    ChunkType candidates[] = { ChunkType::CHUNK_134217728, ChunkType::CHUNK_16777216, ChunkType::CHUNK_2097152 };
    for (ChunkType c : candidates)
        if (CHUNK_SIZES[c] <= worldSize) return c;
    return ChunkType::CHUNK_2097152;
}

ChunkType Chunk::getType()
{
    return this->type;
}

shared_ptr<Chunk> Chunk::getSubChunk(Offset& offset)
{
    int index = Chunk::convertToIndex(offset.x, offset.z, this->getSquareOfSub());
    return subChunks[index];
}

void Chunk::get_coordinates_by_chunk_type(ChunkType type, int64_t& x, int64_t& z)
{
    int size = (int) type;
    x = (x / size ) * size;
    z = (z / size ) * size;
}

string Chunk::chunk16_group_id(double x, double z) {
    return Chunk::type_group_id(ChunkType::CHUNK_16, x, z);
}

string Chunk::type_group_id(ChunkType type, double x, double z) {

    // four doubles like 3.99 should be interpreted as 4
    int64_t x_i = Util::round_if_almost(x, (double)type);
    int64_t z_i = Util::round_if_almost(z, (double)type);

    // snap to chunk size
    Chunk::get_coordinates_by_chunk_type(ChunkType::CHUNK_16, x_i, z_i);
    
    // make string tag
    return Block::stringTagNoY((int) type, x_i, z_i);
}

int64_t Chunk::render_distance_for_position(int64_t x, int64_t z, int64_t player_x, int64_t player_z, shared_ptr<Planet>& planet) {

    int64_t dist = CoordinateConversion::cheapWrapMaxDistance(x, z, player_x, player_z, planet->worldSize);

    if (dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_1])
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_1];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_4]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_4];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_64]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_64];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_512]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_512];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_8192]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_8192];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_131072]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_131072];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_2097152]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_2097152];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16777216]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_16777216];
    else if(dist < Chunk::RENDER_DISTANCES[ChunkType::CHUNK_134217728]) 
        return Chunk::RENDER_DISTANCES[ChunkType::CHUNK_134217728];

    return -1;
}



/*
* neighborChunks in this order:
*
* _ 0 4
* 3 X 1
* _ 2
* 
* where X is the chunk of pos
*
* To get consistant results from this you need to ensure that all neighboring parents have also initialized their chunks
*/
vector<shared_ptr<Chunk>> Chunk::getChunkNeighbors_N_E_S_W_NE(ChunkType type, shared_ptr<Position>& pos)
{
    // WARNING assumes chunk size is less than world size
    int64_t northZ = pos->z + Chunk::CHUNK_SIZES[type];
    northZ -= (northZ >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t eastX = pos->x + Chunk::CHUNK_SIZES[type];
    eastX -= (eastX >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t southZ = pos->z - Chunk::CHUNK_SIZES[type];
    southZ += (southZ < 0) ? this->rootChunk.lock()->world_size : 0;
    int64_t westX = pos->x - Chunk::CHUNK_SIZES[type];
    westX += (westX < 0) ? this->rootChunk.lock()->world_size : 0;

    vector<shared_ptr<Chunk>> neighbors;


    // North
    shared_ptr<Chunk> north = this->getChunk(type, pos->x, northZ);
    neighbors.push_back(north);

    // East
    shared_ptr<Chunk> east = this->getChunk(type, eastX, pos->z);
    neighbors.push_back(east);

    // South
    shared_ptr<Chunk> south = this->getChunk(type, pos->x, southZ);
    neighbors.push_back(south);

    // West
    shared_ptr<Chunk> west = this->getChunk(type, westX, pos->z);
    neighbors.push_back(west);

    // North East
    shared_ptr<Chunk> northEast = this->getChunk(type, eastX, northZ);
    neighbors.push_back(northEast);


    return neighbors;
}


shared_ptr<Chunk> Chunk::getChunk(ChunkType type, int64_t x, int64_t z) {
    ChunkType top = this->topType();
    vector<Offset> path = Chunk::getPath(type, x, z, top);
    shared_ptr<Chunk> currentChunk = this->rootChunk.lock();
    int64_t chunk_level = level(type) - level(top) + 1;
    for (int64_t i = 0; i < chunk_level; i++) {
        if (!currentChunk->subChunks.empty()) {
            currentChunk = currentChunk->getSubChunk(path[i]);
        }
        else {
            return nullptr;
        }
    }

    return currentChunk;
}

/*
* neighborChunks in this order:
*
*   0 
* 3 X 1
*   2 
*
* where X is the chunk of pos
* 
* if a chunk isn't initialized, it's parent/most recent ancestor, is returned
* 
* This method is mostly meant for determining downblocks, and shouldn't used in other cases that need consistant results
*
*/
vector<shared_ptr<Chunk>> Chunk::getNESWNeighborsInitialized(ChunkType type, shared_ptr<Position>& pos)
{
    // WARNING assumes chunk size is less than world size
    int64_t northZ = pos->z + Chunk::CHUNK_SIZES[type];
    northZ -= (northZ >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t eastX = pos->x + Chunk::CHUNK_SIZES[type];
    eastX -= (eastX >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t southZ = pos->z - Chunk::CHUNK_SIZES[type];
    southZ += (southZ < 0) ? this->rootChunk.lock()->world_size : 0;
    int64_t westX = pos->x - Chunk::CHUNK_SIZES[type];
    westX += (westX < 0) ? this->rootChunk.lock()->world_size : 0;

    vector<shared_ptr<Chunk>> neighbors;


    // North
    shared_ptr<Chunk> north = this->getChunkOrClosestAncestorInitialized(type, pos->x, northZ);
    neighbors.push_back(north);

    // East
    shared_ptr<Chunk> east = this->getChunkOrClosestAncestorInitialized(type, eastX, pos->z);
    neighbors.push_back(east);

    // South
    shared_ptr<Chunk> south = this->getChunkOrClosestAncestorInitialized(type, pos->x, southZ);
    neighbors.push_back(south);

    // West
    shared_ptr<Chunk> west = this->getChunkOrClosestAncestorInitialized(type, westX, pos->z);
    neighbors.push_back(west);


    return neighbors;
}

shared_ptr<Chunk> Chunk::getNeighborInitialized(shared_ptr<Chunk> chunk, int64_t side)
{
    // WARNING assumes chunk size is less than world size


    // north
    if (side == 0) {
        int64_t northZ = chunk->position->z + Chunk::CHUNK_SIZES[chunk->getType()];
        northZ -= (northZ >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
        return this->getChunkOrClosestAncestorInitialized(chunk->getType(), chunk->position->x, northZ);
    }
    // east
    else if (side == 1) {
        int64_t eastX = chunk->position->x + Chunk::CHUNK_SIZES[chunk->getType()];
        eastX -= (eastX >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
        return this->getChunkOrClosestAncestorInitialized(chunk->getType(), eastX, chunk->position->z);
    }
    // south
    else if (side == 2) {
        int64_t southZ = chunk->position->z - Chunk::CHUNK_SIZES[chunk->getType()];
        southZ += (southZ < 0) ? this->rootChunk.lock()->world_size : 0;
        return this->getChunkOrClosestAncestorInitialized(chunk->getType(), chunk->position->x, southZ);
    }
    // west
    else if (side == 3) {
        int64_t westX = chunk->position->x - Chunk::CHUNK_SIZES[chunk->getType()];
        westX += (westX < 0) ? this->rootChunk.lock()->world_size : 0;
        return this->getChunkOrClosestAncestorInitialized(chunk->getType(), westX, chunk->position->z);
    }

    return nullptr;
}


shared_ptr<Chunk> Chunk::getChunkOrClosestAncestorInitialized(ChunkType type, int64_t x, int64_t z) {
    ChunkType top = this->topType();
    vector<Offset> path = Chunk::getPath(type, x, z, top);
    shared_ptr<Chunk> currentChunk = this->rootChunk.lock();
    int64_t chunk_level = level(type) - level(top) + 1;
    for (int64_t i = 0; i < chunk_level; i++) {
        if (currentChunk->subChunksInitialized) {
            currentChunk = currentChunk->getSubChunk(path[i]);
        }
        else {
            return currentChunk;
        }
    }

    return currentChunk;
}


vector<shared_ptr<Chunk>> Chunk::getNeighborsNESWOrAncestorsInWorld(ChunkType type, shared_ptr<Position>& pos, const unordered_map<string, shared_ptr<Block>>& blocks_in_world) {
    // WARNING assumes chunk size is less than world size
    int64_t northZ = pos->z + Chunk::CHUNK_SIZES[type];
    northZ -= (northZ >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t eastX = pos->x + Chunk::CHUNK_SIZES[type];
    eastX -= (eastX >= this->rootChunk.lock()->world_size) ? this->rootChunk.lock()->world_size : 0;
    int64_t southZ = pos->z - Chunk::CHUNK_SIZES[type];
    southZ += (southZ < 0) ? this->rootChunk.lock()->world_size : 0;
    int64_t westX = pos->x - Chunk::CHUNK_SIZES[type];
    westX += (westX < 0) ? this->rootChunk.lock()->world_size : 0;

    vector<shared_ptr<Chunk>> neighbors;

    // North
    shared_ptr<Chunk> north = this->getChunkOrClosestAncestorInWorld(type, pos->x, northZ, blocks_in_world);
    neighbors.push_back(north);

    // East
    shared_ptr<Chunk> east = this->getChunkOrClosestAncestorInWorld(type, eastX, pos->z, blocks_in_world);
    neighbors.push_back(east);

    // South
    shared_ptr<Chunk> south = this->getChunkOrClosestAncestorInWorld(type, pos->x, southZ, blocks_in_world);
    neighbors.push_back(south);

    // West
    shared_ptr<Chunk> west = this->getChunkOrClosestAncestorInWorld(type, westX, pos->z, blocks_in_world);
    neighbors.push_back(west);

    return neighbors;
}


shared_ptr<Chunk> Chunk::getChunkOrClosestAncestorInWorld(ChunkType type, int64_t x, int64_t z, const unordered_map<string, shared_ptr<Block>>& blocks_in_world) {
    ChunkType top = this->topType();
    vector<Offset> path = Chunk::getPath(type, x, z, top);
    shared_ptr<Chunk> last_in_world_or_not_spawnable = this->rootChunk.lock();
    shared_ptr<Chunk> currentChunk = this->rootChunk.lock();
    int64_t chunk_level = level(type) - level(top) + 1;
    for (int64_t i = 0; i < chunk_level; i++) {
        if (!currentChunk->subChunks.empty()) {
            currentChunk = currentChunk->getSubChunk(path[i]);
            bool can_be_in_world = currentChunk->getSize() <= (int) ChunkType::CHUNK_16;
            bool is_in_world = blocks_in_world.find(currentChunk->stringTag()) != blocks_in_world.end();
            if (!can_be_in_world || is_in_world) {
                last_in_world_or_not_spawnable = currentChunk;
            }
        }
        else {
            break;
        }

    }

    return last_in_world_or_not_spawnable;
}

vector<shared_ptr<Chunk>> Chunk::getChunksAtPosition(int64_t x, int64_t z) {
    ChunkType top = this->topType();
    vector<Offset> path = Chunk::getPath(ChunkType::CHUNK_1, x, z, top);
    
    vector<shared_ptr<Chunk>> chunks;

    shared_ptr<Chunk> currentChunk = this->rootChunk.lock();
    chunks.push_back(currentChunk);
    for (int64_t i = 0; i < path.size(); i++) {
        if (currentChunk != nullptr && !currentChunk->subChunks.empty()) {
            currentChunk = currentChunk->getSubChunk(path[i]);
        }
        else {
            break;
        }
        chunks.push_back(currentChunk);
    }

    return chunks;
}


vector<Offset> Chunk::getPath(ChunkType type, int64_t x, int64_t z, ChunkType startType) {

    int64_t chunk_level = level(type) - level(startType) + 1;
    vector<Offset> path;
    path.reserve(chunk_level); // avoid reallocations
    ChunkType currentType = startType;
    for (int64_t i = 0; i < chunk_level; i++) {
        int size = (int) currentType;
        int64_t ox = x / size;
        int64_t oz = z / size;
        path.emplace_back(ox, oz); // construct in-place, no copy
        x -= ox * size;            // reuse locals instead of indexing back
        z -= oz * size;
        currentType = smaller(currentType);
    }

    return path;
}


vector<shared_ptr<Chunk>> Chunk::getParents(ChunkType type, int64_t x, int64_t z)
{
    vector<shared_ptr<Chunk>> parents;

    ChunkType top = this->topType();
    vector<Offset> path = Chunk::getPath(type, x, z, top);
    shared_ptr<Chunk> currentChunk = this->rootChunk.lock();
    int64_t chunk_level = level(type) - level(top) + 1;
    for (int64_t i = 0; i < chunk_level; i++) {
        if (currentChunk != nullptr && !currentChunk->subChunks.empty()) {
            currentChunk = currentChunk->getSubChunk(path[i]);
            parents.push_back(currentChunk);
        }
        else {
            parents.push_back(nullptr);
            currentChunk = nullptr;
        }
    }

    return parents;
}

vector<shared_ptr<Chunk>> Chunk::getChunksInRadius(ChunkType type, int64_t x, int64_t z, int64_t radius)
{
    int64_t chunk_size = Chunk::CHUNK_SIZES[type];

    
    vector<shared_ptr<Chunk>> chunks;
    for (int64_t x_i = x - radius; x_i <= x + radius; x_i += chunk_size) {
        for (int64_t z_i = z - radius; z_i <= z + radius; z_i += chunk_size) {
            int64_t chunk_x = x_i;
            int64_t chunk_z = z_i;
            CoordinateConversion::wrap(chunk_x, chunk_z, this->rootChunk.lock()->world_size);
            // cout << "chunk_x: " << chunk_x << " chunk_z: " << chunk_z << endl;
            shared_ptr<Chunk> chunk = this->getChunk(type, chunk_x, chunk_z);
            if (chunk != nullptr) {
                chunks.push_back(chunk);
            }
        }
    }
    
    return chunks;
}

shared_ptr<Chunk> Chunk::get_neighbor(ChunkType type, int64_t side, shared_ptr<Chunk> chunk)
{

    int64_t x = chunk->position->x;
    int64_t z = chunk->position->z;
    if (side == 0) { // N
        z += Chunk::CHUNK_SIZES[type];
    }
    if (side == 1) { // E
        x += Chunk::CHUNK_SIZES[type];
    }
    if (side == 2) { // S
        z -= Chunk::CHUNK_SIZES[type];
    }
    if (side == 3) { // W
        x -= Chunk::CHUNK_SIZES[type];
    }


    CoordinateConversion::wrap(x, z, this->rootChunk.lock()->world_size);

    return this->getChunk(type, x, z);
}

PrecipitationType Chunk::get_precipitationType(int precipitation)
{
    if (precipitation <= (int)PrecipitationType::HURRICANE && precipitation > (int)PrecipitationType::RAIN_FOREST) {
        return PrecipitationType::HURRICANE;
    }
    else if (precipitation <= (int)PrecipitationType::RAIN_FOREST && precipitation > (int)PrecipitationType::TEMPERATE) {
        return PrecipitationType::RAIN_FOREST;
    }
    else if (precipitation <= (int)PrecipitationType::TEMPERATE && precipitation > (int)PrecipitationType::SEMI_ARID) {
        return PrecipitationType::TEMPERATE;
    }
    else if (precipitation <= (int)PrecipitationType::SEMI_ARID && precipitation > (int)PrecipitationType::DESERT) {
        return PrecipitationType::SEMI_ARID;
    }
    else {
        return PrecipitationType::DESERT;
    }
}

void Chunk::get_relative_slope_and_highest_neighbor(ChunkType type, vector<shared_ptr<Chunk>> parentNeighborChunks, double& slope_result, int64_t& highest_neighbor_result)
{
    // get max and min slope of neigbors and self
    int64_t y_min = this->position->y;
    int64_t y_max = this->position->y;
    for (shared_ptr<Chunk> pNeighbor: parentNeighborChunks) {
        if (pNeighbor->position->y < y_min) y_min = pNeighbor->position->y;
        if (pNeighbor->position->y > y_max) y_max = pNeighbor->position->y;
    }

    // set results
    slope_result = (y_max - y_min) / (double) type;
    highest_neighbor_result = y_max;
}


shared_ptr<Position> Chunk::center_pos()
{
    int half_size = this->size / 2;
    return std::make_shared<Position>(this->position->x + half_size, this->position->y, this->position->z + half_size);
}
