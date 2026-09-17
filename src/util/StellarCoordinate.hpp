#pragma once

#include <sstream>
#include "util/Random.h"

using namespace std;


/**
 * A absolute position in the universe
 * (quadrant x):(quadrant y):(quadrant z) :: (light year x):(light year y):(light year z) :: (km x):(km y):(km z)
 * - quadrant range is -1,321,122 to 1,321,122
 * - light year range is 0 to 10,000
 * - km range 0 to 9,460,730,472,581
 * 
 * So a quandrent is 10,000 light-years accross, and a light year is 9,460,730,472,581 km accross
 * 
 * And our universe would be 26,422,450,000 light years square.
 * 
 * But I think after you get outside these coordinates we'll just mod your coordinates to get the seed.
 * That'll give a bigger universe even though it will actually repeat. So that's be a range of 
 * 18,446,744,073,709,551,616 quadrants, or 184,467,440,737,095,516,160,000 light years. So 1,983,520,868,140
 * or ~2 trillion times the size of of the real life observable universe (93 billion lyr estimate, though the 
 * actual universe is likely to be bigger).  
 */
struct StellarCoordinate {
    int64_t quadrant_x;
    int64_t quadrant_y;
    int64_t quadrant_z;
    uint16_t light_year_x;
    uint16_t light_year_y;
    uint16_t light_year_z;
    uint64_t km_x;
    uint64_t km_y;
    uint64_t km_z;

    StellarCoordinate() : quadrant_x(0), quadrant_y(0), quadrant_z(0),
        light_year_x(0), light_year_y(0), light_year_z(0),
        km_x(0), km_y(0), km_z(0) {}
    StellarCoordinate(
        int64_t quadrant_x, 
        int64_t quadrant_y, 
        int64_t quadrant_z, 
        uint16_t light_year_x, 
        uint16_t light_year_y, 
        uint16_t light_year_z, 
        uint64_t km_x, 
        uint64_t km_y, 
        uint64_t km_z
    ) : 
        quadrant_x(quadrant_x), 
        quadrant_y(quadrant_y), 
        quadrant_z(quadrant_z), 
        light_year_x(light_year_x), 
        light_year_y(light_year_y), 
        light_year_z(light_year_z), 
        km_x(km_x), 
        km_y(km_y), 
        km_z(km_z) 
    {}


    string to_string() {
        stringstream ss;
        ss << quadrant_x << ':' << quadrant_y << ':' << quadrant_z << " :: ";
        ss << light_year_x << ':' << light_year_y << ':' << light_year_z << " :: ";
        ss << km_x << ':' << km_y << ':' << km_z;

        return ss.str();
    }


