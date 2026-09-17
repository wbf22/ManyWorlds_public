#pragma once


using namespace std;



#include "server/Interface.hpp"
#include "Planet.h"
#include "Time.hpp"
#include "util/Grid.hpp"
#include "util/SpaceGrid.hpp"
#include "economies/Economies.hpp"
#include "util/StellarCoordinate.hpp"

using namespace std;



struct Star;
struct AsteriodBelt;
struct SolarSystem;
struct StellarCradle;
struct SuperCluster;
struct Cluster;
struct Nebula;
struct Galaxy;
struct Chamber;
struct Cosmic_River;
struct Continuum;
struct Universe;




// COSMIC ENTITIES
struct BlackHole {

};

struct Star {

    enum class StarType {
        RED_DWARF,
        ORANGE,
        YELLOW,
        RED_GIANT,
        WHITE_DWARF,
        BLACK_DWARF,
        BLUE_GIANT,
        WHITE_GIANT,
        PURPLE_GIANT,
        NEUTRON_STAR,
        BLACK_HOLE,
    };


    /*
    0.08–0.5 = Red dwarfs
    0.5–1.5 = Sun-like
    1.5–8 = Bright white stars
    8+ = Massive blue stars
    */
    double mass; // solar masses
    double age; // billion years
    int64_t metallicity; // 0-100
    int64_t rotation_speed; // 0-100
    double radius_ly = 0; // glow radius in light-years (derived from mass/type)

    /*
    red dwarf - 30% chance
    yellow - 50% chance
    massive stars - 80% chance
    */
    int64_t multiplicity; // 1–3+ stars

    StarType type; // calculated from attributes

    shared_ptr<StellarCoordinate> location;

    std::weak_ptr<SolarSystem> parent; // owning solar system

    /*
    Computed properties

    Color = f(mass, age)
    Temperature = f(mass)
    Luminosity = f(mass)
    Habitable zone = f(luminosity)
    Expected lifespan = f(mass)

    Paths
    - red dwarf: Main Sequence -> white dwarf
    - medium stars (some red, orange/yellow): Main sequence → Red giant → White dwarf → (eventually black dwarf, theoretically)
    - high mass (blue and white): Main sequence → Supergiant → Supernova → Neutron star or Black hole
    */

};

struct AsteriodBelt {
    int64_t orbit_distance; // km
    int64_t width; // km
    int64_t density; // 0-100 amount per 10 million km (sizable asteriods are typically 1 million km apart)
    int64_t vertical_thickness; // km
    
};

struct SolarSystem : Economy {

    vector<shared_ptr<Star>> stars; // usually 1 but 0–3+ (0 would represent rogue planets)
    vector<shared_ptr<Planet>> planets;
    vector<shared_ptr<AsteriodBelt>> asteriodBelts;
    shared_ptr<StellarCoordinate> location;
    double mass; // solar masses
    double radius_ly = 0; // approximate extent of the system in light-years

    std::weak_ptr<StellarCradle> parent; // owning stellar cradle

    vector<shared_ptr<Economy>> get_child_economies() override {
        return vector<shared_ptr<Economy>>(planets.begin(), planets.end());
    }
};

struct Nebula {
    // One point of the nebula's filament skeleton (a main strand point or a
    // sub-strand point) plus the soft cloud that surrounds it. Block-gen turns
    // each point into `n` blocks spread over a disk of radius `cloud_size`,
    // `n` scaling with cloud_size/block_size so LOD is built in.
    struct FilamentPoint {
        int64_t x = 0, y = 0, z = 0; // LY offsets from the nebula center
        float cloud_size = 0.1f;     // cloud radius around this point (x radius)
    };

