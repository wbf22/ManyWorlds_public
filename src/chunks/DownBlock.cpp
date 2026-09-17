// Fill out your copyright notice in the Description page of Project Settings.


#include "DownBlock.h"


DownBlock::DownBlock(string material, shared_ptr<Position> position, uptr<int[]> waterFlows, bool isCave, int size)
{
	this->material = material;
	this->position = position;
	this->waterFlows = std::move(waterFlows);
	this->isCave = isCave;
	this->size = size;
}

DownBlock::~DownBlock()
{
}

int DownBlock::getSize()
{
	return this->size;
}
