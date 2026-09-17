// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <cstdlib>
#include <string>
#include <sstream>
#include <memory>

using namespace std;

/**
 * Offset is used for a 'index' in a 2d array (even though we use a 1d array internally)
 */
class Offset
{
public:
	Offset(int x, int z);
	~Offset();

	int x;
	int z;


	// Overload the equality operator
	bool operator==(const Offset& other) const {
		return x == other.x && z == other.z;
	}

	string toString();

};

// Hash function for Offset
struct OffsetHash {
	std::size_t operator()(const std::shared_ptr<Offset>& offset) const {
		std::size_t hash = 17;
		hash = hash * 31 + std::hash<int>()(offset->x);
		hash = hash * 31 + std::hash<int>()(offset->z);
		return hash;
	}
};