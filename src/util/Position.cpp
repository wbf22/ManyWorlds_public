// Fill out your copyright notice in the Description page of Project Settings.


#include "Position.h"

Position::Position(int64_t x, int64_t y, int64_t z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

Position::~Position()
{
}


std::unique_ptr<Position> Position::build(int x, int z)
{
	return std::make_unique<Position>(x, 0, z);
}

std::unique_ptr<Position> Position::build(int x, int y, int z)
{
	return std::make_unique<Position>(x, y, z);
}

std::shared_ptr<Position> Position::buildS(int x, int y, int z)
{
	return std::make_shared<Position>(x, y, z);
}

int64_t Position::manhatten(std::unique_ptr<Position> other)
{
	return abs(x - other->x) + abs(y - other->y) + abs(z - other->z);
}

int64_t Position::manhatten(std::shared_ptr<Position> other)
{
	return abs(x - other->x) + abs(y - other->y) + abs(z - other->z);
}

int64_t Position::makeSpike()
{
	return x ^ y ^ z << 6;
	/*int xored = x ^ y ^ z;
	int shift = xored << 6;
	return xored ^ shift;*/
}

shared_ptr<PositionDouble> Position::to_position_double() {
	return std::make_shared<PositionDouble>(this->x, this->y, this->z);
}

shared_ptr<Position> Position::from_position_double(shared_ptr<PositionDouble> pos) {
	return std::make_shared<Position>(
		round(pos->x),
		round(pos->y),
		round(pos->z)
	);

}

shared_ptr<Position> Position::mult(int64_t scalar)
{
	int64_t x = this->x * scalar;
	int64_t y = this->y * scalar;
	int64_t z = this->z * scalar;

	return std::make_shared<Position>(x, y, z);
}

string Position::toString()
{
	std::ostringstream oss;
	oss << "x=" << x << " y=" << y << " z=" << z;
	return oss.str();
}

Position Position::wrap(int world_size) {
	int s_x = this->x;
	while (s_x < 0) s_x += world_size;
	if (s_x >= world_size) s_x %= world_size;

	int s_z = this->z;
	while (s_z < 0) s_z += world_size;
	if (s_z >= world_size) s_z %= world_size;


	return Position(
		s_x,
		this->y,
		s_z
	);
}