    int64_t generation_seed;
    double radius_ly = 0; // physical extent of the nebula in light-years
    shared_ptr<StellarCoordinate> center; // nebula center for per-nebula sky placement
    std::weak_ptr<Galaxy> parent; // owning galaxy
    // Base colour / brightness so block-gen can shade blocks deterministically
    float base_r = 0.5f, base_g = 0.3f, base_b = 0.3f;
    float neb_dark = 1.0f;
    double fax = 1, fay = 0, faz = 0;
    double fbx = 0, fby = 1, fbz = 0;
    double fcx = 0, fcy = 0, fcz = 1;
    // Strand spec: meandering filament centerlines (stored in `strand_paths` as
    // int64 LY offsets) with short sub-strand wisps branching off each path
    // point. The full skeleton is flattened into `filament_points`, each with a
    // per-point cloud size, so far/near LOD render the same shape. Per-nebula
    // constants (from the nebula seed) so block-gen stays LOD-consistent.
    int strands = 2;         // number of parallel strands in the bundle
    double along_extent = 1; // strand length along the axis (x radius)
    double meander_amp = 0.8;   // how far strands bend off-axis (x radius)
    int meander_knots = 16;     // resolution of each stored strand centerline
    int64_t wavy_seed = 0;      // seed for the meander walk (per-nebula, LOD-consistent)
    vector<vector<array<int64_t, 3>>> strand_paths; // per-strand centerlines ({x,y,z} LY offsets)
    vector<FilamentPoint> filament_points; // full skeleton (path + sub-strand points) + cloud sizes
};

/**
 * StellarCradle
 * - a region with 1 - 1000 stars
 */
struct StellarCradle : Economy {
    vector<shared_ptr<SolarSystem>> solar_systems;
    shared_ptr<StellarCoordinate> location;
    double mass; // solar masses
    double radius_ly = 0; // approximate physical radius in light-years
    bool has_massive_stars = false; // will spawn giants/blues (bright spot at cluster LOD)
    std::weak_ptr<Cluster> parent; // owning cluster
};

/**
 * Cluster
 * - contains 1 - 1 million stars
 * - each contains 1 - 1000 stellar cradles
 */
struct Cluster : Economy {
    vector<shared_ptr<StellarCradle>> stellar_cradles;
    shared_ptr<StellarCoordinate> location;
    double mass; // solar masses
    double radius_ly = 0; // approximate physical radius in light-years
    double gas_fraction = 0; // fraction of mass that is gas / star-forming fuel
    double massive_star_fraction = 0; // fraction of cradles hosting massive stars (bright spots)
    std::weak_ptr<SuperCluster> parent; // owning super cluster
};

/**
 * SuperCluster
 * - contains 1 - 1 billion stars
 * - or 0 - 300 millino solar masses
 * - each contains 1 - 1000 clusters
 */
struct SuperCluster : Economy {
	Grid<Grid<shared_ptr<Cluster>>> clusters; // quadrant xyz, light year xyz, to cluster if present 
    shared_ptr<StellarCoordinate> location;
    double mass; // solar masses
    double radius_ly = 0; // approximate physical radius in light-years
    double gas_fraction = 0; // fraction of mass that is gas / star-forming fuel
    std::weak_ptr<Galaxy> parent; // owning galaxy
};

/**
 * Galaxy
 * - 1k - 6 million light years in size
 * - 1-10 million light years between galaxies
 * - the milky way is 1,500,000,000,000 solar masses
 * - ~200 billion stars in the milky way
 * - the milky way would have 400 super clusters
 * 
 */
struct Galaxy : Economy {
	/*
	Typically evolution:
	cloud -> spiral -> faded spiral
	                -> eliptical if collision happens

	But with crazy collisions other shapes can happen
	*/
	enum class GalaxyType {
		CLOUD, // disorganized cloud. (new or recently disrupted galaxy, very active)
		SPIRAL, //. bulge in middle, spiral arms, sometimes bar through center. (active uncollided galaxy rich in gas)
		ELIPTICAL, // spherical to football shaped,, typically galaxy merger. (Old and spent, with lots of collisions or gravity influences in the past)
		
		RING, // ring around an empty center. 
		RANDOM, // collisions can cause random twisty shapes to appear
	};
	GalaxyType type;
	double angular_velocity; // radians per aeon (2.23 billion years)
	int64_t mass; // solar masses
	double age; // aeons or (2.23 billion years), most are 10-13 billion years old, basically as old as the universe
	shared_ptr<StellarCoordinate> location;

