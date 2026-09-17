// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Position.h"
#include "util/PositionDouble.h"
#include <sstream>
#include <iomanip> 





using namespace std;


/**
 * 
 */
class WaterBlock : public Block
{
public:

    int water_level;
    int depth;
    bool splashing;


	WaterBlock(int water_level, shared_ptr<Position> position, int depth, bool splashing, int size) {
        this->water_level = water_level;
        this->position = position;
        this->depth = depth;
        this->splashing = splashing;
        this->size = size;
    }
	~WaterBlock() {}

};
