// Fill out your copyright notice in the Description page of Project Settings.


#include "util/Offset.h"

Offset::Offset(int x, int z)
{
	this->x = x;
	this->z = z;
}

Offset::~Offset()
{
}


string Offset::toString()
{
	std::ostringstream oss;
	oss << "(" << x << ", " << z << ")";
	return oss.str();
}
