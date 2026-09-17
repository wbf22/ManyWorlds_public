#pragma once


#include "space/Planet.h"
#include "npcs/AlienGen.hpp"
#include "City.hpp"
#include "space/Space.hpp"
#include "../blocks/Block.h"
#include "../chunks/Chunk.h"
#include "util/Random.h"
#include "util/BMaterial.hpp"


using namespace std;


struct CityGen {

    // ═══════════════════════════════════════════════════════════════
    //  Phase 1 — City Planning (generated once)
    // ═══════════════════════════════════════════════════════════════

    /* Plan the macro layout of the city: choose shape style, place
       main roads, assign districts.  Called once when the city is
       first visited. */
    static void plan_city_layout(
        int64_t seed,
        int64_t spike,
        shared_ptr<City> city,
        shared_ptr<Planet> planet
    ) {
        static constexpr double PI = 3.14159265358979323846;
        int64_t loc_seed = seed + spike + 101;

        auto get_ground = [&](int64_t sx, int64_t sz) -> int64_t {
            auto chunk = planet->rootChunk->getChunkOrClosestAncestorInitialized(
                ChunkType::CHUNK_1, sx, sz);
            return chunk ? chunk->position->y : 0;
        };

        // ── 1. PICK LOCATION ─────────────────────────────────────
        int64_t cx = 0, cz = 0;
        int64_t ground_y = 0;
        int64_t planet_span = planet->worldSize /
            CoordinateConversion::BLOCK_SCALE;
        int64_t half_span = planet_span / 2;
        int found = 0;
        for (int attempt = 0; attempt < 30; attempt++) {
            cx = Random::randInt(loc_seed + attempt, -half_span, half_span);
            cz = Random::randInt(loc_seed + 1000 + attempt,
                                 -half_span, half_span);
            ground_y = get_ground(cx, cz);
            if (ground_y < planet->seaLevel + 5) continue;

            int64_t probe = 50;
            int64_t h[] = {
                get_ground(cx - probe, cz - probe),
                get_ground(cx + probe, cz - probe),
                get_ground(cx - probe, cz + probe),
                get_ground(cx + probe, cz + probe),
                get_ground(cx, cz)
            };
            int64_t mn = h[0], mx = h[0];
            for (auto v : h) {
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            if (mx - mn > 15) continue;

            found = 1;
            break;
        }
        if (!found) ground_y = get_ground(cx, cz);

        // ── 2. CITY SIZE ─────────────────────────────────────────
        double wealth = city->economy ? city->economy->cash : 1000.0;
        int64_t base_size = static_cast<int64_t>(sqrt(wealth) * 10.0);
        base_size = max<int64_t>(300, min<int64_t>(3000, base_size));
        if (Random::randInt(loc_seed + 50, 0, 100) > 50) {
            double growth = Random::randDouble(loc_seed + 51, 1.0, 1.0, 1.5);
            base_size = static_cast<int64_t>(base_size * growth);
        }

        // ── 3. CITY SHAPE ────────────────────────────────────────
        int shape_r = Random::randInt(loc_seed + 99, 0, 99);
        if (shape_r < 40)      city->city_shape_style = 0; // radial
        else if (shape_r < 80) city->city_shape_style = 1; // grid
        else                   city->city_shape_style = 2; // organic

        // ── 4. SET CITY BOUNDING BOX ─────────────────────────────
        int64_t half = base_size / 2;
        city->x0 = cx - half;
        city->z0 = cz - half;
        city->x1 = cx + half;
        city->z1 = cz + half;
        city->y0 = ground_y;
        city->y1 = ground_y + city->max_building_height;

        // ── 5. GENERATE MAIN ROADS ───────────────────────────────
        city->main_roads.clear();

        auto add_road = [&](vector<RoadPoint> pts, int64_t w,
                            Road::Type t) {
            Road r;
            r.points = std::move(pts);
            r.width = w;
            r.type = t;
            city->main_roads.push_back(std::move(r));
        };

        if (city->city_shape_style == 0) {
            // RADIAL — concentric rings + spoke roads
            int n_rings = Random::randInt(loc_seed + 10, 2, 5);
            int n_spokes = Random::randInt(loc_seed + 11, 4, 8);
            for (int i = 0; i < n_rings; i++) {
                int64_t radius = half * (i + 1) / n_rings;
                int segments = max(8, n_spokes * 2);
                vector<RoadPoint> pts;
                for (int j = 0; j <= segments; j++) {
                    double a = 2.0 * PI * j / segments;
                    pts.push_back({
                        cx + static_cast<int64_t>(radius * cos(a)),
                        cz + static_cast<int64_t>(radius * sin(a))
                    });
                }
                auto rt = (i == 0) ? Road::Type::ARTERIAL
                                   : Road::Type::COLLECTOR;
                add_road(pts, (rt == Road::Type::ARTERIAL) ? 10 : 6, rt);
            }
            for (int i = 0; i < n_spokes; i++) {
                double a = 2.0 * PI * i / n_spokes;
                int64_t ex = cx + static_cast<int64_t>(half * cos(a));
                int64_t ez = cz + static_cast<int64_t>(half * sin(a));
                add_road({{cx, cz}, {ex, ez}}, 10, Road::Type::ARTERIAL);
            }
        } else if (city->city_shape_style == 1) {
            // GRID — N-S and E-W arterials
            int64_t spacing = base_size / 3;
            for (int64_t x = cx - half + spacing;
                 x < cx + half; x += spacing) {
                add_road({{x, cz - half}, {x, cz + half}},
                         10, Road::Type::ARTERIAL);
            }
            for (int64_t z = cz - half + spacing;
                 z < cz + half; z += spacing) {
                add_road({{cx - half, z}, {cx + half, z}},
                         10, Road::Type::ARTERIAL);
            }
            if (Random::randBool(loc_seed + 60)) {
                add_road({{cx - half, cz - half},
                          {cx + half, cz + half}},
                         8, Road::Type::COLLECTOR);
            }
        } else {
            // ORGANIC — main roads from center + branches
            int n_main = Random::randInt(loc_seed + 20, 2, 4);
            for (int i = 0; i < n_main; i++) {
                double a = Random::randDouble(loc_seed + 30 + i,
                                              0.0, 2.0 * PI);
                double len = half * Random::randDouble(
                    loc_seed + 40 + i, 0.6, 1.0);
                int64_t ex = cx + static_cast<int64_t>(len * cos(a));
                int64_t ez = cz + static_cast<int64_t>(len * sin(a));
                add_road({{cx, cz}, {ex, ez}}, 10, Road::Type::ARTERIAL);
                // branch at midpoint
                double ba = a + Random::randDouble(
                    loc_seed + 50 + i, -0.5, 0.5);
                int64_t mx = (cx + ex) / 2;
                int64_t mz = (cz + ez) / 2;
                int64_t bx = mx + static_cast<int64_t>(
                    len * 0.4 * cos(ba));
                int64_t bz = mz + static_cast<int64_t>(
                    len * 0.4 * sin(ba));
                add_road({{mx, mz}, {bx, bz}}, 6, Road::Type::COLLECTOR);
            }
        }

        // ── 6. ASSIGN DISTRICTS ─────────────────────────────────
        city->districts.clear();

        auto add_district = [&](int64_t x0, int64_t z0,
                                int64_t x1, int64_t z1,
                                ZoneType z, double d, double reg,
                                int64_t mh) {
            District dist;
            dist.x0 = x0; dist.z0 = z0;
            dist.x1 = x1; dist.z1 = z1;
            dist.y0 = ground_y;
            dist.y1 = ground_y + city->max_building_height;
            dist.zone = z;
            dist.density = d;
            dist.street_regularity = reg;
            dist.max_building_height = mh;
            dist.local_road_spacing = 40;
            city->districts.push_back(dist);
        };

        // Civic at center
        int64_t ch = half / 4;
        add_district(cx - ch, cz - ch, cx + ch, cz + ch,
                     ZoneType::CIVIC, 70.0, 80.0,
                     city->max_building_height);

        // Commercial ring around civic
        int64_t cr = half / 2;
        int64_t cw = half / 6;
        add_district(cx - cr - cw, cz - cr - cw,
                     cx - cr + cw, cz + cr + cw,
                     ZoneType::COMMERCIAL, 60.0, 70.0,
                     city->max_building_height);
        add_district(cx + cr - cw, cz - cr - cw,
                     cx + cr + cw, cz + cr + cw,
                     ZoneType::COMMERCIAL, 60.0, 70.0,
                     city->max_building_height);
        add_district(cx - cr - cw, cz - cr - cw,
                     cx + cr + cw, cz - cr + cw,
                     ZoneType::COMMERCIAL, 60.0, 70.0,
                     city->max_building_height);
        add_district(cx - cr - cw, cz + cr - cw,
                     cx + cr + cw, cz + cr + cw,
                     ZoneType::COMMERCIAL, 60.0, 70.0,
                     city->max_building_height);

        // Industrial at south-east edge
        add_district(cx + half / 2, cz + half / 2,
                     cx + half, cz + half,
                     ZoneType::INDUSTRIAL, 40.0, 50.0,
                     city->max_building_height * 3 / 4);

        // Residential — remaining quadrants
        add_district(cx - half, cz - half,
                     cx + half, cz - cr - cw,
                     ZoneType::RESIDENTIAL, 50.0, 60.0,
                     city->max_building_height);
        add_district(cx - half, cz + cr + cw,
                     cx + half, cz + half,
                     ZoneType::RESIDENTIAL, 50.0, 60.0,
                     city->max_building_height);
        add_district(cx - half, cz - cr - cw,
                     cx - cr - cw, cz + cr + cw,
                     ZoneType::RESIDENTIAL, 50.0, 60.0,
                     city->max_building_height);
        add_district(cx + cr + cw, cz - cr - cw,
                     cx + half / 2, cz + cr + cw,
                     ZoneType::RESIDENTIAL, 50.0, 60.0,
                     city->max_building_height);

        // Parks — random pockets
        int n_parks = Random::randInt(loc_seed + 70, 2, 5);
        for (int i = 0; i < n_parks; i++) {
            int64_t px = Random::randInt(loc_seed + 80 + i,
                                         city->x0, city->x1);
            int64_t pz = Random::randInt(loc_seed + 90 + i,
                                         city->z0, city->z1);
            int64_t pw = Random::randInt(loc_seed + 100 + i, 30, 120);
            int64_t pd = Random::randInt(loc_seed + 110 + i, 30, 120);
            add_district(px, pz, px + pw, pz + pd,
                         ZoneType::PARK, 10.0, 100.0, 10);
        }
    }


    // ═══════════════════════════════════════════════════════════════
    //  Phase 2 — Per-Chunk Generation (streamed on demand)
    // ═══════════════════════════════════════════════════════════════

    /* Generate buildings for a single chunk that overlaps the city.
       Returns a flat vector of world-space Block objects ready for
       WorldBlockPool::spawn_blocks.

       chunk_x, chunk_z = chunk-aligned position (from
       Chunk::get_coordinates_by_chunk_type). */
    static vector<shared_ptr<Block>> generate_chunk_buildings(
        int64_t seed,
        int64_t chunk_x,
        int64_t chunk_z,
        int64_t chunk_size,
        shared_ptr<City> city,
        shared_ptr<Planet> planet
    ) {
        vector<shared_ptr<Block>> result;

        // ── 1. CHECK OVERLAP ────────────────────────────────────
        if (!city->overlaps_chunk(chunk_x, chunk_z, chunk_size))
            return result;

        string chunk_key = to_string(chunk_x) + "," + to_string(chunk_z);
        if (city->generated_chunks.count(chunk_key))
            return result;

        // ── 2. FIND DISTRICT ────────────────────────────────────
        District default_dist;
        const District* dist = city->district_at(
            chunk_x + chunk_size / 2, chunk_z + chunk_size / 2);
        if (!dist) {
            default_dist.x0 = city->x0; default_dist.z0 = city->z0;
            default_dist.x1 = city->x1; default_dist.z1 = city->z1;
            default_dist.y0 = city->y0; default_dist.y1 = city->y1;
            default_dist.zone = ZoneType::RESIDENTIAL;
            default_dist.density = 50.0;
            default_dist.street_regularity = 60.0;
            default_dist.max_building_height = city->max_building_height;
            default_dist.local_road_spacing = 40;
            dist = &default_dist;
        }

        // ── 3. LOCAL ROADS & PLOTS ──────────────────────────────
        vector<Plot> plots = generate_local_roads_and_plots(
            seed, chunk_x, chunk_z, chunk_size, *dist, city->main_roads);

        auto get_ground = [&](int64_t sx, int64_t sz) -> int64_t {
            auto ch = planet->rootChunk->getChunkOrClosestAncestorInitialized(
                ChunkType::CHUNK_1, sx, sz);
            return ch ? ch->position->y : 0;
        };

        // ── 4. PROCESS EACH PLOT ────────────────────────────────
        for (const auto& plot : plots) {
            int64_t plot_w = plot.x1 - plot.x0 - 2 * plot.road_setback;
            int64_t plot_d = plot.z1 - plot.z0 - 2 * plot.road_setback;
            if (plot_w <= 0 || plot_d <= 0) continue;

            int64_t plot_x = plot.x0 + plot.road_setback;
            int64_t plot_z = plot.z0 + plot.road_setback;

            // a. BUILDING TYPE
            BuildingType btype = pick_building_type(
                dist->zone, seed + plot_x + plot_z, dist->density);
            if (btype == BuildingType::NONE || btype == BuildingType::PARK)
                continue;

            // b. TERRAIN CHECK
            if (is_steep_slope(plot_x, plot_z, plot_w, plot_d,
                               dist->density, planet->rootChunk))
                continue;

            // c. FIND HIGHEST GROUND (for fill target / y offset)
            int64_t h[] = {
                get_ground(plot_x, plot_z),
                get_ground(plot_x + plot_w, plot_z),
                get_ground(plot_x, plot_z + plot_d),
                get_ground(plot_x + plot_w, plot_z + plot_d),
                get_ground(plot_x + plot_w / 2, plot_z + plot_d / 2)
            };
            int64_t highest_y = h[0];
            for (auto v : h)
                if (v > highest_y) highest_y = v;

            // d. FILL DIRT
            vector<shared_ptr<Block>> fill = compute_fill(
                plot_x, plot_z, plot_w, plot_d, planet->rootChunk);
            result.insert(result.end(), fill.begin(), fill.end());

            // e. GENERATE BUILDING
            int64_t bldg_seed = seed + plot_x + plot_z;
            vector<shared_ptr<Block>> building =
                BuildingGen::gen_building(
                    bldg_seed, bldg_seed + 1, btype,
                    plot_w, plot_d, dist->density, planet, city);

            for (auto& block : building) {
                if (block->position) {
                    block->position->y += highest_y;
                }
                result.push_back(block);
            }
        }

        // ── 5. MARK GENERATED ───────────────────────────────────
        city->generated_chunks.insert(chunk_key);

        return result;
    }


    /* Subdivide a chunk within a district into local roads and
       rectangular plots.  Roads are data only — this just carves
       the space, it doesn't produce blocks. */
    static vector<Plot> generate_local_roads_and_plots(
        int64_t seed,
        int64_t chunk_x,
        int64_t chunk_z,
        int64_t chunk_size,
        const District& district,
        const vector<Road>& main_roads = {}
    ) {
        vector<Plot> plots;

        int64_t cx0 = chunk_x, cz0 = chunk_z;
        int64_t cx1 = chunk_x + chunk_size - 1, cz1 = chunk_z + chunk_size - 1;
        int64_t spacing = max<int64_t>(8, district.local_road_spacing);
        int64_t half_road = 2; // LOCAL road half-width (4 wide)

        // Min plot area from citizen size in blocks
        double citizen_blocks = 3.0 / CoordinateConversion::BLOCK_SCALE;
        int64_t min_plot_area = static_cast<int64_t>(
            citizen_blocks * citizen_blocks * 2.0);

        // Check if chunk edges abut ARTERIAL roads
        auto road_near = [&](int64_t coord, bool is_x) -> bool {
            for (const auto& r : main_roads) {
                if (r.type != Road::Type::ARTERIAL) continue;
                for (const auto& pt : r.points) {
                    int64_t d = is_x ? abs(pt.x - coord) : abs(pt.z - coord);
                    if (d < spacing) return true;
                }
            }
            return false;
        };
        bool edge_north = road_near(cz0, false);
        bool edge_south = road_near(cz1, false);
        bool edge_west  = road_near(cx0, true);
        bool edge_east  = road_near(cx1, true);

        int64_t normal_setback = 2;
        int64_t arterial_setback = 8;

        auto create_plots = [&](const vector<int64_t>& x_str,
                                const vector<int64_t>& z_str) {
            // Build x intervals (gaps between N-S streets)
            struct Interval { int64_t lo, hi; };
            vector<Interval> x_ints, z_ints;

            auto build_intervals = [](int64_t min_c, int64_t max_c,
                                      const vector<int64_t>& streets,
                                      int64_t half_w) -> vector<Interval> {
                vector<Interval> res;
                if (streets.empty()) {
                    res.push_back({min_c, max_c});
                    return res;
                }
                res.push_back({min_c, streets[0] - half_w});
                for (size_t i = 1; i < streets.size(); i++) {
                    int64_t lo = streets[i-1] + half_w;
                    int64_t hi = streets[i] - half_w;
                    if (lo <= hi) res.push_back({lo, hi});
                }
                res.push_back({streets.back() + half_w, max_c});
                return res;
            };

            x_ints = build_intervals(cx0, cx1, x_str, half_road);
            z_ints = build_intervals(cz0, cz1, z_str, half_road);

            for (const auto& xi : x_ints) {
                for (const auto& zi : z_ints) {
                    int64_t w = xi.hi - xi.lo;
                    int64_t d = zi.hi - zi.lo;
                    if (w <= 0 || d <= 0) continue;
                    if (w * d < min_plot_area) continue;

                    Plot p;
                    p.x0 = xi.lo; p.z0 = zi.lo;
                    p.x1 = xi.hi; p.z1 = zi.hi;
                    p.y0 = district.y0; p.y1 = district.y1;
                    p.building_type = BuildingType::NONE;
                    p.style_group = nullptr;

                    p.road_setback = normal_setback;
                    if (xi.lo == cx0 && edge_west)   p.road_setback = arterial_setback;
                    if (xi.hi == cx1 && edge_east)   p.road_setback = arterial_setback;
                    if (zi.lo == cz0 && edge_north)  p.road_setback = arterial_setback;
                    if (zi.hi == cz1 && edge_south)  p.road_setback = arterial_setback;

                    plots.push_back(p);
                }
            }
        };

        if (district.street_regularity > 50) {
            // ── GRID — regular N-S/E-W streets ──────────────────
            vector<int64_t> x_str, z_str;
            for (int64_t x = cx0 + spacing; x < cx1; x += spacing)
                x_str.push_back(x);
            for (int64_t z = cz0 + spacing; z < cz1; z += spacing)
                z_str.push_back(z);
            create_plots(x_str, z_str);
        } else {
            // ── ORGANIC — jittered streets aligned to nearest ARTERIAL ──
            int64_t ccx = (cx0 + cx1) / 2;
            int64_t ccz = (cz0 + cz1) / 2;
            int64_t nearest_d = LLONG_MAX;
            double bearing = 0.0;
            for (const auto& r : main_roads) {
                if (r.type != Road::Type::ARTERIAL || r.points.size() < 2) continue;
                for (size_t i = 0; i + 1 < r.points.size(); i++) {
                    int64_t mx = (r.points[i].x + r.points[i+1].x) / 2;
                    int64_t mz = (r.points[i].z + r.points[i+1].z) / 2;
                    int64_t dd = (ccx - mx) * (ccx - mx) + (ccz - mz) * (ccz - mz);
                    if (dd < nearest_d) {
                        nearest_d = dd;
                        bearing = atan2(static_cast<double>(r.points[i+1].z - r.points[i].z),
                                        static_cast<double>(r.points[i+1].x - r.points[i].x));
                    }
                }
            }

            int64_t jitter_max = max<int64_t>(1, spacing / 4);
            vector<int64_t> x_str, z_str;

            int64_t x_cur = cx0 + spacing;
            while (x_cur < cx1) {
                int64_t jit = Random::randInt(seed + x_cur, 0, jitter_max);
                if (x_cur + jit < cx1) x_str.push_back(x_cur + jit);
                x_cur += spacing;
            }
            int64_t z_cur = cz0 + spacing;
            while (z_cur < cz1) {
                int64_t jit = Random::randInt(seed + 1000 + z_cur, 0, jitter_max);
                if (z_cur + jit < cz1) z_str.push_back(z_cur + jit);
                z_cur += spacing;
            }

            create_plots(x_str, z_str);
        }

        return plots;
    }


    // ═══════════════════════════════════════════════════════════════
    //  Helpers
    // ═══════════════════════════════════════════════════════════════

    /* Pick a building type suited to this district's zone. */
    static BuildingType pick_building_type(
        ZoneType zone,
        int64_t seed,
        double density = 50.0
    ) {
        int64_t roll = Random::randInt(seed, 0, 99);
        switch (zone) {
            case ZoneType::RESIDENTIAL: {
                int64_t home_chance = 80 - static_cast<int64_t>(density * 0.6);
                if (home_chance < 0) home_chance = 0;
                return roll < home_chance ? BuildingType::HOME : BuildingType::APARTMENTS;
            }
            case ZoneType::COMMERCIAL:
                if (roll < 40) return BuildingType::STORE;
                if (roll < 70) return BuildingType::RESTAURANT;
                if (roll < 90) return BuildingType::HOTEL;
                return BuildingType::OFFICE;
            case ZoneType::INDUSTRIAL:
                return roll < 80 ? BuildingType::FACTORY : BuildingType::STORE;
            case ZoneType::CIVIC:
                if (roll < 40) return BuildingType::GOVERNMENT;
                if (roll < 70) return BuildingType::RELIGIOUS;
                return BuildingType::PLAY;
            case ZoneType::PARK:
                return BuildingType::PARK;
            case ZoneType::MIXED: {
                int idx = Random::randInt(seed + 1, 0, 6);
                static const BuildingType types[] = {
                    BuildingType::HOME, BuildingType::STORE, BuildingType::RESTAURANT,
                    BuildingType::OFFICE, BuildingType::GOVERNMENT,
                    BuildingType::PLAY, BuildingType::HOTEL
                };
                return types[idx];
            }
            default:
                return BuildingType::HOME;
        }
    }


    /* True if the terrain across a plot of the given size is too
       steep to build on (given the district density). */
    static bool is_steep_slope(
        int64_t x,
        int64_t z,
        int64_t size_x,
        int64_t size_z,
        double density,
        shared_ptr<Chunk> root_chunk
    ) {
        auto get_height = [&](int64_t sx, int64_t sz) -> int64_t {
            auto chunk = root_chunk->getChunkOrClosestAncestorInitialized(
                ChunkType::CHUNK_1, sx, sz);
            if (!chunk) return 0;
            return chunk->position->y;
        };

        int64_t heights[] = {
            get_height(x, z),
            get_height(x + size_x, z),
            get_height(x, z + size_z),
            get_height(x + size_x, z + size_z),
            get_height(x + size_x / 2, z + size_z / 2)
        };

        int64_t min_h = heights[0];
        int64_t max_h = heights[0];
        for (auto h : heights) {
            if (h < min_h) min_h = h;
            if (h > max_h) max_h = h;
        }

        int64_t threshold = 10 + static_cast<int64_t>((100.0 - density) * 2.0);
        return (max_h - min_h) > threshold;
    }


    /* Compute fill-dirt blocks to level the plot at the height of
       the highest ground corner.  Returns blocks in world coords. */
    static vector<shared_ptr<Block>> compute_fill(
        int64_t x_base,
        int64_t z_base,
        int64_t width,
        int64_t depth,
        shared_ptr<Chunk> root_chunk
    ) {
        vector<shared_ptr<Block>> fill;

        auto get_height = [&](int64_t sx, int64_t sz) -> int64_t {
            auto chunk = root_chunk->getChunkOrClosestAncestorInitialized(
                ChunkType::CHUNK_1, sx, sz);
            if (!chunk) return 0;
            return chunk->position->y;
        };

        int64_t heights[] = {
            get_height(x_base, z_base),
            get_height(x_base + width, z_base),
            get_height(x_base, z_base + depth),
            get_height(x_base + width, z_base + depth),
            get_height(x_base + width / 2, z_base + depth / 2)
        };

        int64_t target_y = heights[0];
        for (auto h : heights) {
            if (h > target_y) target_y = h;
        }

        for (int64_t bx = x_base; bx <= x_base + width; ++bx) {
            for (int64_t bz = z_base; bz <= z_base + depth; ++bz) {
                int64_t ground_y = get_height(bx, bz);
                if (ground_y >= target_y) continue;
                for (int64_t y = ground_y + 1; y <= target_y; ++y) {
                    auto block = make_shared<Block>();
                    block->position = make_shared<Position>(bx, y, bz);
                    block->size = 1;
                    block->material = BMaterial::DIRT;
                    fill.push_back(block);
                }
            }
        }

        return fill;
    }

};