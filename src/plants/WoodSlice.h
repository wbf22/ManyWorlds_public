// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include <unordered_map>
#include "../blocks/Block.h"
#include "util/Offset.h"





using namespace std;


/**
 * 
 */
class WoodSlice
{
public:
	WoodSlice();
	~WoodSlice();

	shared_ptr<Position> straightnessOffset;
	unordered_map<string, shared_ptr<Block>> blocks; // map of position string (offset from slice center) to block


};
