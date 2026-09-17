// Fill out your copyright notice in the Description page of Project Settings.


#include "PositionDouble.h"


PositionDouble::PositionDouble(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

PositionDouble::PositionDouble()
{
}

PositionDouble::~PositionDouble()
{
}

double PositionDouble::euclideanDistance(shared_ptr<PositionDouble> other)
{
	double xDiff = this->x - other->x;
	double yDiff = this->y - other->y;
	double zDiff = this->z - other->z;

	return std::sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
}


double PositionDouble::manhattenDistance(shared_ptr<PositionDouble>& other) {
	double xDiff = this->x - other->x;
	double yDiff = this->y - other->y;
	double zDiff = this->z - other->z;

	return std::abs(xDiff) + std::abs(yDiff) + std::abs(zDiff);
}


double PositionDouble::wrapManhattenDistance(shared_ptr<PositionDouble>& other, int wrapValue) {
	double xdist = std::min({ abs(this->x - other->x), abs(this->x - wrapValue - other->x), abs(this->x + wrapValue - other->x) });
	double ydist = abs(this->y - other->y);
	double zdist = std::min({ abs(this->z - other->z), abs(this->z - wrapValue - other->z), abs(this->z + wrapValue - other->z) });
	return std::max({ xdist, ydist, zdist });
}

string PositionDouble::toString()
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2);
	oss << "x=" << this->x << " y=" << this->y << " z=" << this->z;
	return oss.str();
}

shared_ptr<PositionDouble> PositionDouble::add(shared_ptr<PositionDouble> other)
{
	double x = this->x + other->x;
	double y = this->y + other->y;
	double z = this->z + other->z;

    return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::sub(shared_ptr<PositionDouble> other)
{
	double x = this->x - other->x;
	double y = this->y - other->y;
	double z = this->z - other->z;

    return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::add(shared_ptr<PositionDouble> other, int64_t world_size)
{
	double x = this->x + other->x;
	while(x > world_size) {
		x -= world_size;
	}
	double y = this->y + other->y;
	double z = this->z + other->z;
	while(z > world_size) {
		z -= world_size;
	}

    return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::sub(shared_ptr<PositionDouble> other, int64_t world_size)
{
	double x = this->x - other->x;
	while(x <= 0) {
		x += world_size;
	}
	double y = this->y - other->y;
	double z = this->z - other->z;
	while(z <= 0) {
		z += world_size;
	}

    return std::make_shared<PositionDouble>(x, y, z);
}

// New methods: multiply and divide follow the same wrapping logic as add/subtract
shared_ptr<PositionDouble> PositionDouble::mult(shared_ptr<PositionDouble> other, int64_t world_size)
{
	double x = this->x * other->x;
	while (x > world_size) {
		x -= world_size;
	}

	double y = this->y * other->y;
	// for multiplication we won't wrap y (following add/subtract pattern where y isn't wrapped)
	double z = this->z * other->z;
	while (z > world_size) {
		z -= world_size;
	}

	return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::div(shared_ptr<PositionDouble> other, int64_t world_size)
{
	double x = this->x / other->x;
	while (x <= 0) {
		x += world_size;
	}

	double y = this->y / other->y;
	double z = this->z / other->z;
	while (z <= 0) {
		z += world_size;
	}

	return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::mult(double scalar, int64_t world_size)
{
	double x = this->x * scalar;
	while (x > world_size) {
		x -= world_size;
	}

	double y = this->y * scalar;
	double z = this->z * scalar;
	while (z > world_size) {
		z -= world_size;
	}

	return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::mult(double scalar)
{
	double x = this->x * scalar;
	double y = this->y * scalar;
	double z = this->z * scalar;

	return std::make_shared<PositionDouble>(x, y, z);
}

shared_ptr<PositionDouble> PositionDouble::div(double scalar, int64_t world_size)
{
	double x = this->x / scalar;
	while (world_size != -1 && x <= 0) {
		x += world_size;
	}

	double y = this->y / scalar;
	double z = this->z / scalar;
	while (world_size != -1 && z <= 0) {
		z += world_size;
	}

	return std::make_shared<PositionDouble>(x, y, z);
}
