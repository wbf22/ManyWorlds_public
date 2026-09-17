// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/Position.h"



/**
 * 
 */
class WaterContract
{
public:
	WaterContract();
	~WaterContract();



	int index;
	bool isInFlow = false; // inflow into this chunk
};