	// Shape parameters for generation
	int arm_count = 0; // 0 for elliptical/cloud
	double arm_winding = 0.5; // 0=loose, 1=tight
	double bar_length = 0.0; // fraction of disk radius, 0 = no bar
	double bar_direction_ly[3] = {}; // bar axis (LY), magnitude = bar_length * scale_length
	double scale_length = 15000.0; // light years
	double radius_ly = 0; // outer disc radius in light-years (scale_length * 2.5)
	double core_mass = 1e8; // central black hole mass in solar masses
	double gas_fraction = 0; // fraction of mass that is gas / star-forming fuel

	// 3 reference points defining the galactic disc plane
	// (light-year offsets from galaxy center).
	// Two of these define the orientation of the long axis;
	// the third can be on the disc rim for a triangular reference.
	double orient_a_ly[3] = {};
	double orient_b_ly[3] = {};
	double orient_c_ly[3] = {};

	// Visual color
	double color_r = 0.7;
	double color_g = 0.5;
	double color_b = 0.3;

    Grid<shared_ptr<SuperCluster>> super_clusters;
    vector<shared_ptr<Nebula>> nebulae;
    std::weak_ptr<Chamber> parent; // owning chamber
};

/**
 * 
 * Cosmic Filaments
 * - lengths range from 100 million - 10 billion light years
 */
struct Cosmic_River {
    vector<shared_ptr<StellarCoordinate>> path_points;
    int64_t min_qx = 0, min_qy = 0, min_qz = 0;
    int64_t max_qx = 0, max_qy = 0, max_qz = 0;
};


/**
 * Chamber:
 * - 100 million lightyears square
 * - all galaxies will be placed within chambers and won't overlap 2 chambers
 */
struct Chamber : Economy {
    Grid<shared_ptr<Galaxy>> galaxies; // quadrant xyz to galaxy if present. A chamber could have up to 10,000 but usually around 300
    vector<shared_ptr<Cosmic_River>> cosmic_rivers;
};


/**
 * Cotinuum:
 * - 1 billion lightyears square
 * - Just contains cosmic rivers
 */
struct Continuum : Economy {
    vector<shared_ptr<Cosmic_River>> cosmic_rivers;
};


/**
 * The biggest structure. A cosmic web about 26 billion light years across. 
 * As you near the edge of it we'll just wrap your coordinates and generate the same universe again.
 * 
 */
struct Universe {
    vector<shared_ptr<Cosmic_River>> main_cosmic_rivers;
    Grid<shared_ptr<Continuum>> continuums;
    Grid<shared_ptr<Chamber>> chambers;

    // ─── Spatial constants ────────────────────────────────────────
    // A quadrant is 10 000 ly across.
    static constexpr int64_t QUADRANTS_PER_CONTINUUM = 100000; // 1B ly
    static constexpr int64_t QUADRANTS_PER_CHAMBER   = 10000;  // 100M ly

    // ─── Integer math helpers ─────────────────────────────────────

    static int64_t floor_div(int64_t a, int64_t b) {
        int64_t q = a / b;
        int64_t r = a % b;
        if (r != 0 && ((a ^ b) < 0)) q--;
        return q;
    }

    // ─── Grid-key helpers ─────────────────────────────────────────
    //
    // Continuum and chamber membership is a pure function of quadrant
    // coordinates, so their Grid keys can be computed in O(1).

    /// Grid key for the continuum containing `pos`.
    static Index3 continuum_key(shared_ptr<StellarCoordinate> pos) {
        return Index3{
            floor_div(pos->quadrant_x, QUADRANTS_PER_CONTINUUM),
            floor_div(pos->quadrant_y, QUADRANTS_PER_CONTINUUM),
            floor_div(pos->quadrant_z, QUADRANTS_PER_CONTINUUM),
        };
    }

