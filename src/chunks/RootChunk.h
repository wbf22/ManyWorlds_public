// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Chunk.h"
#include <deque>
#include <functional>
#include <unordered_map>
#include <unordered_set>


template<typename T>
using uptr = std::unique_ptr<T>;




using namespace std;


/**
 * 
 */
class RootChunk : public Chunk
{
public:

	int square;



	RootChunk(int square, int parentSmoothness, shared_ptr<Planet>& planet);

	void init(shared_ptr<Planet>& planet);

	void planRivers(unordered_map<int, int>& indexToNextIndex, vector<int>& riverStarts, vector<int>& seaChunks, const shared_ptr<Planet>& planet);

	void gradeBetweenSeasAndRiverStarts(unordered_map<int, int>& indexToNextIndex, vector<int>& riverStarts, vector<int>& seaChunks, const shared_ptr<Planet>& planet);


	vector<int> getNeighborsRoot(int currentIndex, int numSubChunks);

	int positionToIndex(shared_ptr<Position> pos, const shared_ptr<Planet>& planet);


	int getSquareOfSub() override;



};
