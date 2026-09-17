#pragma once

#include <vector>
#include <unordered_set>

#include "styles/Styles.hpp"
#include "space/Space.hpp"
#include "space/Planet.h"
#include "server/Interface.hpp"
#include "util/Position.h"
#include "buildings_and_cities/Destination.hpp"
#include "economies/Economies.hpp"
#include "util/StellarCoordinate.hpp"


using namespace std;


enum class BuildingType {
    NONE,
    HOME,
    STORE,
    RESTAURANT,
    OFFICE,
    GOVERNMENT,
    FACTORY,
    PLAY,
    HOTEL,
    RELIGIOUS,
    APARTMENTS,
    PARK
};


// ── City layout data structures ───────────────────────────────────


/* A single waypoint along a road polyline (world x,z coords). */
struct RoadPoint {
    int64_t x, z;
    int64_t y = 0;               // vertical coordinate (for 3D cities)
};

/* A road is a polyline with a width.  Data only — no block geometry. */
struct Road {
    vector<RoadPoint> points;           // waypoints in world coords
    int64_t width;                      // world units
    enum class Type { ARTERIAL, COLLECTOR, LOCAL };
    Type type;
};

/* A rectangular plot bounded by roads, ready for a building. */
struct Plot {
    int64_t x0, z0, x1, z1;            // world bounds (inclusive)
    int64_t y0 = 0, y1 = 0;            // vertical bounds (for 3D cities)
    int64_t road_setback;               // world units offset from road edge
    BuildingType building_type;
    shared_ptr<StyleGroup> style_group;
};

/* Zone classification for a district of the city. */
enum class ZoneType {
    RESIDENTIAL,
    COMMERCIAL,
    INDUSTRIAL,
    CIVIC,
    PARK,
    MIXED
};

/* A city district — a contiguous area with a consistent character. */
struct District {
    int64_t x0, z0, x1, z1;            // world-space bounding box
    int64_t y0 = 0, y1 = 0;            // vertical bounds (for 3D cities)
    ZoneType zone;
    double density;                      // 0 (sparse) – 100 (packed)
    double street_regularity;            // 0 (organic/winding) – 100 (rigid grid)
    int64_t max_building_height;         // world units
    int64_t local_road_spacing;          // world units between local streets
};


struct City : Destination, Economy {

    int64_t max_building_height = 300; // meters
    double building_roundness = 20; // 0-100 roundness of buildings. 100 everything is round all the time


    double smallest_citizens = 2; // meters 
    double largest_citizens = 3; // meters 


    double technological_advancement = 50; // 0 - 100 how advanced technologically 


    double snow_amount = 0; // 0-100 
    double precipitation = 0; // 0-100
    
    unordered_map<BuildingType, vector<shared_ptr<StyleGroup>>> styles;


    // The city either has space location or a location on a planet (or planet orbit)
    shared_ptr<Position> position;
    shared_ptr<StellarCoordinate> space_location;


    // ── City layout (populated by CityGen::plan_city_layout) ──────

    int64_t x0 = 0, z0 = 0;             // city bounding box (world coords)
    int64_t x1 = 0, z1 = 0;
    int64_t y0 = 0, y1 = 0;             // vertical bounds (for 3D cities)

    int city_shape_style = 0;            // 0 = radial, 1 = grid, 2 = organic

    vector<Road> main_roads;             // Phase-1 arterial roads
    vector<District> districts;          // zones broken into districts

    unordered_set<string> generated_chunks; // "x,z" of generated chunks

    // ── Helpers ───────────────────────────────────────────────────

    /* True if this chunk overlaps the city bounding box. */
    bool overlaps_chunk(int64_t chunk_x, int64_t chunk_z, int64_t chunk_size) const {
        int64_t cx0 = chunk_x;
        int64_t cz0 = chunk_z;
        int64_t cx1 = chunk_x + chunk_size - 1;
        int64_t cz1 = chunk_z + chunk_size - 1;
        return cx0 <= x1 && cx1 >= x0 && cz0 <= z1 && cz1 >= z0;
    }

    /* Returns the district containing (x, z), or nullptr. */
    const District* district_at(int64_t x, int64_t z) const {
        for (const auto& d : districts) {
            if (x >= d.x0 && x <= d.x1 && z >= d.z0 && z <= d.z1) {
                return &d;
            }
        }
        return nullptr;
    }
    District* district_at(int64_t x, int64_t z) {
        return const_cast<District*>(
            const_cast<const City*>(this)->district_at(x, z));
    }

};