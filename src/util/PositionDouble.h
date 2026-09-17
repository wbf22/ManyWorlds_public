// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include <sstream>
#include <iomanip> 
#include <memory>
#include <cmath>
#include <algorithm>
#include "util/serialization/KVSerializer.hpp"


using namespace std;


/**
 * 
 */
class PositionDouble
{
public:
	PositionDouble(double x, double y, double z);
	PositionDouble();
	~PositionDouble();


	double x;
	double y;
	double z;

	double euclideanDistance(shared_ptr<PositionDouble> other);

	double manhattenDistance(shared_ptr<PositionDouble>& other);

	double wrapManhattenDistance(shared_ptr<PositionDouble>& other, int wrapValue);

	string toString();


	shared_ptr<PositionDouble> add(shared_ptr<PositionDouble> other);
	
	shared_ptr<PositionDouble> sub(shared_ptr<PositionDouble> other);


	// with wrapping
	shared_ptr<PositionDouble> add(shared_ptr<PositionDouble> other, int64_t world_size);

	shared_ptr<PositionDouble> sub(shared_ptr<PositionDouble> other, int64_t world_size);

	shared_ptr<PositionDouble> mult(shared_ptr<PositionDouble> other, int64_t world_size);

	shared_ptr<PositionDouble> div(shared_ptr<PositionDouble> other, int64_t world_size);

	shared_ptr<PositionDouble> mult(double scalar, int64_t world_size);

	shared_ptr<PositionDouble> mult(double scalar);

	shared_ptr<PositionDouble> div(double scalar, int64_t world_size=-1);


    DECLARE_NAMED_FIELDS(
        FIELD("x", &PositionDouble::x),
        FIELD("y", &PositionDouble::y),
        FIELD("z", &PositionDouble::z)
    );
};
