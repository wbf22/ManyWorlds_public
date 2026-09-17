// Fill out your copyright notice in the Description page of Project Settings.


#include "RootChunk.h"


RootChunk::RootChunk(int square, int parentSmoothness, shared_ptr<Planet>& planet)
    : Chunk(parentSmoothness, ChunkType::ROOT, std::make_shared<Position>(0,0,0), planet->seed, {}, planet->precipitation)
{
    this->substrate_material = "";
	this->square = square;

    // adaptive top tier: largest chunk size <= planet circumference (falls back to CHUNK_2097152)
    this->subChunkType = Chunk::topTypeFor(planet->worldSize);

    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_134217728] = planet->max_134217728;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_16777216] = planet->max_16777216;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_2097152] = planet->max_2097152;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_131072] = planet->max_131072;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_8192] = planet->max_8192;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_512] = planet->max_512;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_64] = planet->max_64;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_16] = planet->max_16;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_4] = planet->max_4;
    Chunk::CHUNK_SMOOTHNESS_MAX[ChunkType::CHUNK_1] = planet->max_1;

    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_134217728] = planet->min_134217728;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_16777216] = planet->min_16777216;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_2097152] = planet->min_2097152;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_131072] = planet->min_131072;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_8192] = planet->min_8192;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_512] = planet->min_512;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_64] = planet->min_64;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_16] = planet->min_16;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_4] = planet->min_4;
    Chunk::CHUNK_SMOOTHNESS_MIN[ChunkType::CHUNK_1] = planet->min_1;

    this->smoothness = parentSmoothness;

    this->world_size = planet->worldSize;

    this->type = ChunkType::ROOT;
}

void RootChunk::init(shared_ptr<Planet>& planet) {

    int numSubChunks = this->square * this->square;

    // init rock layers (0-400 blocks or 0-200 meters, execpt when you're really deep)
    int64_t pos_spike = planet->seed * 21;
    int64_t pos_seed_spike = planet->seed + 21;

    int j = Planet::maxHeight;
    this->rock_slope = 0.0;
    this->angle = 0.0;
    while (j > Planet::minHeight) {
        int layer_thickness = 0; 
        if (j < -7000) {
            layer_thickness = Random::randInt(planet->seed + pos_spike + j, 1, 100, 1000);
        }
        else {
            layer_thickness = Random::randInt(planet->seed + pos_spike + j, 1, 5, 200);
        }
        this->rockLayerHeights.push_back(j);
        this->rockLayerTypes.push_back(
            BMaterial::getRandomMaterial(
                pos_seed_spike + j, 
                planet->precipitation > 0, 
                planet->hasLife, 
                false
            )
        );

        j -= layer_thickness;
    }


    // INIT SUB CHUNKS
    this->subChunks.resize(numSubChunks);
    for (int i = 0; i < numSubChunks; i++) {
        shared_ptr<Position> subChunkPos = this->convertIndex(i);
        shared_ptr<Chunk> subChunk = this->newSubChunk(this->smoothness, subChunkPos, planet->seed);

        this->subChunks[i] = subChunk;

        subChunk->precipitation = planet->precipitation * Random::randInt(planet->seed + subChunk->position->makeSpike(), 50, 150) * 0.01;
        subChunk->precipitation = (subChunk->precipitation > 100) ? 100 : subChunk->precipitation;

    }

    // MAKE SEAS AND CHUNK HEIGHTS
    vector<int> seaChunks;
    int maxHeightAdjusted = planet->maxHeight / 2;
    for (int i = 0; i < numSubChunks; i++) {
        int random = Random::randInt(planet->seed + this->subChunks[i]->position->makeSpike(), planet->minHeight, maxHeightAdjusted);
        int randomWeight = 100 - this->smoothness;
        int height = (planet->seaLevel * this->smoothness) + (random * randomWeight);
        height /= 100;


        this->subChunks[i]->position->y = height;
        this->subChunks[i]->lakeLevel = planet->seaLevel;
        if (height < planet->seaLevel) {
            seaChunks.push_back(i);
        }
    }

    
    // MAKE ROCK LAYERS AND INIT SUBCHUNKS
    for (int i = 0; i < numSubChunks; i++) {
        shared_ptr<Chunk> subChunk = this->subChunks[i];
        subChunk->initRockLayers(this, {}, planet);
        // subChunk->makeEmptySubChunks(1, Chunk::LEVEL_SUB_CHUNKS[ChunkType::CHUNK_2097152], planet);
    }


    // PLAN RIVERS
    unordered_map<int, int> indexToNextIndex;
    vector<int> riverStarts;
    this->planRivers(indexToNextIndex, riverStarts, seaChunks, planet);


    // GRADE AND MAKE RIVERS
    this->gradeBetweenSeasAndRiverStarts(indexToNextIndex, riverStarts, seaChunks, planet);



    // marks this root chunk as initialized so it doesn't get initialized by the Chunk.cpp initialization
    this->subChunksInitialized = true;
    
    // mark all subchunks as initialized (since we've set height and rock layers)
    for (int i = 0; i < numSubChunks; i++) {
        shared_ptr<Chunk> subChunk = this->subChunks[i];
        subChunk->initialized = true;
    }

    this->toString();

}



