// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Position.h"
#include "util/Random.h"
#include "util/Offset.h"
#include <array>
#include <stack>
#include "../blocks/Block.h"



template<typename T>
using uptr = std::unique_ptr<T>;



using namespace std;


/**
 * 
 */
class DownBlock : public Block
{
public:
	DownBlock(string material, shared_ptr<Position> position, uptr<int[]> waterFlows, bool isCave, int size);
	~DownBlock();


	uptr<int[]> waterFlows; // N E S W U
	bool isCave;



	int getSize();


};