    /*
        THIS METHOD ONLY GIVES YOU QUADRANT COORDINATES
        When the user provides a seed we calculate the quadrant from that and generate the quadrant. We then let them choose
        a light year (solar system) and a planet to determine the other portions of the steller coordinate. 


        quadrant coordinates can be calculated from a seed with this equation (Note maxSeed - minSeed needs to be odd, so subtract 1 if need be)

        span = (maxSeed - minSeed)^(1/3)
        half = floor(span / 2)
        max = half - 1
        min = -half
        Which for our square cube 2,642,245 quadrants would be

        span = 2,642,245
        half = 1,321,122
        max = 1,321,122
        min = -1,321,122
        midSub = span * half + half
        if seed > 0
            y = floor( (seed + midSub) / span^2 )
            adj = (seed > midSub)? (seed - midSub - 1) % span^2 : seed + half
            x = (adj % span) - half
            zSub = (seed > midSub)? half : 0
            z = floor(adj / span) - zSub
        else
            y = floor( (seed - midSub) / span^2 )
            adj = (seed < -midSub)? (seed + midSub + 1) % span^2 : seed - half
            x = (adj % span) + half
            zSub = (seed < -midSub)? half : 0
            z = floor(adj / span) + zSub
        To get the seed from the coordinates use the following equation:

            midSub = span * half + half
            if y > 0
            x + half + (z + half) * span + (y - 1) * span^2 + midSub + 1
            elif y == 0
            x + z * span
            else
            x - half + (z - half) * -span + (y + 1) * span^2 - midSub - 1
        This means the center quadrant (0:0:0) would be seed:

        0 + 0 * 2,642,245 = 0
        Makes Sense!

        The max cooridinates (1,321,122:1,321,122:1,321,122) and (-1,321,122:-1,321,122:-1,321,122) would be seeds:

        midSub = 2,642,245 * 1,321,122 + 1,321,122 = 3,490,729,320,012

        1,321,122 + 1,321,122 + (1,321,122 + 1,321,122) * 2,642,245 + (1,321,122 - 1) * 2,642,245^2 + midSub + 1 = 2,642,244 + 6,981,453,355,536 + 9,223,351,619,968,468,025 + 3,490,729,320,012 + 1 = 9,223,362,092,153,785,818

        -1,321,122 - 1,321,122 + (-1,321,122 - 1,321,122) * 2,642,245 + (-1,321,122 + 1) * 2,642,245^2 - midSub - 1 = -2,642,244 - 6,981,453,355,536 - 9,223,351,619,968,468,025 - 3,490,729,320,012 - 1 = -9,223,362,092,153,785,818

        So these are a little less than our max number of seeds (because of int64_t overflow)
    
    */
    static StellarCoordinate from_seed_old(int64_t seed) {
        int64_t span = 2642245; // how many quadrants across the universe is
        int64_t span_squared = span * span; // quadrants in a slice of the universe
        int64_t half = 1321122;
        int64_t max_quadrant = 1321122;
        int64_t min_quadrant = -1321122;
        int64_t mid_sub = span * half + half; // the point 

        int64_t quadrant_x, quadrant_y, quadrant_z;
        if (seed > 0) {
            quadrant_y = floor(seed + mid_sub) / span_squared;
            int64_t adj = seed > mid_sub? (seed - mid_sub - 1) % span_squared : seed + half;
            quadrant_x = (adj % span) - half;
            int64_t z_sub = seed > mid_sub? half : 0;
            quadrant_z = floor(adj/span) - z_sub;
        }
        else {
            quadrant_y = floor((seed - mid_sub) / span_squared);
            int64_t adj = seed < -mid_sub ? (seed + mid_sub + 1) % span_squared : seed - half;
            quadrant_x = (adj % span) + half;
            int64_t z_sub = seed < - mid_sub? half : 0;
            quadrant_z = floor(adj/span) + z_sub;
        }


        StellarCoordinate location;
        location.quadrant_x = quadrant_x;
        location.quadrant_y = quadrant_y;
        location.quadrant_z = quadrant_z;
        location.light_year_x = 0;
        location.light_year_y = 0;
        location.light_year_z = 0;
        location.km_x = 0;
        location.km_y = 0;
        location.km_z = 0;

        return location;
    }

    static int64_t seed_from_coordinates_old(const StellarCoordinate& c) {
        int64_t span = 2642245;
        int64_t span_squared = span * span;
        int64_t half = 1321122;
        int64_t mid_sub = span * half + half;

        int64_t x = c.quadrant_x;
        int64_t y = c.quadrant_y;
        int64_t z = c.quadrant_z;

        if (y > 0) {
            return x + half + (z + half) * span + (y - 1) * span_squared + mid_sub + 1;
        } else if (y == 0) {
            return x + z * span;
        } else {
            return x - half + (z - half) * -span + (y + 1) * span_squared - mid_sub - 1;
        }
    }

    int64_t to_seed() {
        int64_t h = 0;
        auto combine = [&h](int64_t v) {
            unsigned int rot = abs(h) % 64;
            h ^= Random::rotl64(v, rot);
        };
        combine(quadrant_x);
        combine(quadrant_y);
        combine(quadrant_z);
        combine(light_year_x);
        combine(light_year_y);
        combine(light_year_z);
        combine(km_x);
        combine(km_y);
        combine(km_z);
        return Random::adjustedSeed(h);
    }

    friend bool operator==(const StellarCoordinate& a, const StellarCoordinate& b) {
        return a.quadrant_x == b.quadrant_x && a.quadrant_y == b.quadrant_y && a.quadrant_z == b.quadrant_z
            && a.light_year_x == b.light_year_x && a.light_year_y == b.light_year_y && a.light_year_z == b.light_year_z
            && a.km_x == b.km_x && a.km_y == b.km_y && a.km_z == b.km_z;
    }

    size_t hash() const {
        size_t h = 0;
        auto combine = [](size_t& seed, size_t v) {
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
        combine(h, std::hash<int64_t>()(quadrant_x));
        combine(h, std::hash<int64_t>()(quadrant_y));
        combine(h, std::hash<int64_t>()(quadrant_z));
        combine(h, std::hash<uint16_t>()(light_year_x));
        combine(h, std::hash<uint16_t>()(light_year_y));
        combine(h, std::hash<uint16_t>()(light_year_z));
        combine(h, std::hash<uint64_t>()(km_x));
        combine(h, std::hash<uint64_t>()(km_y));
        combine(h, std::hash<uint64_t>()(km_z));
        return h;
    }

};

struct StellarCoordinateHash {
    size_t operator()(const StellarCoordinate& c) const {
        return c.hash();
    }
};



template<>
struct std::hash<StellarCoordinate> {
    size_t operator()(const StellarCoordinate& c) const {
        return c.hash();
    }
};