#pragma once


#include <cstdlib>
#include <string>
#include <sstream>
#include "PositionDouble.h"
#include "util/serialization/KVSerializer.hpp"

using namespace std;


/**
 * Position is a world coordinate (but not actual position in unreal engine)
 */
class Position
{
public:


	int64_t x;
	int64_t y;
	int64_t z;


	Position(int64_t x, int64_t y, int64_t z);
	Position() = default;
	~Position();


	static std::unique_ptr<Position> build(int x, int z);
	static std::unique_ptr<Position> build(int x, int y, int z);
	static std::shared_ptr<Position> buildS(int x, int y, int z);
	int64_t manhatten(std::unique_ptr<Position> other);
	int64_t manhatten(shared_ptr<Position> other);
	int64_t makeSpike();
	shared_ptr<PositionDouble> to_position_double();

	static shared_ptr<Position> from_position_double(shared_ptr<PositionDouble> pos);

	shared_ptr<Position> mult(int64_t scalar);

	string toString();

    Position wrap(int world_size);

    // Overload the equality operator
	bool operator==(const Position& other) const {
		return x == other.x && y == other.y && z == other.z;
	}

    size_t hash() const {
        size_t h = 0;
        auto combine = [](size_t& seed, size_t v) {
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
        combine(h, std::hash<int64_t>()(x));
        combine(h, std::hash<int64_t>()(y));
        combine(h, std::hash<int64_t>()(z));
        return h;
    }




    DECLARE_NAMED_FIELDS(
        FIELD("x", &Position::x),
        FIELD("y", &Position::y),
        FIELD("z", &Position::z)
    );

};

/**
 * These structs are to allow hasing of positions for sets and stuff. Make the set like this:
 * 
 * std::unordered_set<std::shared_ptr<Position>, PositionHash, PositionEqual>
*/


// // Hash function for Position
// struct PositionHash {
// 	std::size_t operator()(const std::shared_ptr<Position>& position) const {
// 		// Custom hash combining the hash values of x, y, and z
// 		std::size_t hash = 17;
// 		hash = hash * 31 + std::hash<int>()(position->x);
// 		hash = hash * 31 + std::hash<int>()(position->y);
// 		hash = hash * 31 + std::hash<int>()(position->z);
// 		return hash;
// 	}
// };

// // Equality comparison for Position
// struct PositionEqual {
//     bool operator()(const std::shared_ptr<Position>& lhs, const std::shared_ptr<Position>& rhs) const {
//         return *lhs == *rhs;
//     }
// };