void RootChunk::planRivers(unordered_map<int, int>& indexToNextIndex, vector<int>& riverStarts, vector<int>& seaChunks, const shared_ptr<Planet>& planet) {
    int numSubChunks = this->subChunks.size();

    // exploreQueue from sea chunks
    int spike = this->subChunks.size() + planet->maxHeight;

    // if sea chunks is empty, add the lowest chunk as a sea chunk to start river planning
    if (seaChunks.size() == 0) {
        int lowestIndex = 0;
        int lowestHeight = this->subChunks[0]->position->y;
        for (int i = 1; i < numSubChunks; i++) {
            if (this->subChunks[i]->position->y < lowestHeight) {
                lowestHeight = this->subChunks[i]->position->y;
                lowestIndex = i;
            }
        }
        seaChunks.push_back(lowestIndex);
    }

    unordered_map<int, deque<int>> seaIndexToExploreQueue;
    for (int seaIndex : seaChunks)
        seaIndexToExploreQueue[seaIndex] = { seaIndex };

    unordered_set<int> visited;
    while (visited.size() < numSubChunks) {

        for (int seaIndex : seaChunks) {

            deque<int>& exploreQueue = seaIndexToExploreQueue[seaIndex];

            if (exploreQueue.empty()) continue; // avoid popping insanity off the queue

            int index = exploreQueue.back();
            exploreQueue.pop_back();

            if (visited.find(index) != visited.end()) continue;
            visited.insert(index);

            // get neighbors
            vector<int> neighbors = this->getNeighborsRoot(index, numSubChunks);
            vector<int> valids;
            for (int n : neighbors) {
                if (!indexToNextIndex.contains(n) && !seaIndexToExploreQueue.contains(n))
                    valids.push_back(n);
            }

            // dead ends are riverstarts
            if (valids.size() == 0 && !seaIndexToExploreQueue.contains(index))
                riverStarts.push_back(index);

            // enqueue randomly
            for (int n : valids) {

                if (this->subChunks[n]->position->y > planet->seaLevel)
                    indexToNextIndex[n] = index;

                // randomly enqueue on front or back to visit indices out of order
                if (Random::cheapRandom(spike, 0, 2) == 0) {
                    exploreQueue.push_front(n);
                }
                else {
                    exploreQueue.push_back(n);
                }
                spike += n;
            }


            spike += index;
        }

    }
}


void RootChunk::gradeBetweenSeasAndRiverStarts(unordered_map<int, int>& indexToNextIndex, vector<int>& riverStarts, vector<int>& seaChunks, const shared_ptr<Planet>& planet) {
    int numSubChunks = this->subChunks.size();
    int spike = this->position->makeSpike();
    int childSquare = this->subChunks[0]->getSquareOfSub();
    int childNumSubChunks = childSquare * childSquare;


    // COLLECT PATHS
    unordered_map<int, vector<int>> riverStartToPath;
    for (int start : riverStarts) {
        int index = start;
        vector<int> path;
        while (indexToNextIndex.contains(index)) {
            int nextIndex = indexToNextIndex[index];
            path.push_back(nextIndex);
            index = nextIndex;
        }
        riverStartToPath[start] = path;
    }


    // GRADE AND MAKE FLOWS
    std::sort(riverStarts.begin(), riverStarts.end(), [this](const auto& a, const auto& b) {
        int heightA = this->subChunks[a]->position->y;
        int heightB = this->subChunks[b]->position->y;
        return heightA > heightB; // lowest to highest
    });

    unordered_set<int> alreadyGraded;
    for (int index : seaChunks)
        alreadyGraded.insert(index);
    for (int start : riverStarts) {
        int lastIndex = start;
        int lastHeight = this->subChunks[start]->position->y;

        vector<int>& path = riverStartToPath[start];
        for (int index : path) {
            if (alreadyGraded.contains(index)) {
                lastHeight = this->subChunks[index]->position->y;
            }
            else {
                if (lastHeight > planet->seaLevel)
                    lastHeight = Random::randInt(planet->seed+this->subChunks[index]->position->makeSpike(), planet->seaLevel, lastHeight);

                this->subChunks[index]->position->y = lastHeight;
                alreadyGraded.insert(index);
            }

            // make flow
            this->makeWaterContracts(lastIndex, index, 1, this->square, childNumSubChunks, childSquare, planet);
            lastIndex = index;
        }

    }

    // init corner heights
    for (int i = 0; i < numSubChunks; i++) {
        this->subChunks[i]->initCornerHeight(planet);
    }

}


vector<int> RootChunk::getNeighborsRoot(int currentIndex, int numSubChunks) {

    int north = currentIndex + this->square;
    north -= (north >= numSubChunks) ? this->subChunks.size() : 0;
    int east = currentIndex + 1;
    east -= (east % square == 0) ? square : 0;
    int south = currentIndex - this->square;
    south += (south < 0) ? this->subChunks.size() : 0;
    int west = currentIndex - 1;
    west += ((west + 1) % square == 0) ? square : 0;

    vector<int> neighbors;
    neighbors.push_back(north);
    neighbors.push_back(east);
    neighbors.push_back(south);
    neighbors.push_back(west);

    return neighbors;
}



int RootChunk::positionToIndex(shared_ptr<Position> pos, const shared_ptr<Planet>& planet) {
    int xPos = pos->x;
    int zPos = pos->z;

    while (xPos >= planet->worldSize)
        xPos -= planet->worldSize;
    while (xPos < 0)
        xPos += planet->worldSize;

    while (zPos >= planet->worldSize)
        zPos -= planet->worldSize;
    while (zPos < 0)
        zPos += planet->worldSize;

    int x = xPos / CHUNK_SIZES[this->subChunkType];
    int z = zPos / CHUNK_SIZES[this->subChunkType];

    return z * this->square + x;
}


int RootChunk::getSquareOfSub()
{
    return this->square;
}