    /// Grid key for the chamber containing `pos`.
    static Index3 chamber_key(shared_ptr<StellarCoordinate> pos) {
        return Index3{
            floor_div(pos->quadrant_x, QUADRANTS_PER_CHAMBER),
            floor_div(pos->quadrant_y, QUADRANTS_PER_CHAMBER),
            floor_div(pos->quadrant_z, QUADRANTS_PER_CHAMBER),
        };
    }

    /// Minimum quadrant coordinate of the continuum containing `q`.
    static int64_t continuum_base_q(int64_t q) {
        return floor_div(q, QUADRANTS_PER_CONTINUUM) * QUADRANTS_PER_CONTINUUM;
    }

    /// Minimum quadrant coordinate of the chamber (within its
    /// continuum) containing `q`.
    static int64_t chamber_base_q(int64_t q) {
        int64_t cb = continuum_base_q(q);
        return cb + floor_div(q - cb, QUADRANTS_PER_CHAMBER) * QUADRANTS_PER_CHAMBER;
    }

    /// Populate `out` with the quadrant-origin of the continuum
    /// that contains `pos`.
    static void continuum_origin(shared_ptr<StellarCoordinate> pos,
                                  int64_t& ox, int64_t& oy, int64_t& oz) {
        ox = continuum_base_q(pos->quadrant_x);
        oy = continuum_base_q(pos->quadrant_y);
        oz = continuum_base_q(pos->quadrant_z);
    }

    /// Populate `out` with the quadrant-origin of the chamber
    /// (within its continuum) that contains `pos`.
    static void chamber_origin(shared_ptr<StellarCoordinate> pos,
                                int64_t& ox, int64_t& oy, int64_t& oz) {
        ox = chamber_base_q(pos->quadrant_x);
        oy = chamber_base_q(pos->quadrant_y);
        oz = chamber_base_q(pos->quadrant_z);
    }

    // ─── Object lookups ───────────────────────────────────────────
    //
    // All lookups are O(1) Grid key computations.

    /// The continuum containing `pos` (O(1) Grid lookup).
    shared_ptr<Continuum> continuum_at(shared_ptr<StellarCoordinate> pos) const {
        Index3 k = continuum_key(pos);
        return continuums.has(k) ? continuums[k] : nullptr;
    }

    /// The chamber containing `pos` (O(1) Grid lookup).
    shared_ptr<Chamber> chamber_at(shared_ptr<StellarCoordinate> pos) const {
        Index3 k = chamber_key(pos);
        return chambers.has(k) ? chambers[k] : nullptr;
    }

    /// The galaxy at `pos`'s quadrant (O(1) Grid lookup).
    shared_ptr<Galaxy> galaxy_at(shared_ptr<StellarCoordinate> pos) const {
        auto ch = chamber_at(pos);
        if (!ch) return nullptr;
        Index3 k{pos->quadrant_x, pos->quadrant_y, pos->quadrant_z};
        return ch->galaxies.has(k) ? ch->galaxies[k] : nullptr;
    }

    /// The super cluster at `pos`'s quadrant (O(1) Grid lookup).
    shared_ptr<SuperCluster> super_cluster_at(shared_ptr<StellarCoordinate> pos) const {
        auto g = galaxy_at(pos);
        if (!g) return nullptr;
        Index3 qk{pos->quadrant_x, pos->quadrant_y, pos->quadrant_z};
        return g->super_clusters.has(qk) ? g->super_clusters[qk] : nullptr;
    }

    /// The cluster at `pos`'s exact position (O(1) Grid lookup).
    shared_ptr<Cluster> cluster_at(shared_ptr<StellarCoordinate> pos) const {
        auto sc = super_cluster_at(pos);
        if (!sc) return nullptr;
        Index3 qk{pos->quadrant_x, pos->quadrant_y, pos->quadrant_z};
        if (!sc->clusters.has(qk)) return nullptr;
        Index3 lyk{pos->light_year_x, pos->light_year_y, pos->light_year_z};
        auto& inner = sc->clusters[qk];
        return inner.has(lyk) ? inner[lyk] : nullptr;
    }
};
