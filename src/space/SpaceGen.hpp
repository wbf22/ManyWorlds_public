#pragma once

#include "Space.hpp"
#include "util/Random.h"
#include "../server/Interface.hpp"
#include "../blocks/Block.h"
#include "../world/maps/IsometricRenderer.hpp"
#include "util/BMaterial.hpp"
#include "SkyGen.hpp"
#include "../world/World.hpp"
#include "../chunks/RootChunk.h"
#include "util/StellarCoordinate.hpp"
#include <vector>
#include <array>
#include <memory>
#include <utility>
  #include <tuple>
  #include <cmath>
#include <algorithm>
#include <climits>
#include <unordered_map>
#include <unordered_set>
#include <format>

using namespace std;

struct SpaceGen {

    // ─── Constants ──────────────────────────────────────────────
    static constexpr double BLOCKS_PER_RADIAN = 1000.0;
    static constexpr double SINGLE_BLOCK_THRESHOLD = 0.01;
    static constexpr int MIN_SPHERE_RADIUS_BLOCKS = 1;
    static constexpr int MAX_PLANET_VIEW_BLOCKS = 512;
    static constexpr int MAX_STAR_RADIUS_BLOCKS = 128;
    static constexpr int MAX_PLANET_BLOCKS = MAX_PLANET_VIEW_BLOCKS * MAX_PLANET_VIEW_BLOCKS;
    static constexpr int MAX_BLOCKS = 1'000'000;
    static constexpr double MAX_RANGE = 10000.0;
    static constexpr double LY_PER_M = 1.0 / 9.4607304725808e15;
    static constexpr double KM_PER_LY = 9460730472581.0;


    // ─── Coordinate helpers ───────────────────────────────────────

    static int64_t seed_from_coordinate(shared_ptr<StellarCoordinate> coord, int64_t spike = 0) {
        int64_t h = 0;
        auto combine = [](int64_t& seed, int64_t v) {
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
        combine(h, coord->quadrant_x);
        combine(h, coord->quadrant_y);
        combine(h, coord->quadrant_z);
        combine(h, coord->light_year_x);
        combine(h, coord->light_year_y);
        combine(h, coord->light_year_z);
        combine(h, coord->km_x);
        combine(h, coord->km_y);
        combine(h, coord->km_z);
        return Random::adjustedSeed(h + spike);
    }

    static int64_t floor_div(int64_t a, int64_t b) {
        int64_t q = a / b;
        int64_t r = a % b;
        if (r != 0 && ((a ^ b) < 0)) q--;
        return q;
    }

    static constexpr int64_t QUADRANTS_PER_CONTINUUM = 100000; // 1B ly / 10k ly per quadrant
    static constexpr int64_t QUADRANTS_PER_CHAMBER   = 10000;  // 100M ly / 10k ly per quadrant
    static constexpr int64_t AVG_SOLAR_MASSES_PER_SUPER_CLUSTER = 3750000000;  // 100M ly / 10k ly per quadrant

    static int64_t continuum_min_q(int64_t quadrant) {
        return floor_div(quadrant, QUADRANTS_PER_CONTINUUM) * QUADRANTS_PER_CONTINUUM;
    }

    static int64_t chamber_min_q(int64_t quadrant) {
        int64_t cont_min = continuum_min_q(quadrant);
        int64_t offset = quadrant - cont_min;
        int64_t ch_off = floor_div(offset, QUADRANTS_PER_CHAMBER) * QUADRANTS_PER_CHAMBER;
        return cont_min + ch_off;
    }

    static int64_t clamp_q(int64_t val, int64_t lo, int64_t hi) {
        return max(min(val, hi), lo);
    }

    // ─── Major cosmic river geometry ──────────────────────────────

    static constexpr double RIVER_INDIRECTNESS = 0.6;
    static constexpr double RIVER_SMOOTHNESS = 0.05;

    // Generic meandering filament walk (shared by cosmic rivers and nebula
    // strands). Advances in `step_size` chunks from `start` toward `end`,
    // accumulating a perpendicular offset (brownian, clamped to
    // step_size*indirectness) that eases back to the axis near the end via
    // `approach`. Both endpoints sit on the axis, so the walk starts at offset 0
    // and converges to 0 at the end. Callers tune the constants separately.
    // `max_steps < 0` runs the distance-based termination (rivers); a positive
    // `max_steps` bounds the walk to exactly that many steps (nebula strands) so
    // large meander amplitudes can't stall termination.
    static vector<shared_ptr<StellarCoordinate>> meander_walk(
        shared_ptr<StellarCoordinate> start,
        shared_ptr<StellarCoordinate> end,
        int64_t step_size,
        double indirectness,
        double smoothness,
        double approach_div,
        double damp_div,
        int64_t seed,
        int max_steps,
        int64_t clamp_lo_x, int64_t clamp_lo_y, int64_t clamp_lo_z,
        int64_t clamp_hi_x, int64_t clamp_hi_y, int64_t clamp_hi_z
    ) {
        vector<shared_ptr<StellarCoordinate>> pts;
        pts.push_back(start);

        int64_t cx = start->quadrant_x, cy = start->quadrant_y, cz = start->quadrant_z;
        int64_t ex = end->quadrant_x, ey = end->quadrant_y, ez = end->quadrant_z;
        int step = 0;

        auto dist_to_end = [&]() -> double {
            double dx = ex - cx, dy = ey - cy, dz = ez - cz;
            return sqrt(dx * dx + dy * dy + dz * dz);
        };

        double current_off1 = 0, current_off2 = 0;
        double off_max = (double)step_size * indirectness;

        while (dist_to_end() > step_size && (max_steps < 0 || step < max_steps)) {
            double remaining = dist_to_end();
            double dx = (ex - cx) / remaining;
            double dy = (ey - cy) / remaining;
            double dz = (ez - cz) / remaining;

            double up_x = 0, up_y = 0, up_z = 1;
            if (abs(dz) > 0.9) { up_x = 1; up_y = 0; up_z = 0; }

            double p1x = dy * up_z - dz * up_y;
            double p1y = dz * up_x - dx * up_z;
            double p1z = dx * up_y - dy * up_x;
            double p1len = sqrt(p1x * p1x + p1y * p1y + p1z * p1z);
            if (p1len > 0) { p1x /= p1len; p1y /= p1len; p1z /= p1len; }

            double p2x = dy * p1z - dz * p1y;
            double p2y = dz * p1x - dx * p1z;
            double p2z = dx * p1y - dy * p1x;
            double p2len = sqrt(p2x * p2x + p2y * p2y + p2z * p2z);
            if (p2len > 0) { p2x /= p2len; p2y /= p2len; p2z /= p2len; }

            double damp = min(remaining / (step_size * damp_div), 1.0);
            current_off1 *= damp;
            current_off2 *= damp;

            double delta1 = Random::randDouble(seed + step * 3 + 0,
                -(double)step_size * smoothness, (double)step_size * smoothness);
            double delta2 = Random::randDouble(seed + step * 3 + 1,
                -(double)step_size * smoothness, (double)step_size * smoothness);
            current_off1 = max(-off_max, min(off_max, current_off1 + delta1));
            current_off2 = max(-off_max, min(off_max, current_off2 + delta2));

            double approach = min(remaining / (step_size * approach_div), 1.0);
            double off1 = current_off1 * approach;
            double off2 = current_off2 * approach;

            cx += (int64_t)(dx * step_size + p1x * off1 + p2x * off2);
            cy += (int64_t)(dy * step_size + p1y * off1 + p2y * off2);
            cz += (int64_t)(dz * step_size + p1z * off1 + p2z * off2);

            cx = clamp_q(cx, clamp_lo_x, clamp_hi_x);
            cy = clamp_q(cy, clamp_lo_y, clamp_hi_y);
            cz = clamp_q(cz, clamp_lo_z, clamp_hi_z);

            auto pt = make_shared<StellarCoordinate>();
            pt->quadrant_x = cx; pt->quadrant_y = cy; pt->quadrant_z = cz;
            pts.push_back(pt);
            step++;
        }

        pts.push_back(end);
        return pts;
    }

    static shared_ptr<Cosmic_River> walk_river(
        shared_ptr<StellarCoordinate> start,
        shared_ptr<StellarCoordinate> end,
        int64_t step_size,
        int64_t seed,
        int64_t clamp_lo_x, int64_t clamp_lo_y, int64_t clamp_lo_z,
        int64_t clamp_hi_x, int64_t clamp_hi_y, int64_t clamp_hi_z
    ) {
        auto river = make_shared<Cosmic_River>();
        river->path_points = meander_walk(
            start, end, step_size,
            RIVER_INDIRECTNESS, RIVER_SMOOTHNESS, 3.0, 5.0, seed, -1,
            clamp_lo_x, clamp_lo_y, clamp_lo_z,
            clamp_hi_x, clamp_hi_y, clamp_hi_z);

        river->min_qx = river->max_qx = start->quadrant_x;
        river->min_qy = river->max_qy = start->quadrant_y;
        river->min_qz = river->max_qz = start->quadrant_z;
        for (auto& pt : river->path_points) {
            if (pt->quadrant_x < river->min_qx) river->min_qx = pt->quadrant_x;
            if (pt->quadrant_x > river->max_qx) river->max_qx = pt->quadrant_x;
            if (pt->quadrant_y < river->min_qy) river->min_qy = pt->quadrant_y;
            if (pt->quadrant_y > river->max_qy) river->max_qy = pt->quadrant_y;
            if (pt->quadrant_z < river->min_qz) river->min_qz = pt->quadrant_z;
            if (pt->quadrant_z > river->max_qz) river->max_qz = pt->quadrant_z;
        }

        return river;
    }

    static vector<shared_ptr<Cosmic_River>> generate_major_rivers() {
        vector<shared_ptr<Cosmic_River>> rivers;
        int64_t seed = Random::adjustedSeed(0);
        int64_t half_span = Util::MAX_INT64_T - 1; 
        int64_t margin = half_span / 10;
        int64_t min_q = -half_span + margin;
        int64_t max_q = half_span - margin;

        // MemInfo::start();
        double SPREAD = 0;
        for (int i = 0; i < 256; i++) {
            auto start = make_shared<StellarCoordinate>();
            start->quadrant_x = Random::randInt(seed + i * 9 + 0, min_q, 0, max_q, SPREAD);
            start->quadrant_y = Random::randInt(seed + i * 9 + 1, min_q, 0, max_q, SPREAD);
            start->quadrant_z = Random::randInt(seed + i * 9 + 2, min_q, 0, max_q, SPREAD);

            auto end = make_shared<StellarCoordinate>();
            end->quadrant_x = Random::randInt(seed + i * 9 + 3, min_q, 0, max_q, SPREAD);
            end->quadrant_y = Random::randInt(seed + i * 9 + 4, min_q, 0, max_q, SPREAD);
            end->quadrant_z = Random::randInt(seed + i * 9 + 5, min_q, 0, max_q, SPREAD);

            int64_t dist = Util::manhatten_dist(
                start->quadrant_x, 
                start->quadrant_y, 
                start->quadrant_z, 
                end->quadrant_x, 
                end->quadrant_y, 
                end->quadrant_z
            );
            
            auto river = walk_river(start, end, dist/256, seed + i * 9 + 6,
                                    min_q, min_q, min_q, max_q, max_q, max_q);
            rivers.push_back(river);
        }
        // MemInfo::stop("universe mem");
        return rivers;
    }

    // Checks if a line segment (a→b) passes through the given axis-aligned box.
    // Uses Liang-Barsky / slab method — exact, O(1), no iterations.
    static bool segment_touches_box(
        shared_ptr<StellarCoordinate> a,
        shared_ptr<StellarCoordinate> b,
        int64_t bx1, int64_t by1, int64_t bz1,
        int64_t bx2, int64_t by2, int64_t bz2
    ) {
        double sx = (double)a->quadrant_x, sy = (double)a->quadrant_y, sz = (double)a->quadrant_z;
        double ex = (double)b->quadrant_x, ey = (double)b->quadrant_y, ez = (double)b->quadrant_z;

        double t_min = 0.0, t_max = 1.0;

        auto slab = [&](double s, double e, double lo, double hi) -> bool {
            if (e == s) return s >= lo && s <= hi;
            double t1 = (lo - s) / (e - s);
            double t2 = (hi - s) / (e - s);
            if (t1 > t2) swap(t1, t2);
            if (t1 > t_min) t_min = t1;
            if (t2 < t_max) t_max = t2;
            return t_min <= t_max;
        };

        return slab(sx, ex, (double)bx1, (double)bx2)
            && slab(sy, ey, (double)by1, (double)by2)
            && slab(sz, ez, (double)bz1, (double)bz2);
    }

    static vector<shared_ptr<Cosmic_River>> find_rivers_in_box(
        shared_ptr<Universe> universe,
        int64_t bx1, int64_t by1, int64_t bz1,
        int64_t bx2, int64_t by2, int64_t bz2,
        shared_ptr<Continuum> continuum = nullptr
    ) {
        // Normalise query box so lo ≤ hi
        if (bx1 > bx2) swap(bx1, bx2);
        if (by1 > by2) swap(by1, by2);
        if (bz1 > bz2) swap(bz1, bz2);

        vector<shared_ptr<Cosmic_River>> out;

        auto scan = [&](auto& rivers) {
            for (auto& river : rivers) {
                if (river->max_qx < bx1 || river->min_qx > bx2 ||
                    river->max_qy < by1 || river->min_qy > by2 ||
                    river->max_qz < bz1 || river->min_qz > bz2)
                    continue;

                auto& pts = river->path_points;
                for (size_t i = 0; i + 1 < pts.size(); i++) {
                    if (segment_touches_box(pts[i], pts[i + 1], bx1, by1, bz1, bx2, by2, bz2)) {
                        out.push_back(river);
                        break;
                    }
                }
            }
        };

        scan(universe->main_cosmic_rivers);
        if (continuum) scan(continuum->cosmic_rivers);

        return out;
    }

    // ─── Universe ─────────────────────────────────────────────────

    static shared_ptr<Universe> generate_universe() {
        auto universe = make_shared<Universe>();
        universe->main_cosmic_rivers = generate_major_rivers();
        return universe;
    }

    // ─── Continuum (1B ly³) ───────────────────────────────────────

    static shared_ptr<Continuum> generate_continuum(
        shared_ptr<Universe> universe,
        shared_ptr<StellarCoordinate> coordinate
    ) {
        auto continuum = make_shared<Continuum>();

        int64_t cmin_x = continuum_min_q(coordinate->quadrant_x);
        int64_t cmin_y = continuum_min_q(coordinate->quadrant_y);
        int64_t cmin_z = continuum_min_q(coordinate->quadrant_z);
        int64_t cmax_x = cmin_x + QUADRANTS_PER_CONTINUUM - 1;
        int64_t cmax_y = cmin_y + QUADRANTS_PER_CONTINUUM - 1;
        int64_t cmax_z = cmin_z + QUADRANTS_PER_CONTINUUM - 1;
        int64_t step_size = QUADRANTS_PER_CHAMBER;

        int64_t cont_seed = Random::seed_from_coordinates(
            (cmin_x / QUADRANTS_PER_CONTINUUM),
            (cmin_y / QUADRANTS_PER_CONTINUUM),
            (cmin_z / QUADRANTS_PER_CONTINUUM)
        );

        auto nearby = find_rivers_in_box(universe, cmin_x, cmin_y, cmin_z, cmax_x, cmax_y, cmax_z);

        if (nearby.empty()) {
            // 5% chance for a void continuum to still have a minor river
            if (Random::randBool(cont_seed, 5)) {
                auto start = make_shared<StellarCoordinate>();
                start->quadrant_x = Random::randInt(cont_seed + 1,
                    cmin_x, cmax_x);
                start->quadrant_y = Random::randInt(cont_seed + 2,
                    cmin_y, cmax_y);
                start->quadrant_z = Random::randInt(cont_seed + 3,
                    cmin_z, cmax_z);

                auto end = make_shared<StellarCoordinate>();
                end->quadrant_x = Random::randInt(cont_seed + 4,
                    cmin_x, cmax_x);
                end->quadrant_y = Random::randInt(cont_seed + 5,
                    cmin_y, cmax_y);
                end->quadrant_z = Random::randInt(cont_seed + 6,
                    cmin_z, cmax_z);


                auto river = walk_river(start, end, step_size, cont_seed + 8,
                    cmin_x, cmin_y, cmin_z, cmax_x, cmax_y, cmax_z);
                continuum->cosmic_rivers.push_back(river);
            }
        } else {
            // Up to 3 major filaments get branches inside this continuum
            int limit = min((int)nearby.size(), 3);
            for (int fi = 0; fi < limit; fi++) {
                auto& f = nearby[fi];

                // Main river segment — entry → exit across the continuum
                auto& f_pts = f->path_points;

                auto entry = make_shared<StellarCoordinate>();
                entry->quadrant_x = clamp_q(f_pts[0]->quadrant_x, cmin_x, cmax_x);
                entry->quadrant_y = clamp_q(f_pts[0]->quadrant_y, cmin_y, cmax_y);
                entry->quadrant_z = clamp_q(f_pts[0]->quadrant_z, cmin_z, cmax_z);

                auto end = make_shared<StellarCoordinate>();
                end->quadrant_x = Random::randInt(cont_seed, cmin_x, cmax_x);
                end->quadrant_y = Random::randInt(cont_seed, cmin_y, cmax_y);
                end->quadrant_z = Random::randInt(cont_seed, cmin_z, cmax_z);

                auto main_river = walk_river(entry, end, step_size, cont_seed,
                                             cmin_x, cmin_y, cmin_z, cmax_x, cmax_y, cmax_z);

                continuum->cosmic_rivers.push_back(main_river);

                // Side branches — meandering filaments via walk_river
                int nb = Random::randInt(cont_seed + fi * 5, 0, 3);
                for (int b = 0; b < nb; b++) {
                    double t = Random::randDouble(cont_seed + fi * 5 + b * 3 + 1, 0.1, 0.9);
                    int64_t bx = entry->quadrant_x + (int64_t)((end->quadrant_x - entry->quadrant_x) * t);
                    int64_t by = entry->quadrant_y + (int64_t)((end->quadrant_y - entry->quadrant_y) * t);
                    int64_t bz = entry->quadrant_z + (int64_t)((end->quadrant_z - entry->quadrant_z) * t);

                    int64_t blen = Random::randInt(cont_seed + fi * 5 + b * 3 + 2,
                        (QUADRANTS_PER_CONTINUUM * 0.05), (QUADRANTS_PER_CONTINUUM * 0.3));

                    int64_t dx = 0, dy = 0, dz = 0;
                    switch (Random::randInt(cont_seed + fi * 5 + b * 3 + 3, 0, 5)) {
                        case 0: dx =  blen; break; case 1: dx = -blen; break;
                        case 2: dy =  blen; break; case 3: dy = -blen; break;
                        case 4: dz =  blen; break; case 5: dz = -blen; break;
                    }

                    auto start = make_shared<StellarCoordinate>();
                    start->quadrant_x = clamp_q(bx,             cmin_x, cmax_x);
                    start->quadrant_y = clamp_q(by,             cmin_y, cmax_y);
                    start->quadrant_z = clamp_q(bz,             cmin_z, cmax_z);

                    end = make_shared<StellarCoordinate>();
                    end->quadrant_x = clamp_q(bx + dx, cmin_x, cmax_x);
                    end->quadrant_y = clamp_q(by + dy, cmin_y, cmax_y);
                    end->quadrant_z = clamp_q(bz + dz, cmin_z, cmax_z);

                    auto branch = walk_river(start, end, step_size, cont_seed + fi * 5 + b * 3 + 5,
                                             cmin_x, cmin_y, cmin_z, cmax_x, cmax_y, cmax_z);
                    continuum->cosmic_rivers.push_back(branch);
                }
            }
        }

        return continuum;
    }

    // ─── Chamber (100M ly³) ──────────────────────────────────────

    static shared_ptr<Chamber> generate_chamber(
        shared_ptr<Universe> universe,
        shared_ptr<StellarCoordinate> coordinate,
        shared_ptr<Continuum> continuum
    ) {
        auto chamber = make_shared<Chamber>();

        int64_t ch_x = chamber_min_q(coordinate->quadrant_x);
        int64_t ch_y = chamber_min_q(coordinate->quadrant_y);
        int64_t ch_z = chamber_min_q(coordinate->quadrant_z);
        int64_t ch_max_x = ch_x + QUADRANTS_PER_CHAMBER - 1;
        int64_t ch_max_y = ch_y + QUADRANTS_PER_CHAMBER - 1;
        int64_t ch_max_z = ch_z + QUADRANTS_PER_CHAMBER - 1;

        int64_t ch_seed = Random::seed_from_coordinates(ch_x, ch_y, ch_z);

        auto nearby = find_rivers_in_box(universe, ch_x, ch_y, ch_z, ch_max_x, ch_max_y, ch_max_z, continuum);
        bool has_river = !nearby.empty();
        bool void_gal = !has_river && Random::randBool(ch_seed, 2); // 2% void chance

        // ── Chamber-scale cosmic filaments ──
        // Only generate when a major river passes through, giving
        // the chamber a web-like structure.
        if (has_river) {
            int nf = Random::randInt(ch_seed + 200, 0, 3);
            for (int f = 0; f < nf; f++) {
                auto f_start = make_shared<StellarCoordinate>();
                f_start->quadrant_x = Random::randInt(ch_seed + f * 7 + 201, ch_x, ch_max_x);
                f_start->quadrant_y = Random::randInt(ch_seed + f * 7 + 202, ch_y, ch_max_y);
                f_start->quadrant_z = Random::randInt(ch_seed + f * 7 + 203, ch_z, ch_max_z);

                auto f_end = make_shared<StellarCoordinate>();
                f_end->quadrant_x = Random::randInt(ch_seed + f * 7 + 204, ch_x, ch_max_x);
                f_end->quadrant_y = Random::randInt(ch_seed + f * 7 + 205, ch_y, ch_max_y);
                f_end->quadrant_z = Random::randInt(ch_seed + f * 7 + 206, ch_z, ch_max_z);

                int64_t step = 1000;

                auto filament = walk_river(f_start, f_end, step, ch_seed + f * 7 + 208,
                    ch_x, ch_y, ch_z, ch_max_x, ch_max_y, ch_max_z);
                chamber->cosmic_rivers.push_back(filament);
            }
            // Include chamber-scale filaments so galaxies place along them too
            for (auto& r : chamber->cosmic_rivers)
                nearby.push_back(r);
        }


        if (has_river || void_gal) {
            int ng = has_river ? Random::randInt(ch_seed + 1, 1, 6) * nearby.size() : 1;

            for (int g = 0; g < ng; g++) {
                int64_t gx, gy, gz;

                if (g < (int)nearby.size()) {
                    auto& f = nearby[g];
                    auto& f_pts = f->path_points;
                    if (f_pts.size() >= 2) {
                        int seg = Random::randInt(ch_seed + g * 100 + 50, 0, f_pts.size() - 2);
                        double t = Random::randDouble(ch_seed + g * 100 + 51, 0.0, 1.0);
                        gx = f_pts[seg]->quadrant_x + (int64_t)((f_pts[seg + 1]->quadrant_x - f_pts[seg]->quadrant_x) * t);
                        gy = f_pts[seg]->quadrant_y + (int64_t)((f_pts[seg + 1]->quadrant_y - f_pts[seg]->quadrant_y) * t);
                        gz = f_pts[seg]->quadrant_z + (int64_t)((f_pts[seg + 1]->quadrant_z - f_pts[seg]->quadrant_z) * t);

                        // Blend out-of-bounds coordinates toward chamber center to
                        // prevent galaxy pile-up on faces from rivers that clip the edge.
                        int64_t ch_cx = (ch_x + ch_max_x) / 2;
                        int64_t ch_cy = (ch_y + ch_max_y) / 2;
                        int64_t ch_cz = (ch_z + ch_max_z) / 2;
                        auto soften = [&](int64_t& v, int64_t lo, int64_t hi, int64_t center, int64_t sd) {
                            if (v < lo || v > hi) {
                                double w = Random::randDouble(sd, 0.2, 0.8);
                                v = (int64_t)((double)clamp_q(v, lo, hi) * w + (double)center * (1.0 - w));
                            }
                        };
                        soften(gx, ch_x, ch_max_x, ch_cx, ch_seed + g * 100 + 52);
                        soften(gy, ch_y, ch_max_y, ch_cy, ch_seed + g * 100 + 53);
                        soften(gz, ch_z, ch_max_z, ch_cz, ch_seed + g * 100 + 54);
                    } else {
                        gx = f_pts[0]->quadrant_x;
                        gy = f_pts[0]->quadrant_y;
                        gz = f_pts[0]->quadrant_z;
                    }
                } else {
                    gx = Random::randInt(ch_seed + g * 3 + 2, ch_x, ch_max_x);
                    gy = Random::randInt(ch_seed + g * 3 + 3, ch_y, ch_max_y);
                    gz = Random::randInt(ch_seed + g * 3 + 4, ch_z, ch_max_z);
                }

                auto gal_pos = make_shared<StellarCoordinate>();
                gal_pos->quadrant_x = clamp_q(gx, ch_x, ch_max_x);
                gal_pos->quadrant_y = clamp_q(gy, ch_y, ch_max_y);
                gal_pos->quadrant_z = clamp_q(gz, ch_z, ch_max_z);

                auto galaxy = make_shared<Galaxy>();
                galaxy->location = gal_pos;
                galaxy->parent = chamber;
                generate_galaxy_properties(galaxy, gal_pos);
                chamber->galaxies[Index3{gal_pos->quadrant_x, gal_pos->quadrant_y, gal_pos->quadrant_z}] = galaxy;
            }
        }


        return chamber;
    }

    // ─── Galaxy ───────────────────────────────────────────────────

    static void generate_galaxy_properties(
        shared_ptr<Galaxy>& galaxy,
        shared_ptr<StellarCoordinate> coordinate
    ) {
        galaxy->location = coordinate;

        int64_t gs = Random::seed_from_coordinates(
            coordinate->quadrant_x,
            coordinate->quadrant_y,
            coordinate->quadrant_z
        );

        galaxy->type = Random::weighted_choice<Galaxy::GalaxyType>(
            {55, 25, 10, 5, 5},
            {Galaxy::GalaxyType::SPIRAL, Galaxy::GalaxyType::ELIPTICAL,
             Galaxy::GalaxyType::CLOUD, Galaxy::GalaxyType::RING,
             Galaxy::GalaxyType::RANDOM},
            gs + 0
        );

        galaxy->mass       = (int64_t)Random::randDouble(gs + 1, 1e9, 5e10, 1e12);
        galaxy->age        = Random::randDouble(gs + 2, 1.0, 6.0);
        galaxy->angular_velocity = Random::randDouble(gs + 3, 0.0, 0.0005);

        // Gas fraction by galaxy type: spirals/clouds are gas-rich, ellipticals spent
        double gas_lo, gas_hi;
        switch (galaxy->type) {
            case Galaxy::GalaxyType::SPIRAL:
            case Galaxy::GalaxyType::CLOUD:
                gas_lo = 0.35; gas_hi = 0.6; break;
            case Galaxy::GalaxyType::RANDOM:
                gas_lo = 0.25; gas_hi = 0.5; break;
            case Galaxy::GalaxyType::RING:
                gas_lo = 0.15; gas_hi = 0.35; break;
            default: // ELLIPTICAL
                gas_lo = 0.01; gas_hi = 0.06; break;
        }
        galaxy->gas_fraction = Random::randDouble(gs + 16, gas_lo, gas_hi);

        double mass_cuberoot = pow((double)galaxy->mass / 1.0e9, 1.0 / 3.0);
        double base_scale = 3000.0 * mass_cuberoot;
        double scatter = Random::randDouble(gs + 4, 0.4, 1.0, 2.5);
        galaxy->scale_length = base_scale * scatter;
        galaxy->radius_ly = galaxy->scale_length * 2.5;
        galaxy->core_mass    = Random::randDouble(gs + 5, 1e6, 1e8, 1e10);

        if (galaxy->type == Galaxy::GalaxyType::SPIRAL) {
            galaxy->arm_count   = Random::randInt(gs + 6, 2, 8);
            galaxy->arm_winding = Random::randDouble(gs + 7, 0.2, 0.5, 0.9);
            galaxy->bar_length  = Random::randDouble(gs + 8, 0.0, 0.3, 0.6);
        }

        // Disc orientation — 3 reference points on the galactic disc rim.
        {
            double disc_r = galaxy->scale_length * 2.5;
            double theta_offset = Random::randDouble(gs + 12, 0.0, 2.0 * M_PI);
            double plane_normal_el = acos(Random::randDouble(gs + 13, -1.0, 1.0));
            double plane_normal_az = Random::randDouble(gs + 14, 0.0, 2.0 * M_PI);

            double nx = sin(plane_normal_el) * cos(plane_normal_az);
            double ny = sin(plane_normal_el) * sin(plane_normal_az);
            double nz = cos(plane_normal_el);

            // Build two orthogonal in-plane vectors
            double ux = 1, uy = 0, uz = 0;
            if (abs(nx) < 0.9) {
                ux = 0; uy = 1; uz = 0;
            }
            double vx = ny * uz - nz * uy;
            double vy = nz * ux - nx * uz;
            double vz = nx * uy - ny * ux;
            double vlen = sqrt(vx*vx + vy*vy + vz*vz);
            vx /= vlen; vy /= vlen; vz /= vlen;

            double u2x = ny * vz - nz * vy;
            double u2y = nz * vx - nx * vz;
            double u2z = nx * vy - ny * vx;

            auto disc_point = [&](double theta) {
                double ct = cos(theta), st = sin(theta);
                double px = disc_r * (ct * vx + st * u2x);
                double py = disc_r * (ct * vy + st * u2y);
                double pz = disc_r * (ct * vz + st * u2z);

                if (galaxy->type != Galaxy::GalaxyType::SPIRAL) {
                    // Elliptical/cloud: scatter within the volume
                    double spread = Random::randDouble(gs + 15, 0.3, 0.8);
                    px *= spread; py *= spread; pz *= spread;
                }

                return array<double, 3>{px, py, pz};
            };

            auto a = disc_point(theta_offset);
            auto b = disc_point(theta_offset + 2.0 * M_PI / 3.0);
            auto c = disc_point(theta_offset + 4.0 * M_PI / 3.0);

            galaxy->orient_a_ly[0] = a[0]; galaxy->orient_a_ly[1] = a[1]; galaxy->orient_a_ly[2] = a[2];
            galaxy->orient_b_ly[0] = b[0]; galaxy->orient_b_ly[1] = b[1]; galaxy->orient_b_ly[2] = b[2];
            galaxy->orient_c_ly[0] = c[0]; galaxy->orient_c_ly[1] = c[1]; galaxy->orient_c_ly[2] = c[2];

            // Bar direction — along the v in-plane axis (θ=0 reference on disc)
            if (galaxy->bar_length > 0.0) {
                double bm = galaxy->bar_length * galaxy->scale_length;
                galaxy->bar_direction_ly[0] = vx * bm;
                galaxy->bar_direction_ly[1] = vy * bm;
                galaxy->bar_direction_ly[2] = vz * bm;
            }
        }

        if (galaxy->type == Galaxy::GalaxyType::ELIPTICAL) {
            galaxy->color_r = Random::randDouble(gs + 9,  0.6, 0.8);
            galaxy->color_g = Random::randDouble(gs + 10, 0.3, 0.5);
            galaxy->color_b = Random::randDouble(gs + 11, 0.1, 0.3);
        } else {
            galaxy->color_r = Random::randDouble(gs + 9,  0.3, 0.6);
            galaxy->color_g = Random::randDouble(gs + 10, 0.4, 0.7);
            galaxy->color_b = Random::randDouble(gs + 11, 0.5, 0.9);
        }
    }

    // ── Shared disc-plane helpers ─────────────────────────────────

    /* Reconstructs the galaxy's disc plane as an orthonormal basis
       (u, v in-plane, n = disc normal) from orient_a_ly / b_ly. */
    static void galaxy_disc_basis(
        const Galaxy& galaxy,
        double& ux, double& uy, double& uz,
        double& vx, double& vy, double& vz,
        double& nx, double& ny, double& nz
    ) {
        // u = normalize(orient_a_ly)
        double u_len = sqrt(galaxy.orient_a_ly[0] * galaxy.orient_a_ly[0]
                          + galaxy.orient_a_ly[1] * galaxy.orient_a_ly[1]
                          + galaxy.orient_a_ly[2] * galaxy.orient_a_ly[2]);
        ux = galaxy.orient_a_ly[0] / u_len;
        uy = galaxy.orient_a_ly[1] / u_len;
        uz = galaxy.orient_a_ly[2] / u_len;

        // n = normalize(cross(orient_a, orient_b))
        nx = galaxy.orient_a_ly[1] * galaxy.orient_b_ly[2] - galaxy.orient_a_ly[2] * galaxy.orient_b_ly[1];
        ny = galaxy.orient_a_ly[2] * galaxy.orient_b_ly[0] - galaxy.orient_a_ly[0] * galaxy.orient_b_ly[2];
        nz = galaxy.orient_a_ly[0] * galaxy.orient_b_ly[1] - galaxy.orient_a_ly[1] * galaxy.orient_b_ly[0];
        double n_len = sqrt(nx*nx + ny*ny + nz*nz);
        nx /= n_len; ny /= n_len; nz /= n_len;

        // v = cross(n, u)
        vx = ny * uz - nz * uy;
        vy = nz * ux - nx * uz;
        vz = nx * uy - ny * ux;
    }

    /* Transform polar coords (r, θ) on the galaxy's disc plane into
       3D world-space LY offsets from galaxy center.
       Optionally returns the in-plane tangent direction. */
    static void galaxy_disc_point(
        const Galaxy& galaxy,
        double r, double theta,
        double& x_ly, double& y_ly, double& z_ly,
        double* tx = nullptr, double* ty = nullptr, double* tz = nullptr
    ) {
        double ux, uy, uz, vx, vy, vz, nx, ny, nz;
        galaxy_disc_basis(galaxy, ux, uy, uz, vx, vy, vz, nx, ny, nz);

        double ct = cos(theta), st = sin(theta);
        x_ly = r * (ct * ux + st * vx);
        y_ly = r * (ct * uy + st * vy);
        z_ly = r * (ct * uz + st * vz);

        if (tx && ty && tz) {
            *tx = -st * ux + ct * vx;
            *ty = -st * uy + ct * vy;
            *tz = -st * uz + ct * vz;
        }
    }

    static void galaxy_spiral_pos(
        shared_ptr<Galaxy> galaxy,
        int64_t seed,
        double& x_ly, double& y_ly, double& z_ly,
        double arm_min = 0.1,
        double arm_max = 3.0,
        double theta_noise = 0.3,
        double r_noise = 0.1,
        double phi_noise = 0.2,
        double halo_max = 2.5
    ) {
        double r_ly, theta, phi;
        if (galaxy->type == Galaxy::GalaxyType::SPIRAL && galaxy->arm_count > 0) {
            double arm_r = Random::randDouble(seed, galaxy->scale_length * arm_min, galaxy->scale_length * arm_max);
            double arm_angle = Random::randDouble(seed + 1, 0.0, 6.2832) * galaxy->arm_count;
            double wind_shift = galaxy->arm_winding * (arm_r / galaxy->scale_length);
            theta = arm_angle + wind_shift + Random::randDouble(seed + 2, -theta_noise, theta_noise);
            r_ly  = arm_r + Random::randDouble(seed + 3, -galaxy->scale_length * r_noise, galaxy->scale_length * r_noise);
            phi   = Random::randDouble(seed + 4, -phi_noise, phi_noise);
        } else {
            r_ly  = Random::randDouble(seed, 0.0, galaxy->scale_length * halo_max);
            theta = Random::randDouble(seed + 1, 0.0, 6.2832);
            phi   = acos(Random::randDouble(seed + 2, -1.0, 1.0));
        }

        // Base position on the disc plane
        galaxy_disc_point(*galaxy, r_ly, theta, x_ly, y_ly, z_ly);

        // Vertical scatter along disc normal
        double ux, uy, uz, vx, vy, vz, nx, ny, nz;
        galaxy_disc_basis(*galaxy, ux, uy, uz, vx, vy, vz, nx, ny, nz);
        double v_offset = r_ly * sin(phi);
        x_ly += nx * v_offset;
        y_ly += ny * v_offset;
        z_ly += nz * v_offset;
    }

    // Filament axis: normalize, then two orthonormal perp vectors (b, c) around it
    static void orthonormal_basis(
        double& ax, double& ay, double& az,
        double& bx, double& by, double& bz,
        double& cx, double& cy, double& cz
    ) {
        double alen = sqrt(ax*ax + ay*ay + az*az);
        ax /= alen; ay /= alen; az /= alen;

        double ref_x = 1.0, ref_y = 0.0, ref_z = 0.0;
        if (fabs(ax) >= 0.9) { ref_x = 0.0; ref_y = 1.0; }
        bx = ay*ref_z - az*ref_y;
        by = az*ref_x - ax*ref_z;
        bz = ax*ref_y - ay*ref_x;
        double blen = sqrt(bx*bx + by*by + bz*bz);
        bx /= blen; by /= blen; bz /= blen;

        cx = ay*bz - az*by;
        cy = az*bx - ax*bz;
        cz = ax*by - ay*bx;
    }

    // Offset along a filament: ±80 along the axis, ±40 perp, scaled by extent
    static void scatter_filament_point(
        int64_t seed,
        double extent,
        double ax, double ay, double az,
        double bx, double by, double bz,
        double cx, double cy, double cz,
        double& px, double& py, double& pz
    ) {
        double scale = extent / 80.0;
        double along = Random::randDouble(seed, -80.0, 80.0) * scale;
        double perp1 = Random::randDouble(seed + 1, -40.0, 40.0) * scale;
        double perp2 = Random::randDouble(seed + 2, -40.0, 40.0) * scale;
        px = along * ax + perp1 * bx + perp2 * cx;
        py = along * ay + perp1 * by + perp2 * cy;
        pz = along * az + perp1 * bz + perp2 * cz;
    }

    // Random unit filament axis + its orthonormal basis (seed-driven, same offsets as old inline code)
    static void random_filament_axis(
        int64_t seed,
        double& ax, double& ay, double& az,
        double& bx, double& by, double& bz,
        double& cx, double& cy, double& cz
    ) {
        ax = Random::randDouble(seed + 20, -1.0, 1.0);
        ay = Random::randDouble(seed + 21, -1.0, 1.0);
        az = Random::randDouble(seed + 22, -1.0, 1.0);
        orthonormal_basis(ax, ay, az, bx, by, bz, cx, cy, cz);
    }

    // Nebula skeleton: meandering filament threads (river-style centerlines)
    // that extend along the dominant axis. Each path point is also a cloud
    // anchor — block-gen scatters blocks around it with a per-point cloud size.
    // The meander is per-nebula (derived from the nebula seed) so far/near LOD
    // keep the same shape.
    static void generate_nebula_strand_paths(shared_ptr<Nebula> nebula, int64_t seed) {
        nebula->strand_paths.clear();
        nebula->filament_points.clear();
        double len_ly = nebula->along_extent * nebula->radius_ly;
        double ax = nebula->fax, ay = nebula->fay, az = nebula->faz;
        int knots = max(4, nebula->meander_knots);
        int64_t step_size = max((int64_t)1, (int64_t)lround(2.0 * len_ly / knots));
        // Nebula-tuned meander constants so strands sweep ~meander_amp off-axis
        // (river's 0.6/0.05 would stay too straight at this short scale).
        double indirectness = 1.4 * nebula->meander_amp * knots / (2.0 * nebula->along_extent);
        double smoothness = nebula->meander_amp * sqrt((double)knots) / (2.0 * nebula->along_extent);
        const int64_t BIG = (int64_t)1 << 50;
        int idx = 0;
        for (int s = 0; s < nebula->strands; s++) {
            auto start = make_shared<StellarCoordinate>();
            start->quadrant_x = (int64_t)lround(-ax * len_ly);
            start->quadrant_y = (int64_t)lround(-ay * len_ly);
            start->quadrant_z = (int64_t)lround(-az * len_ly);
            auto end = make_shared<StellarCoordinate>();
            end->quadrant_x = (int64_t)lround(ax * len_ly);
            end->quadrant_y = (int64_t)lround(ay * len_ly);
            end->quadrant_z = (int64_t)lround(az * len_ly);
            auto path = meander_walk(start, end, step_size, indirectness, smoothness, 3.0, 5.0,
                                     nebula->wavy_seed + s * 7919, knots,
                                     -BIG, -BIG, -BIG, BIG, BIG, BIG);
            vector<array<int64_t, 3>> pts;
            pts.reserve(path.size());
            for (auto& p : path) {
                pts.push_back({p->quadrant_x, p->quadrant_y, p->quadrant_z});
                // Each path point is a cloud anchor with a per-point cloud size
                Nebula::FilamentPoint fp;
                fp.x = p->quadrant_x; fp.y = p->quadrant_y; fp.z = p->quadrant_z;
                fp.cloud_size = (float)(nebula->radius_ly * Random::randDouble(seed + 100 + idx * 13, 0.06, 0.2));
                nebula->filament_points.push_back(fp);
                ++idx;
            }
            nebula->strand_paths.push_back(pts);
        }
    }

    // Sub-strands: short meandering wisps branching off every strand path point,
    // so the skeleton reads as a web of filaments rather than a bare path. Each
    // wisp runs a mini meander_walk toward a random direction; every wisp point
    // becomes a cloud anchor too. Per-point seeds keep the shape deterministic
    // across LODs.
    static void generate_nebula_sub_strands(shared_ptr<Nebula> nebula, int64_t seed) {
        const int64_t BIG = (int64_t)1 << 50;
        int idx = 0;
        for (size_t s = 0; s < nebula->strand_paths.size(); s++) {
            auto& path = nebula->strand_paths[s];
            for (size_t p = 0; p < path.size(); p++) {
                int64_t ps = seed + 1000 + (int64_t)s * 7919 + (int64_t)p * 37;
                int nsub = Random::randInt(ps, 1, 10);
                for (int w = 0; w < nsub; w++) {
                    int64_t ws = ps + w * 104729; // prime so sub-wisps don't collide
                    double sub_len = nebula->radius_ly * Random::randDouble(ws, 0.2, 0.6);
                    // Random 3D direction for the wisp
                    double dx = Random::randDouble(ws + 1, -1.0, 1.0);
                    double dy = Random::randDouble(ws + 2, -1.0, 1.0);
                    double dz = Random::randDouble(ws + 3, -1.0, 1.0);
                    double dn = sqrt(dx * dx + dy * dy + dz * dz) + 1e-9;
                    dx /= dn; dy /= dn; dz /= dn;
                    auto start = make_shared<StellarCoordinate>();
                    start->quadrant_x = path[p][0];
                    start->quadrant_y = path[p][1];
                    start->quadrant_z = path[p][2];
                    auto end = make_shared<StellarCoordinate>();
                    end->quadrant_x = path[p][0] + (int64_t)lround(dx * sub_len);
                    end->quadrant_y = path[p][1] + (int64_t)lround(dy * sub_len);
                    end->quadrant_z = path[p][2] + (int64_t)lround(dz * sub_len);
                    int knots = Random::randInt(ws + 4, 3, 5);
                    int64_t step_size = max((int64_t)1, (int64_t)lround(sub_len / max(1, knots)));
                    auto wisp = meander_walk(start, end, step_size, 0.5, 0.5, 3.0, 5.0,
                                             ws + 5, knots,
                                             -BIG, -BIG, -BIG, BIG, BIG, BIG);
                    for (auto& pt : wisp) {
                        Nebula::FilamentPoint fp;
                        fp.x = pt->quadrant_x; fp.y = pt->quadrant_y; fp.z = pt->quadrant_z;
                        fp.cloud_size = (float)(nebula->radius_ly * Random::randDouble(ws + 6 + idx, 0.04, 0.15));
                        nebula->filament_points.push_back(fp);
                        ++idx;
                    }
                }
            }
        }
    }

    // Weighted mass ratios for distributing an object's mass across children
    static vector<double> weighted_ratios(int64_t seed, int count, double lo, double hi) {
        vector<double> weights(count);
        for (int i = 0; i < count; i++) {
            weights[i] = Random::randDouble(seed + i + 100, lo, hi);
        }
        return weights;
    }

    // Even-stride sampling so any count is capped at `cap` without RNG
    static size_t even_stride(size_t count, size_t cap) {
        return max<size_t>(1, (count + cap - 1) / cap);
    }

    static void generate_galaxy_super_clusters(
        shared_ptr<Galaxy> galaxy
    ) {
        int64_t gs = Random::seed_from_coordinates(
            galaxy->location->quadrant_x,
            galaxy->location->quadrant_y,
            galaxy->location->quadrant_z
        );

        int num_clusters = max((int64_t)1, galaxy->mass / SpaceGen::AVG_SOLAR_MASSES_PER_SUPER_CLUSTER);

        vector<double> cluster_weights(num_clusters);
        double weight_sum = 0;
        for (int c = 0; c < num_clusters; c++) {
            cluster_weights[c] = Random::randDouble(gs + c + 1000, 0.01, 1.0);
            weight_sum += cluster_weights[c];
        }

        for (int c = 0; c < num_clusters; c++) {
            int64_t cs = gs + c * 7 + 13;

            double x_ly, y_ly, z_ly;
            galaxy_spiral_pos(galaxy, cs, x_ly, y_ly, z_ly, 0.2, 2.5, 0.5, 0.2, 0.1, 2.0);

            auto cluster_pos = offset_coordinate_ly(galaxy->location, x_ly, y_ly, z_ly);

            auto cluster = make_shared<SuperCluster>();
            cluster->location = cluster_pos;
            cluster->parent = galaxy;
            cluster->mass = (double)galaxy->mass * cluster_weights[c] / weight_sum;
            cluster->radius_ly = pow(cluster->mass / 1e10, 1.0 / 3.0) * 3000.0;
            // Inherit galaxy gas with scatter so a few superclusters come out gas-rich (bright spots)
            cluster->gas_fraction = min(1.0, galaxy->gas_fraction * Random::randDouble(cs + 30, 0.2, 1.5));

            Index3 quad_idx = {cluster_pos->quadrant_x, cluster_pos->quadrant_y, cluster_pos->quadrant_z};
            galaxy->super_clusters[quad_idx] = cluster;
        }
    }

    static void generate_galaxy_nebulae(
        shared_ptr<Galaxy> galaxy
    ) {
        int64_t gs = Random::seed_from_coordinates(
            galaxy->location->quadrant_x,
            galaxy->location->quadrant_y,
            galaxy->location->quadrant_z
        );

        // Nebula richness scales with galaxy gas: gas-rich spirals get many more than today,
        // gas-poor ellipticals almost none
        int num_nebulae = (int)lround(galaxy->gas_fraction * Random::randDouble(gs + 40, 200, 600));

        for (int n = 0; n < num_nebulae; n++) {
            int64_t ns = gs + n * 11 + 50;

            // Position within the galaxy
            double x_ly, y_ly, z_ly;
            galaxy_spiral_pos(galaxy, ns, x_ly, y_ly, z_ly, 0.2, 2.5, 0.5, 0.2, 0.1, 2.0);

            auto nebula_center = offset_coordinate_ly(galaxy->location, x_ly, y_ly, z_ly);

            auto nebula = make_shared<Nebula>();
            nebula->generation_seed = ns;
            nebula->center = nebula_center;
            nebula->parent = galaxy;
            // Larger nebulae in gas-rich galaxies
            nebula->radius_ly = galaxy->scale_length * Random::randDouble(ns + 4, 0.05, 0.25)
                * (0.5 + galaxy->gas_fraction);

            // Color theme for this nebula
            int theme = Random::randInt(ns + 5, 0, 5);
            float base_r, base_g, base_b;
            if (theme == 0) {
                base_r = 0.8f; base_g = 0.15f; base_b = 0.08f;  // muted red HII
            } else if (theme == 1) {
                base_r = 0.2f; base_g = 0.3f; base_b = 0.7f;   // muted blue reflection
            } else if (theme == 2) {
                base_r = 0.25f; base_g = 0.22f; base_b = 0.18f; // gray dust
            } else if (theme == 3) {
                base_r = 0.5f; base_g = 0.35f; base_b = 0.15f; // brown dust
            } else if (theme == 4) {
                base_r = 0.15f; base_g = 0.5f; base_b = 0.35f; // teal/green
            } else {
                base_r = 0.5f; base_g = 0.2f; base_b = 0.6f;   // purple
            }
            // Per-nebula brightness so some nebulae come out dark, some vivid.
            // Broader, darker range so most read as genuinely dim.
            float neb_dark = Random::randDouble(ns + 7, 0.04, 0.75);

            // Per-nebula filament spec so shapes read as a web of cloud threads,
            // never packed blobs. Derived from ns (not per-point seed) so the
            // block-gen LOD path re-scatters with the identical shape.
            double fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz;
            random_filament_axis(ns, fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz);
            nebula->strands       = Random::randInt(ns + 23, 1, 4);
            nebula->along_extent  = Random::randDouble(ns + 24, 0.6, 1.5);
            nebula->meander_amp   = nebula->along_extent * Random::randDouble(ns + 27, 0.3, 0.8);
            nebula->meander_knots = Random::randInt(ns + 28, 20, 40);
            nebula->wavy_seed     = ns + 31;

            nebula->base_r = base_r; nebula->base_g = base_g; nebula->base_b = base_b;
            nebula->neb_dark = neb_dark;
            nebula->fax = fax; nebula->fay = fay; nebula->faz = faz;
            nebula->fbx = fbx; nebula->fby = fby; nebula->fbz = fbz;
            nebula->fcx = fcx; nebula->fcy = fcy; nebula->fcz = fcz;

            // Skeleton: meandering main filaments + short branching sub-strands.
            // Every path point becomes a cloud anchor (with per-point cloud size)
            // that block-gen turns into a soft cloud patch.
            generate_nebula_strand_paths(nebula, ns);
            generate_nebula_sub_strands(nebula, ns);

            galaxy->nebulae.push_back(nebula);
        }
    }

    // ─── SuperCluster ─────────────────────────────────────────────

    static void generate_super_cluster(shared_ptr<SuperCluster> super_cluster) {
        int64_t cs = seed_from_coordinate(super_cluster->location);

        int num_clusters = Random::randInt(cs, 1, 1000);

        vector<double> cluster_weights = weighted_ratios(cs, num_clusters, 0.01, 1.0);
        double weight_sum = 0;
        for (double w : cluster_weights) weight_sum += w;
        double total_mass = super_cluster->mass;
        super_cluster->mass = 0;

        // Filament cloud, not a box: spread scales with radius, dense core via falloff
        double spread = max(50.0, super_cluster->radius_ly * 2.0);
        double fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz;
        random_filament_axis(cs, fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz);

        for (int c = 0; c < num_clusters; c++) {
            double dx, dy, dz;
            scatter_filament_point(cs + c * 5 + 1, spread, fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz, dx, dy, dz);
            double fall = Random::randDouble(cs + c * 5 + 4, 0.0, 1.0);
            fall *= fall;
            dx *= fall; dy *= fall; dz *= fall;

            auto cluster_pos = offset_coordinate_ly(super_cluster->location, dx, dy, dz);

            auto cluster = make_shared<Cluster>();
            cluster->location = cluster_pos;
            cluster->parent = super_cluster;
            cluster->mass = total_mass * cluster_weights[c] / weight_sum;
            cluster->radius_ly = pow(cluster->mass / 1e7, 1.0 / 3.0) * 100.0;
            // Inherit supercluster gas with scatter so occasional clusters are bright spots
            cluster->gas_fraction = min(1.0, super_cluster->gas_fraction * Random::randDouble(cs + c * 13 + 70, 0.3, 2.0));
            super_cluster->mass += cluster->mass;

            Index3 quad_idx = {cluster_pos->quadrant_x, cluster_pos->quadrant_y, cluster_pos->quadrant_z};
            if (!super_cluster->clusters.has(quad_idx)) {
                super_cluster->clusters[quad_idx] = Grid<shared_ptr<Cluster>>();
            }
            Index3 ly_idx = {(int64_t)cluster_pos->light_year_x, (int64_t)cluster_pos->light_year_y, (int64_t)cluster_pos->light_year_z};
            super_cluster->clusters[quad_idx][ly_idx] = cluster;
        }
    }

    // ─── Cluster ──────────────────────────────────────────────────

    static void generate_cluster(shared_ptr<Cluster> cluster) {
        int64_t cs = seed_from_coordinate(cluster->location);

        int num_cradles = Random::randInt(cs, 1, 1000);

        // Distribute cluster mass across cradles
        vector<double> cradle_weights = weighted_ratios(cs, num_cradles, 0.01, 1.0);
        double weight_sum = 0;
        for (double w : cradle_weights) weight_sum += w;
        double total_mass = cluster->mass;
        cluster->mass = 0;

        // Filament cloud like superclusters: spread scales with radius, dense core via falloff
        double spread = max(50.0, cluster->radius_ly * 2.0);
        double fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz;
        random_filament_axis(cs, fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz);

        for (int cr = 0; cr < num_cradles; cr++) {
            double dx, dy, dz;
            scatter_filament_point(cs + cr * 5 + 1, spread, fax, fay, faz, fbx, fby, fbz, fcx, fcy, fcz, dx, dy, dz);
            double fall = Random::randDouble(cs + cr * 5 + 4, 0.0, 1.0);
            fall *= fall;
            dx *= fall; dy *= fall; dz *= fall;

            auto cradle_pos = offset_coordinate_ly(cluster->location, dx, dy, dz);

            auto cradle = make_shared<StellarCradle>();
            cradle->location = cradle_pos;
            cradle->parent = cluster;
            cradle->mass = total_mass * cradle_weights[cr] / weight_sum;
            cradle->radius_ly = max(5.0, pow(cradle->mass / 1e4, 1.0 / 3.0) * 5.0);
            cluster->mass += cradle->mass;
            // Bright spot = massive stars: flag cradles whose systems are heavy enough to spawn giants
            int cradle_systems = Random::randInt(seed_from_coordinate(cradle_pos), 1, 30);
            cradle->has_massive_stars = cradle->mass / (double)cradle_systems >= 1.2;
            cluster->stellar_cradles.push_back(cradle);
        }
        cluster->massive_star_fraction = cluster->stellar_cradles.empty()
            ? 0.0
            : (double)std::count_if(cluster->stellar_cradles.begin(), cluster->stellar_cradles.end(),
                [](const shared_ptr<StellarCradle>& cr) { return cr && cr->has_massive_stars; })
                / (double)cluster->stellar_cradles.size();
    }

    // ─── StellarCradle ────────────────────────────────────────────

    static void generate_stellar_cradle(shared_ptr<StellarCradle> cradle) {
        int64_t cs = seed_from_coordinate(cradle->location);

        int num_systems = Random::randInt(cs, 1, 30);

        // Distribute cradle mass across solar systems
        vector<double> sys_weights = weighted_ratios(cs, num_systems, 0.5, 2.0);
        double weight_sum = 0;
        for (double w : sys_weights) weight_sum += w;
        double total_mass = cradle->mass;
        cradle->mass = 0;

        for (int s = 0; s < num_systems; s++) {
            double dx = Random::randDouble(cs + s * 3 + 1, -5.0, 5.0);
            double dy = Random::randDouble(cs + s * 3 + 2, -5.0, 5.0);
            double dz = Random::randDouble(cs + s * 3 + 3, -5.0, 5.0);

            auto sys_pos = offset_coordinate(cradle->location, dx, dy, dz);

            double sys_mass = total_mass * sys_weights[s] / weight_sum;
            auto sys = generate_solar_system(seed_from_coordinate(sys_pos), sys_pos, sys_mass);
            sys->parent = cradle;
            cradle->solar_systems.push_back(sys);
            cradle->mass += sys->mass;
        }
    }

    // ─── Solar System ─────────────────────────────────────────────

    static shared_ptr<SolarSystem> generate_solar_system(
        int64_t seed,
        shared_ptr<StellarCoordinate> location,
        double mass = 0)
    {
        auto system = make_shared<SolarSystem>();
        system->location = location;

        system->mass = mass > 0 ? mass : Random::randDouble(seed, 0.5, 2.0);
        system->radius_ly = 0.02 + system->mass * 0.04;

        const double AU_KM = 1.495978707e8;
        // km offset along the orbital plane (x,z), slight out-of-plane (y)
        auto orbit_km = [&](int64_t s, double lo_au, double hi_au) {
            double r_km = Random::randDouble(s, lo_au, hi_au) * AU_KM;
            double th = Random::randDouble(s + 1, 0.0, 2.0 * M_PI);
            double tilt = Random::randDouble(s + 2, -0.05, 0.05);
            return array<double, 3>{r_km * cos(th), r_km * tilt, r_km * sin(th)};
        };

        // 1 or 2 stars
        int num_stars = Random::randInt(seed + 1, 0, 1, 3);
        for (int i = 0; i < num_stars; i++) {
            auto star = make_shared<Star>();
            star->parent = system;
            star->mass = system->mass / num_stars * Random::randDouble(seed + i * 10 + 2, 0.8, 1.2);
            star->age  = Random::randDouble(seed + i * 10 + 3, 0.1, 13.0);

            star->metallicity    = Random::randInt(seed + i * 10 + 4, 0, 100);
            star->rotation_speed = Random::randInt(seed + i * 10 + 5, 0, 100);
            star->multiplicity   = Random::randInt(seed + i * 10 + 6, 0, 100);
            // primary star at the system origin; companions orbit a few AU away
            auto km = (i == 0) ? array<double, 3>{0.0, 0.0, 0.0}
                               : orbit_km(seed + i * 10 + 8, 0.1, 8.0);
            star->location = offset_coordinate(
                location, km[0] / KM_PER_LY, km[1] / KM_PER_LY, km[2] / KM_PER_LY);

            // ── Determine type from mass, age, and metallicity ──
            //
            // Approximate main-sequence lifespan (Gyr).
            //   Sun  (1.0 M☉)  ≈ 10 Gyr
            //   0.5 M☉         ≈ 40 Gyr
            //   2.0 M☉         ≈ 2.5 Gyr
            //   8.0 M☉         ≈ 0.16 Gyr
            double lifespan = 10.0 / (star->mass * star->mass);

            if (star->age > lifespan) {
                // ── Post-main-sequence ──
                if (star->mass >= 8.0) {
                    // Core collapse → compact remnant
                    star->type = Random::randBool(Random::adjustedSeed(seed + i * 10 + 7), 50)
                        ? Star::StarType::NEUTRON_STAR
                        : Star::StarType::BLACK_HOLE;
                } else if (star->age > lifespan + 1.0) {
                    // Past the red-giant phase → degenerate remnant
                    star->type = star->age > lifespan + 5.0
                        ? Star::StarType::BLACK_DWARF   // fully cooled
                        : Star::StarType::WHITE_DWARF;
                } else {
                    // Currently in the giant phase
                    star->type = star->mass < 1.5
                        ? Star::StarType::RED_GIANT
                        : Star::StarType::PURPLE_GIANT;
                }
            } else {
                // ── Main sequence — classify by mass ──
                // Lower metallicity → slightly hotter (shift mass boundary down)
                double met_shift = (50.0 - star->metallicity) / 100.0 * 0.3;
                double eff_mass = star->mass + met_shift;

                if      (eff_mass < 0.5)  star->type = Star::StarType::RED_DWARF;
                else if (eff_mass < 0.8)  star->type = Star::StarType::ORANGE;
                else if (eff_mass < 1.2)  star->type = Star::StarType::YELLOW;
                else if (eff_mass < 3.0)  star->type = Star::StarType::WHITE_GIANT;
                else                      star->type = Star::StarType::BLUE_GIANT;
            }

            // Glow radius in LY, varies by type (1e-4 – 1e-2)
            switch (star->type) {
                case Star::StarType::RED_DWARF:
                case Star::StarType::WHITE_DWARF:
                case Star::StarType::BLACK_DWARF:
                case Star::StarType::NEUTRON_STAR:
                case Star::StarType::BLACK_HOLE:
                    star->radius_ly = 2e-4;
                    break;
                case Star::StarType::ORANGE:
                    star->radius_ly = 4e-4;
                    break;
                case Star::StarType::YELLOW:
                    star->radius_ly = 5e-4;
                    break;
                case Star::StarType::WHITE_GIANT:
                    star->radius_ly = 1.5e-3;
                    break;
                case Star::StarType::BLUE_GIANT:
                    star->radius_ly = 2e-3;
                    break;
                case Star::StarType::PURPLE_GIANT:
                    star->radius_ly = 2.5e-3;
                    break;
                case Star::StarType::RED_GIANT:
                    star->radius_ly = 3e-3;
                    break;
            }

            system->stars.push_back(star);
        }

        // Planets
        int num_planets = Random::randInt(seed + 20, 0, 8);
        if (num_stars = 0) num_planets = Random::randInt(seed + 65, 1, 1, 3);
        for (int p = 0; p < num_planets; p++) {
            // each planet orbits the primary at its own radius in the orbital plane
            auto km = orbit_km(seed + p * 100 + 40, 0.3, 30.0);
            auto planet_loc = offset_coordinate(
                location, km[0] / KM_PER_LY, km[1] / KM_PER_LY, km[2] / KM_PER_LY);
            auto planet = make_shared<Planet>(
                seed + p * 100 + 30,
                planet_loc
            );
            planet->parent = system;
            system->planets.push_back(planet);
        }

        return system;
    }




    // ══════════════════════════════════════════════════════════════
    // ─── Block generation — translate cosmic objects → blocks ───
    // ══════════════════════════════════════════════════════════════

    struct ShapeRefs {
        vector<double> a;
        vector<double> b;
        vector<double> c;
    };

    enum class GenTier : int {
        GALAXY         = 0,
        SUPER_CLUSTER  = 1,
        CLUSTER        = 2,
        STELLAR_CRADLE = 3,
        SOLAR_SYSTEM   = 4,
        NEBULA         = 5,
        BLACK_HOLE     = 6
    };

    struct BlockGroup {
        string   source_key;
        GenTier  tier;
        double   dist_ly;
        vector<shared_ptr<Block>> blocks;
    };

    // ─── Shared helpers (move from SkyGen) ─────────────────────

    static void ly_offset(
        shared_ptr<StellarCoordinate> from,
        shared_ptr<StellarCoordinate> to,
        double& lx, double& ly, double& lz
    ) {
        lx = (to->quadrant_x - from->quadrant_x) * 10000.0
           + (int)to->light_year_x - (int)from->light_year_x
           + ((double)(int64_t)to->km_x - (int64_t)from->km_x) / KM_PER_LY;
        ly = (to->quadrant_y - from->quadrant_y) * 10000.0
           + (int)to->light_year_y - (int)from->light_year_y
           + ((double)(int64_t)to->km_y - (int64_t)from->km_y) / KM_PER_LY;
        lz = (to->quadrant_z - from->quadrant_z) * 10000.0
           + (int)to->light_year_z - (int)from->light_year_z
           + ((double)(int64_t)to->km_z - (int64_t)from->km_z) / KM_PER_LY;
    }

    static double viewer_dist(
        shared_ptr<StellarCoordinate> from,
        shared_ptr<StellarCoordinate> to,
        double& ox, double& oy, double& oz
    ) {
        ly_offset(from, to, ox, oy, oz);
        return sqrt(ox*ox + oy*oy + oz*oz);
    }

    // Base coordinate plus a LY offset, normalized back into quadrant/light-year/km space
    static shared_ptr<StellarCoordinate> offset_coordinate(
        shared_ptr<StellarCoordinate> base,
        double dx, double dy, double dz
    ) {
        auto coord = make_shared<StellarCoordinate>();
        // Fold the base km fraction + the (possibly fractional) LY offset into a single
        // shifted value, then split into an integer-LY carry and a km remainder.
        auto decompose = [](int64_t base_q, uint16_t base_ly, uint64_t base_km, double offset) -> tuple<int64_t, uint16_t, uint64_t> {
            double shifted = (double)base_km / KM_PER_LY + offset;
            int64_t ly_carry = (int64_t)floor(shifted);
            double frac = shifted - (double)ly_carry;
            int64_t km = (int64_t)llround(frac * KM_PER_LY);
            if (km >= (int64_t)KM_PER_LY) { km -= (int64_t)KM_PER_LY; ly_carry++; }
            else if (km < 0)              { km += (int64_t)KM_PER_LY; ly_carry--; }
            int64_t total = (int64_t)base_ly + ly_carry;
            int64_t q_off = floor_div(total, (int64_t)10000);
            return {base_q + q_off, (uint16_t)(total - q_off * 10000), (uint64_t)km};
        };
        auto [qx, lx, kx] = decompose(base->quadrant_x, base->light_year_x, base->km_x, dx);
        auto [qy, ly, ky] = decompose(base->quadrant_y, base->light_year_y, base->km_y, dy);
        auto [qz, lz, kz] = decompose(base->quadrant_z, base->light_year_z, base->km_z, dz);
        coord->quadrant_x = qx; coord->light_year_x = lx; coord->km_x = kx;
        coord->quadrant_y = qy; coord->light_year_y = ly; coord->km_y = ky;
        coord->quadrant_z = qz; coord->light_year_z = lz; coord->km_z = kz;
        return coord;
    }

    // Integer-LY-only variant (km always 0): used for everything at solar-system
    // scale and larger, which sit at whole-light-year offsets.
    static shared_ptr<StellarCoordinate> offset_coordinate_ly(
        shared_ptr<StellarCoordinate> base,
        double dx, double dy, double dz
    ) {
        auto coord = make_shared<StellarCoordinate>();
        auto decompose = [](int64_t base_q, uint16_t base_ly, double offset) -> pair<int64_t, uint16_t> {
            int64_t total = (int64_t)base_ly + (int64_t)offset;
            int64_t q_off = floor_div(total, (int64_t)10000);
            return {base_q + q_off, (uint16_t)(total - q_off * 10000)};
        };
        auto [qx, lx] = decompose(base->quadrant_x, base->light_year_x, dx);
        auto [qy, ly] = decompose(base->quadrant_y, base->light_year_y, dy);
        auto [qz, lz] = decompose(base->quadrant_z, base->light_year_z, dz);
        coord->quadrant_x = qx; coord->light_year_x = lx; coord->km_x = 0;
        coord->quadrant_y = qy; coord->light_year_y = ly; coord->km_y = 0;
        coord->quadrant_z = qz; coord->light_year_z = lz; coord->km_z = 0;
        return coord;
    }

    static ShapeRefs get_orient_offsets(
        shared_ptr<StellarCoordinate> view_pos,
        const Galaxy& galaxy,
        double obj_radius,
        shared_ptr<StellarCoordinate> obj_pos
    ) {
        double base_x, base_y, base_z;
        ly_offset(view_pos, obj_pos, base_x, base_y, base_z);

        double scale = obj_radius / max(galaxy.scale_length * 3.0, 1.0);

        ShapeRefs refs;
        refs.a = {base_x + galaxy.orient_a_ly[0] * scale,
                  base_y + galaxy.orient_a_ly[1] * scale,
                  base_z + galaxy.orient_a_ly[2] * scale};
        refs.b = {base_x + galaxy.orient_b_ly[0] * scale,
                  base_y + galaxy.orient_b_ly[1] * scale,
                  base_z + galaxy.orient_b_ly[2] * scale};
        refs.c = {base_x + galaxy.orient_c_ly[0] * scale,
                  base_y + galaxy.orient_c_ly[1] * scale,
                  base_z + galaxy.orient_c_ly[2] * scale};
        return refs;
    }

    // ─── Conversion helpers ────────────────────────────────────

    static shared_ptr<Block> make_space_block(
        const string& material,
        double game_x, double game_y, double game_z,
        double block_size
    ) {
        auto pos_double = make_shared<PositionDouble>(game_x, game_y, game_z);
        return make_shared<Block>(material, nullptr, pos_double, block_size);
    }

    static int sphere_radius_blocks(double angular_radius, int maximum) {
        // Angular size, rather than the old universal block scale, controls
        // how much geometry an object deserves in the local render frame.
        double t = clamp(angular_radius / (M_PI * 0.5), 0.0, 1.0);
        return clamp((int)llround(t * maximum), MIN_SPHERE_RADIUS_BLOCKS, maximum);
    }

    static vector<shared_ptr<Block>> generate_space_sphere(
        double radius_ly,
        int radius_blocks,
        const string& material,
        int max_blocks = MAX_BLOCKS
    ) {
        vector<shared_ptr<Block>> blocks;
        radius_blocks = max(radius_blocks, 1);
        double block_size_ly = radius_ly / (double)radius_blocks;
        int last_r = 0;

        for (int y = -radius_blocks; y <= radius_blocks && (int)blocks.size() < max_blocks; ++y) {
            double r_sq = (double)radius_blocks * radius_blocks - (double)y * y;
            if (r_sq <= 0.0) continue;
            int r = (int)ceil(sqrt(r_sq));
            vector<shared_ptr<Block>> ring;
            PartContour::add_blocks_with_midpoint_circle_algorithm(
                y, r, r, block_size_ly, 0, 0, ring, "y", max(1, abs(r - last_r)));
            last_r = r;

            for (auto& block : ring) {
                if ((int)blocks.size() >= max_blocks) break;
                block->material = material;
                blocks.push_back(block);
            }
        }
        return blocks;
    }

    static void translate_blocks(
        vector<shared_ptr<Block>>& blocks,
        double x, double y, double z
    ) {
        for (auto& block : blocks) {
            if (!block || !block->position_double) continue;
            block->position_double->x += x;
            block->position_double->y += y;
            block->position_double->z += z;
        }
    }

    static double angular_radius_rad(double object_radius_ly, double dist_ly) {
        if (dist_ly <= 0.0 || object_radius_ly >= dist_ly)
            return M_PI * 0.5;
        return asin(clamp(object_radius_ly / dist_ly, 0.0, 1.0));
    }

    static string object_key(GenTier tier, shared_ptr<StellarCoordinate> pos) {
        return format("{}:{}:{}:{}", (int)tier, pos->quadrant_x, pos->quadrant_y, pos->quadrant_z);
    }

    // ─── Spiral arm shared helpers ─────────────────────────────

    struct SpiralArmPoint {
        int arm_idx;
        int seg_idx;
        double t;           // normalized [0,1] along arm
        double r;           // radius from galaxy center (LY)
        double theta;       // angle in disc plane (rad)
        double px, py, pz;  // position in disc plane (LY from center)
        double tx, ty, tz;  // tangent direction at this point
        int64_t seed;       // per-segment seed for random variation
    };

    static pair<double, double> galaxy_arm_radii(const Galaxy& galaxy) {
        double inner_r = galaxy.bar_length > 0.0
            ? galaxy.bar_length * galaxy.scale_length * 1.2
            : galaxy.scale_length * 0.3;
        double outer_r = galaxy.radius_ly > 0.0
            ? galaxy.radius_ly
            : galaxy.scale_length * 2.5;
        return {inner_r, outer_r};
    }

    static double compute_bar_angle(
        const Galaxy& galaxy,
        double ux, double uy, double uz,
        double vx, double vy, double vz,
        int64_t seed
    ) {
        if (galaxy.bar_length > 0.0) {
            double bdu = galaxy.bar_direction_ly[0]*ux + galaxy.bar_direction_ly[1]*uy + galaxy.bar_direction_ly[2]*uz;
            double bdv = galaxy.bar_direction_ly[0]*vx + galaxy.bar_direction_ly[1]*vy + galaxy.bar_direction_ly[2]*vz;
            return atan2(bdv, bdu);
        }
        return Random::randDouble(seed + 50, 0.0, 2.0 * M_PI);
    }

    static void iterate_spiral_arms(
        const Galaxy& galaxy,
        double bar_angle,
        int points_per_arm,
        int64_t seed,
        function<void(const SpiralArmPoint&)> callback
    ) {
        auto [inner_r, outer_r] = galaxy_arm_radii(galaxy);

        for (int a = 0; a < galaxy.arm_count; a++) {
            double arm_base = bar_angle + 2.0 * M_PI * a / galaxy.arm_count;
            for (int s = 0; s < points_per_arm; s++) {
                double t = (s + 0.5) / points_per_arm;
                double r = inner_r + (outer_r - inner_r) * t;
                double wind_shift = galaxy.arm_winding * (r / galaxy.scale_length);
                double noise = Random::randDouble(seed + a * 100 + s * 7, -0.3, 0.3);
                double theta = arm_base + wind_shift + noise;

                double px, py, pz, tx, ty, tz;
                galaxy_disc_point(galaxy, r, theta, px, py, pz, &tx, &ty, &tz);

                callback({a, s, t, r, theta, px, py, pz, tx, ty, tz, seed + a * 100 + s * 7});
            }
        }
    }

    // ─── Per-object generators ─────────────────────────────────

    static void generate_galaxy_blocks(
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        double gx, gy, gz;
        double gdist = viewer_dist(view_pos, galaxy->location, gx, gy, gz);
        if (gdist < 1.0) return;

        // Sub-structure drives the galaxy's blocks: a block per supercluster
        // plus nebula point clouds. Self-generate if the walk hasn't (distant galaxies).
        if (galaxy->super_clusters.size() == 0)
            generate_galaxy_super_clusters(galaxy);
        if (galaxy->nebulae.empty())
            generate_galaxy_nebulae(galaxy);

        auto place = [&](const string& material, double x_ly, double y_ly, double z_ly) {
            double px = round((gx + x_ly) / block_size_ly) * block_size_ly;
            double py = round((gy + y_ly) / block_size_ly) * block_size_ly;
            double pz = round((gz + z_ly) / block_size_ly) * block_size_ly;
            out_blocks.push_back(make_space_block(material, px, py, pz, block_size_ly));
        };

        for (auto& [qk, sc] : galaxy->super_clusters) {
            if (!sc || !sc->location) continue;
            double dx, dy, dz;
            ly_offset(galaxy->location, sc->location, dx, dy, dz);
            string mat = (sc->gas_fraction >= 0.5) ? BMaterial::GALAXY_GLOW : BMaterial::NEBULA_GAS;
            place(mat, dx, dy, dz);
        }

        const int NEBULA_MARKER_BUDGET = 600;
        int placed = 0;
        for (auto& nebula : galaxy->nebulae) {
            if (!nebula) continue;
            // Marker material from the nebula's overall brightness (same rule as
            // block gen) — per-point colors are not stored anymore.
            string mat = (nebula->base_r + nebula->base_g + nebula->base_b) * nebula->neb_dark < 0.5
                ? BMaterial::NEBULA_DUST : BMaterial::NEBULA_GAS;
            double nx, ny, nz;
            ly_offset(galaxy->location, nebula->center, nx, ny, nz);
            for (auto& fp : nebula->filament_points) {
                if (placed >= NEBULA_MARKER_BUDGET) break;
                place(mat, nx + fp.x, ny + fp.y, nz + fp.z);
                ++placed;
            }
        }
    }

    static void generate_super_cluster_blocks(
        shared_ptr<SuperCluster> sc,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        double scx, scy, scz;
        double scdist = viewer_dist(view_pos, sc->location, scx, scy, scz);
        if (scdist < 1.0) return;

        if (sc->clusters.size() == 0)
            generate_super_cluster(sc);

        // One block per cluster at its actual position
        vector<shared_ptr<Cluster>> clusters;
        for (auto& [cqk, cluster_grid] : sc->clusters) {
            for (auto& [lyk, cluster] : cluster_grid) {
                if (cluster && cluster->location) clusters.push_back(cluster);
            }
        }

        // Sparse by default: block budget scales with gas richness
        double intensity = min(1.0, max(0.0, sc->gas_fraction));
        size_t eff_cap = (size_t)(20 + 180 * intensity);
        size_t count = clusters.size();
        size_t step = even_stride(count, eff_cap);
        for (size_t i = 0; i < count; i += step) {
            auto& cluster = clusters[i];
            double dx, dy, dz;
            ly_offset(sc->location, cluster->location, dx, dy, dz);
            // Bright spot = a gas-rich cluster; gas-poor ones stay dim
            double cgas = cluster ? cluster->gas_fraction : 0.0;
            string mat = (cgas >= 0.5) ? BMaterial::GALAXY_GLOW : BMaterial::NEBULA_GAS;
            out_blocks.push_back(make_space_block(mat, scx + dx, scy + dy, scz + dz, block_size_ly));
        }
    }

    static void generate_cluster_blocks(
        shared_ptr<Cluster> cluster,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        double clx, cly, clz;
        double cldist = viewer_dist(view_pos, cluster->location, clx, cly, clz);
        if (cldist < 1.0) return;

        if (cluster->stellar_cradles.empty())
            generate_cluster(cluster);

        // One block per stellar cradle at its actual position
        vector<shared_ptr<StellarCradle>> cradles;
        for (auto& cradle : cluster->stellar_cradles) {
            if (cradle && cradle->location) cradles.push_back(cradle);
        }

        // Sparse by default; bright spots are cradles hosting massive stars
        double intensity = min(1.0, max(0.0, cluster->massive_star_fraction));
        size_t eff_cap = (size_t)(15 + 185 * intensity);
        size_t count = cradles.size();
        size_t step = even_stride(count, eff_cap);
        for (size_t i = 0; i < count; i += step) {
            auto& cradle = cradles[i];
            double dx, dy, dz;
            ly_offset(cluster->location, cradle->location, dx, dy, dz);
            string mat = (cradle && cradle->has_massive_stars) ? BMaterial::BLUE_STAR : BMaterial::NEBULA_GAS;
            out_blocks.push_back(make_space_block(mat, clx + dx, cly + dy, clz + dz, block_size_ly));
        }
    }

    static void generate_cradle_blocks(
        shared_ptr<StellarCradle> cradle,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        double crx, cry, crz;
        double crdist = viewer_dist(view_pos, cradle->location, crx, cry, crz);
        if (crdist < 1.0) return;

        if (cradle->solar_systems.empty())
            generate_stellar_cradle(cradle);

        // One block per solar system at its actual position (≤30 systems)
        size_t count = cradle->solar_systems.size();
        for (size_t i = 0; i < count; ++i) {
            auto& system = cradle->solar_systems[i];
            if (!system || !system->location) continue;
            double dx, dy, dz;
            ly_offset(cradle->location, system->location, dx, dy, dz);
            string mat = (i < 3) ? BMaterial::BLUE_STAR : BMaterial::NEBULA_GAS;
            out_blocks.push_back(make_space_block(mat, crx + dx, cry + dy, crz + dz, block_size_ly));
        }
    }

    static void generate_solar_system_blocks(
        shared_ptr<SolarSystem> system,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        for (auto& star : system->stars) {
            if (!star || !star->location) continue;

            double sx, sy, sz;
            double sdist = viewer_dist(view_pos, star->location, sx, sy, sz);
            if (sdist < 1.0) continue;

            out_blocks.push_back(make_space_block(BMaterial::BLUE_STAR, sx, sy, sz, block_size_ly));

            int64_t seed = seed_from_coordinate(star->location);
            for (int h = 0; h < 8; h++) {
                double hl = Random::randDouble(seed + h, 0.5, 2.0);
                double ht = Random::randDouble(seed + h + 1, 0.0, 2.0 * M_PI);
                double hp = Random::randDouble(seed + h + 2, 0.0, 2.0 * M_PI);
                double hx = sx + hl * sin(ht) * cos(hp);
                double hy = sy + hl * sin(ht) * sin(hp);
                double hz = sz + hl * cos(ht);
                out_blocks.push_back(make_space_block(BMaterial::BLUE_STAR, hx, hy, hz, block_size_ly * 0.5));
            }
        }
    }

    // Nebula LOD: block density scales with angular size — one block per
    // ANG_RES rad on the sky. ANG_RES is the sky-splat footprint: larger values
    // make each block cover more sky so nearby cloud splats overlap and merge
    // into soft gas instead of a field of discrete dots.
    static constexpr double NEBULA_ANG_RES = 0.002;
    static constexpr int NEBULA_MAX_BLOCKS = 3000;
    // Upper bound on blocks per cloud anchor (area-ratio LOD). 16 keeps a
    // resolved near cloud dense without letting one anchor blow the budget.
    static constexpr int CLOUD_MAX_PER_POINT = 16;

    static void generate_nebula_blocks(
        shared_ptr<Nebula> nebula,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        if (!nebula || !nebula->center) return;

        // Self-generate the skeleton if it hasn't been built (lazy path)
        if (nebula->filament_points.empty()) {
            generate_nebula_strand_paths(nebula, nebula->generation_seed);
            generate_nebula_sub_strands(nebula, nebula->generation_seed);
        }

        double nx, ny, nz;
        double ndist = viewer_dist(view_pos, nebula->center, nx, ny, nz);
        if (ndist < 1.0) return;

        // Block footprint: physical size near, sky-pixel size far away
        double step_ly = max(block_size_ly, ndist * NEBULA_ANG_RES);

        // Area-ratio LOD: each cloud anchor emits as many blocks as its cloud
        // covers at this footprint (n = (cloud_size/step_ly)^2), at least one.
        struct P { double x, y, z; float cloud; int n; };
        vector<P> pts;
        pts.reserve(nebula->filament_points.size());
        for (auto& fp : nebula->filament_points) {
            double px = nx + fp.x;
            double py = ny + fp.y;
            double pz = nz + fp.z;
            int n = clamp((int)lround(pow(fp.cloud_size / step_ly, 2.0)), 1, CLOUD_MAX_PER_POINT);
            pts.push_back({px, py, pz, fp.cloud_size, n});
        }
        if (pts.empty()) return;

        // Global budget: if the skeleton over-subscribes, drop the farthest
        // anchors (never below one) so the near, dense cloud wins.
        int total = 0;
        for (auto& p : pts) total += p.n;
        if (total > NEBULA_MAX_BLOCKS) {
            vector<size_t> order(pts.size());
            for (size_t i = 0; i < order.size(); i++) order[i] = i;
            sort(order.begin(), order.end(), [&](size_t a, size_t b) {
                double da = pts[a].x * pts[a].x + pts[a].y * pts[a].y + pts[a].z * pts[a].z;
                double db = pts[b].x * pts[b].x + pts[b].y * pts[b].y + pts[b].z * pts[b].z;
                return da > db;
            });
            // order is sorted farthest-first; keep the nearest `keep` anchors
            size_t keep = pts.size();
            while (total > NEBULA_MAX_BLOCKS && keep > 1) {
                total -= pts[order[keep - 1]].n;
                --keep;
            }
            vector<bool> drop(pts.size(), false);
            for (size_t i = keep; i < pts.size(); i++) drop[order[i]] = true;
            vector<P> kept;
            kept.reserve(keep);
            for (size_t i = 0; i < pts.size(); i++)
                if (!drop[i]) kept.push_back(pts[i]);
            pts.swap(kept);
        }

        // Emit blocks per anchor: center-biased scatter within the cloud size.
        int emitted = 0;
        for (size_t i = 0; i < pts.size(); i++) {
            auto& p = pts[i];
            int64_t bs = nebula->generation_seed + 200000 + (int64_t)i * 13;
            for (int k = 0; k < p.n && emitted < NEBULA_MAX_BLOCKS; k++) {
                int64_t ks = bs + k * 7919;
                double dx = Random::randDouble(ks, -1.0, 1.0);
                double dy = Random::randDouble(ks + 1, -1.0, 1.0);
                double dz = Random::randDouble(ks + 2, -1.0, 1.0);
                double dn = sqrt(dx * dx + dy * dy + dz * dz) + 1e-9;
                dx /= dn; dy /= dn; dz /= dn;
                // Center-biased radius: pow(rand,2) piles blocks near the anchor
                double rad = p.cloud * pow(Random::randDouble(ks + 4, 0.0, 1.0), 2.0);
                double bx = p.x + dx * rad;
                double by = p.y + dy * rad;
                double bz = p.z + dz * rad;
                float noise = (float)Random::randDouble(ks + 3, 0.0, 1.0);
                float sum = (nebula->base_r + nebula->base_g + nebula->base_b) * noise * nebula->neb_dark;
                string mat = sum < 0.5f ? BMaterial::NEBULA_DUST : BMaterial::NEBULA_GAS;
                out_blocks.push_back(make_space_block(mat, bx, by, bz, step_ly));
                ++emitted;
            }
        }
    }

    static void generate_black_hole_blocks(
        shared_ptr<StellarCoordinate> pos,
        shared_ptr<Galaxy> galaxy,
        shared_ptr<StellarCoordinate> view_pos,
        double block_size_ly,
        vector<shared_ptr<Block>>& out_blocks
    ) {
        double bx, by, bz;
        double bdist = viewer_dist(view_pos, pos, bx, by, bz);
        if (bdist < 1.0) return;

        double ux = 1, uy = 0, uz = 0, vx = 0, vy = 0, vz = 1, nx = 0, ny = 1, nz = 0;
        if (galaxy && galaxy->type == Galaxy::GalaxyType::SPIRAL) {
            galaxy_disc_basis(*galaxy, ux, uy, uz, vx, vy, vz, nx, ny, nz);
        }

        int64_t seed = seed_from_coordinate(pos);

        double core_r_ly = block_size_ly * 5.0;
        for (int i = 0; i < 50; i++) {
            double r = Random::randDouble(seed + i, 0.0, core_r_ly);
            double t = Random::randDouble(seed + i + 1, 0.0, 2.0 * M_PI);
            double p = Random::randDouble(seed + i + 2, 0.0, 2.0 * M_PI);
            out_blocks.push_back(make_space_block(BMaterial::OBSIDIAN,
                bx + r * sin(t) * cos(p),
                by + r * sin(t) * sin(p),
                bz + r * cos(t),
                core_r_ly * 0.3));
        }

        double disk_r_ly = block_size_ly * 15.0;
        for (int i = 0; i < 24; i++) {
            double a = 2.0 * M_PI * i / 24.0;
            double rvar = Random::randDouble(seed + i + 10, 0.8, 1.2);
            double vert = Random::randDouble(seed + i + 11, -0.1, 0.1);
            double lx = disk_r_ly * cos(a) * rvar * ux + disk_r_ly * sin(a) * rvar * vx + vert * nx;
            double ly = disk_r_ly * cos(a) * rvar * uy + disk_r_ly * sin(a) * rvar * vy + vert * ny;
            double lz = disk_r_ly * cos(a) * rvar * uz + disk_r_ly * sin(a) * rvar * vz + vert * nz;
            out_blocks.push_back(make_space_block(BMaterial::BLUE_STAR, bx + lx, by + ly, bz + lz, disk_r_ly * 0.3));
        }
    }

    // ─── Planet property generation ────────────────────────────

    static void generate_planet_properties(shared_ptr<Planet> planet) {
        int64_t seed = planet->seed;
        double mass_earth = Random::randDouble(seed, 0.1, 1.0, 4000.0);

        double density; // kg/m³
        if (mass_earth < 0.5) {
            density = 3000;
            planet->h2o = Random::randInt(seed + 1, 0, 10);
        } else if (mass_earth < 2.0) {
            density = 5500;
            planet->h2o = Random::randInt(seed + 1, 0, 20);
            planet->n2  = Random::randInt(seed + 2, 0, 80);
            planet->o2  = Random::randInt(seed + 3, 0, 20);
        } else if (mass_earth < 10.0) {
            density = 4000;
            planet->h2o = Random::randInt(seed + 1, 0, 30);
            planet->h2  = Random::randInt(seed + 2, 0, 50);
        } else if (mass_earth < 100.0) {
            density = 1500;
            planet->methane = Random::randInt(seed + 1, 0, 50);
            planet->h2      = Random::randInt(seed + 2, 0, 50);
        } else {
            density = 700;
            planet->h2 = Random::randInt(seed + 1, 0, 80);
            planet->he = Random::randInt(seed + 2, 0, 20);
        }

        double mass_kg = mass_earth * 5.97e24;
        double radius_m = pow(3.0 * mass_kg / (4.0 * M_PI * density), 1.0 / 3.0);

        planet->radius = radius_m;
        planet->worldSize = 2 * M_PI * radius_m;
        
        // ensure world size can be divided evenly by chunk sizes
        ChunkType type = Chunk::topTypeFor(planet->worldSize);
        planet->worldSize = ceil(planet->worldSize/(double)type) * (int64_t) type;
        planet->radius = planet->worldSize / (2 * M_PI);

        planet->seaLevel = (planet->h2o > 0 || planet->methane > 0) ? -5000 : -10000;

        // surface gravity from mass & radius; gates habitability below
        const double G = 6.674e-11;
        planet->gravity = G * mass_kg / (radius_m * radius_m);

        // broad habitable definition: under 15g life is possible; traits raise odds
        bool has_atmo = planet->n2 > 0 || planet->o2 > 0 || planet->h2 > 0 ||
                        planet->he > 0 || planet->methane > 0 || planet->h2o > 0;
        bool has_solvent = planet->h2o > 0 || planet->methane > 0;
        int habitability = 15;
        if (has_atmo)     habitability += 25;
        if (has_solvent)  habitability += 30;
        habitability += (int)planet->precipitation / 5; // wetter worlds favor life
        habitability = max(0, min(100, habitability));

        planet->hasLife = planet->gravity < 15.0 &&
                          Random::randBool(seed, habitability);
    }

    // ─── Main entry point ─────────────────────────────────────
    struct PlanetBackgroundSphere {
        double center_x_ly = 0.0;
        double center_y_ly = 0.0;
        double center_z_ly = 0.0;
        double radius_godot = 0.0;
        double block_size_godot = 0.0;
        double r = 0.5;
        double g = 0.5;
        double b = 0.5;
        double atmosphere_r = 0.25;
        double atmosphere_g = 0.55;
        double atmosphere_b = 1.0;
        double atmosphere_strength = 0.0;
    };

    static PlanetBackgroundSphere make_planet_background_sphere(
        const shared_ptr<Planet>& planet,
        const shared_ptr<RootChunk>& root,
        double center_x_ly,
        double center_y_ly,
        double center_z_ly,
        double radius_godot,
        double block_size_godot
    ) {
        PlanetBackgroundSphere result;
        result.center_x_ly = center_x_ly;
        result.center_y_ly = center_y_ly;
        result.center_z_ly = center_z_ly;
        result.radius_godot = radius_godot;
        result.block_size_godot = block_size_godot;

        unordered_map<string, array<double, 3>> material_colors;
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        int color_count = 0;

        for (const auto& chunk : root->subChunks) {
            if (!chunk || chunk->material.empty()) continue;

            auto color_it = material_colors.find(chunk->material);
            if (color_it == material_colors.end()) {
                int width = 0;
                int height = 0;
                int channels = 0;
                uint8_t* image = stbi_load(
                    BMaterial::path(chunk->material).c_str(),
                    &width,
                    &height,
                    &channels,
                    4
                );
                if (!image || width <= 0 || height <= 0) {
                    if (image) stbi_image_free(image);
                    continue;
                }

                double material_r = 0.0;
                double material_g = 0.0;
                double material_b = 0.0;
                size_t pixel_count = (size_t)width * (size_t)height;
                for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
                    material_r += image[pixel * 4 + 0] / 255.0;
                    material_g += image[pixel * 4 + 1] / 255.0;
                    material_b += image[pixel * 4 + 2] / 255.0;
                }
                stbi_image_free(image);

                color_it = material_colors.emplace(
                    chunk->material,
                    array<double, 3>{
                        material_r / pixel_count,
                        material_g / pixel_count,
                        material_b / pixel_count
                    }
                ).first;
            }

            r += color_it->second[0];
            g += color_it->second[1];
            b += color_it->second[2];
            ++color_count;
        }

        if (color_count > 0) {
            result.r = r / color_count;
            result.g = g / color_count;
            result.b = b / color_count;
        }

        // Gas composition controls the broad hue; pressure controls visibility.
        double blue = planet->o2 + planet->n2 + planet->h2o;
        double green = planet->methane + planet->chlorine;
        double warm = planet->dustLevel + planet->ammonia + planet->sulfer +
            planet->sulfericAcid + planet->bromine + planet->sodium_potassium + planet->iron;
        double gas_total = blue + green + warm;
        if (gas_total > 0.0) {
            result.atmosphere_r = (blue * 0.15 + green * 0.25 + warm * 0.9) / gas_total;
            result.atmosphere_g = (blue * 0.45 + green * 0.85 + warm * 0.45) / gas_total;
            result.atmosphere_b = (blue * 1.0 + green * 0.75 + warm * 0.2) / gas_total;
        }
        result.atmosphere_strength = clamp(
            (planet->atmosphericPressure + gas_total * 0.35) / 100.0, 0.0, 0.85
        );
        return result;
    }

    struct SpaceGenResult {
        vector<shared_ptr<Block>> blocks;
        float* sky_box; //  WARNING, this needs to be cleaned up manually
        shared_ptr<Planet> planet = nullptr; // if this is null then we're in space, otherwise we'll be doing terrain gen on the planet
        shared_ptr<PlanetBackgroundSphere> planet_background_sphere = nullptr;
        double game_scale = 1.0; // LY -> godot-unit factor used to build blocks; player movement converts back via this

        void clean_up() {
            delete[] sky_box;
        }
    };

    // ─── Priority-queue render candidate ────────────────────────

    struct RenderCandidate {
        enum Type : uint8_t {
            PLANET = 0,            // highest priority
            STAR = 1,
            SOLAR_SYSTEM = 2,
            STELLAR_CRADLE = 3,
            CLUSTER = 4,
            SUPER_CLUSTER = 5,
            NEBULA = 6,
            GALAXY = 7,            // lowest
        };

        Type type;
        double angular_radius;
        double ox, oy, oz;         // viewer-relative LY offset
        double dist_ly;            // distance from viewer (LY)
        double block_size_ly;      // LY size of one block in this object's grid

        shared_ptr<Galaxy> galaxy; // parent galaxy context

        shared_ptr<SuperCluster> super_cluster;
        shared_ptr<Cluster> cluster;
        shared_ptr<StellarCradle> cradle;
        shared_ptr<SolarSystem> solar_system;
        shared_ptr<Star> star;
        shared_ptr<Planet> planet;
        shared_ptr<Nebula> nebula;

        int sphere_radius_blocks = 0;
    };

    static double render_radius_for_candidate(const RenderCandidate& cand, double frame_scale) {
        double dist_godot = cand.dist_ly * frame_scale;
        double angle = min(cand.angular_radius, M_PI * 0.5 - 0.01);
        return max(0.001, dist_godot * tan(max(angle, 0.000001)));
    }

    static string glow_material(const RenderCandidate& c) {
        switch (c.type) {
            case RenderCandidate::GALAXY: return BMaterial::GALAXY_GLOW;
            case RenderCandidate::NEBULA: return BMaterial::NEBULA_GAS;
            default: return BMaterial::BLUE_STAR;
        }
    }

    static int estimate_blocks(RenderCandidate::Type type, int budget) {
        switch (type) {
            case RenderCandidate::GALAXY:         return min(budget, 500);
            case RenderCandidate::SUPER_CLUSTER:  return min(budget, 100);
            case RenderCandidate::CLUSTER:        return min(budget, 50);
            case RenderCandidate::STELLAR_CRADLE: return min(budget, 15);
            case RenderCandidate::SOLAR_SYSTEM:   return min(budget, 10);
            case RenderCandidate::STAR:           return min(budget, 1);
            case RenderCandidate::PLANET:         return min(budget, MAX_PLANET_BLOCKS);
            case RenderCandidate::NEBULA:         return min(budget, 100);
            default:                              return budget;
        }
    }

    static double planet_visible_surface_span(double radius_ly, double distance_ly) {
        if (radius_ly <= 0.0) return 0.0;

        double distance = max(distance_ly, radius_ly);
        double horizon_angle = acos(clamp(radius_ly / distance, 0.0, 1.0));
        double span = 2.0 * radius_ly * horizon_angle;

        // Keep a close-up planet from collapsing to a zero-sized voxel grid.
        return max(span, 2.0 * radius_ly / MAX_PLANET_VIEW_BLOCKS);
    }

    static double planet_shell_block_size(double radius_ly, double distance_ly) {
        return planet_visible_surface_span(radius_ly, distance_ly) / MAX_PLANET_VIEW_BLOCKS;
    }

    static void generate_candidate_blocks(
        RenderCandidate& cand,
        shared_ptr<StellarCoordinate> view_pos,
        vector<shared_ptr<Block>>& out_blocks
    ) {

        switch (cand.type) {
            case RenderCandidate::GALAXY:
                if (cand.galaxy)
                    generate_galaxy_blocks(cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;

            case RenderCandidate::SUPER_CLUSTER:
                if (cand.super_cluster)
                    generate_super_cluster_blocks(cand.super_cluster, cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;

            case RenderCandidate::CLUSTER:
                if (cand.cluster)
                    generate_cluster_blocks(cand.cluster, cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;

            case RenderCandidate::STELLAR_CRADLE:
                if (cand.cradle)
                    generate_cradle_blocks(cand.cradle, cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;

            case RenderCandidate::SOLAR_SYSTEM:
                if (cand.solar_system)
                    generate_solar_system_blocks(cand.solar_system, cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;

            case RenderCandidate::STAR:
                if (cand.star) {
                    double radius_ly = cand.star->radius_ly;
                    auto star_blocks = generate_space_sphere(
                        radius_ly,
                        max(cand.sphere_radius_blocks, 1),
                        BMaterial::BLUE_STAR,
                        MAX_BLOCKS
                    );
                    translate_blocks(star_blocks, cand.ox, cand.oy, cand.oz);
                    out_blocks.insert(out_blocks.end(), star_blocks.begin(), star_blocks.end());
                }
                break;

            case RenderCandidate::PLANET: {
                if (!cand.planet) break;
                auto world = make_shared<World>();
                int64_t root_square = (int64_t)ceil(
                    cand.planet->worldSize / (double)Chunk::CHUNK_SIZES[Chunk::topTypeFor(cand.planet->worldSize)]);
                if (root_square < 1) root_square = 1;

                world->godot_enabled = false;
                world->RIVER_GEN_ON = false;
                world->rootChunk = make_shared<RootChunk>((int)root_square, 0, cand.planet);
                world->rootChunk->rootChunk = world->rootChunk;
                cand.planet->rootChunk = world->rootChunk;
                world->rootChunk->init(cand.planet);
                world->init(cand.planet);
                double planet_radius_ly = cand.planet->radius * LY_PER_M;
                double shell_block_size_ly = planet_shell_block_size(
                    planet_radius_ly, cand.dist_ly
                );
                world->init_for_space(
                    view_pos,
                    1.0 / max(shell_block_size_ly, 1e-12)
                );

                int planet_budget = estimate_blocks(cand.type, MAX_PLANET_BLOCKS);
                auto voxel_blocks = world->generate_voxel_planet(planet_budget);
                translate_blocks(voxel_blocks, cand.ox, cand.oy, cand.oz);
                for (auto& vb : voxel_blocks)
                    out_blocks.push_back(vb);
                break;
            }

            case RenderCandidate::NEBULA:
                if (cand.nebula)
                    generate_nebula_blocks(cand.nebula, cand.galaxy, view_pos, cand.block_size_ly, out_blocks);
                break;
        }
    }


    inline static bool have_shown_debug_warning = false;
    inline static unordered_set<RenderCandidate::Type> debug_filter_types = {
        // RenderCandidate::Type::GALAXY,
        // RenderCandidate::Type::NEBULA,
        // RenderCandidate::Type::SUPER_CLUSTER,
        // RenderCandidate::Type::CLUSTER,
        // RenderCandidate::Type::STELLAR_CRADLE,
        // RenderCandidate::Type::SOLAR_SYSTEM,
        // RenderCandidate::Type::STAR,
        // RenderCandidate::Type::PLANET
    };
    static shared_ptr<SpaceGenResult> generate_space (
        shared_ptr<StellarCoordinate> view_pos,
        bool all_sky_blocks = false
    ) {

        Util::print("Generating space...");

        // 1. Generate universe
        auto universe = generate_universe();

        // 2. Generate 3x3x3 continuums around the viewer
        int64_t ck_x = continuum_min_q(view_pos->quadrant_x);
        int64_t ck_y = continuum_min_q(view_pos->quadrant_y);
        int64_t ck_z = continuum_min_q(view_pos->quadrant_z);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    auto coord = make_shared<StellarCoordinate>();
                    coord->quadrant_x = ck_x + dx * QUADRANTS_PER_CONTINUUM;
                    coord->quadrant_y = ck_y + dy * QUADRANTS_PER_CONTINUUM;
                    coord->quadrant_z = ck_z + dz * QUADRANTS_PER_CONTINUUM;
                    Index3 key = {
                        floor_div(coord->quadrant_x, QUADRANTS_PER_CONTINUUM),
                        floor_div(coord->quadrant_y, QUADRANTS_PER_CONTINUUM),
                        floor_div(coord->quadrant_z, QUADRANTS_PER_CONTINUUM)
                    };
                    if (!universe->continuums.has(key))
                        universe->continuums[key] = generate_continuum(universe, coord);
                }
            }
        }

        // 3. Generate 3x3x3 chambers around the viewer
        int64_t chk_x = chamber_min_q(view_pos->quadrant_x);
        int64_t chk_y = chamber_min_q(view_pos->quadrant_y);
        int64_t chk_z = chamber_min_q(view_pos->quadrant_z);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    auto coord = make_shared<StellarCoordinate>();
                    coord->quadrant_x = chk_x + dx * QUADRANTS_PER_CHAMBER;
                    coord->quadrant_y = chk_y + dy * QUADRANTS_PER_CHAMBER;
                    coord->quadrant_z = chk_z + dz * QUADRANTS_PER_CHAMBER;
                    Index3 key = {
                        floor_div(coord->quadrant_x, QUADRANTS_PER_CHAMBER),
                        floor_div(coord->quadrant_y, QUADRANTS_PER_CHAMBER),
                        floor_div(coord->quadrant_z, QUADRANTS_PER_CHAMBER)
                    };
                    if (!universe->chambers.has(key)) {
                        Index3 ck = {
                            floor_div(coord->quadrant_x, QUADRANTS_PER_CONTINUUM),
                            floor_div(coord->quadrant_y, QUADRANTS_PER_CONTINUUM),
                            floor_div(coord->quadrant_z, QUADRANTS_PER_CONTINUUM)
                        };
                        shared_ptr<Continuum> continuum =
                            universe->continuums.has(ck) ? universe->continuums[ck] : nullptr;
                        universe->chambers[key] = generate_chamber(universe, coord, continuum);
                    }
                }
            }
        }

        const double SKIP_THRESHOLD = 0.001;
        const double RECURSE_THRESHOLD = 0.02;
        const double BLOCK_THRESHOLD = 0.16;

        float* image = new float[SkyGen::TOTAL_FLOATS]();

        vector<shared_ptr<Block>> out_blocks;
        vector<shared_ptr<Block>> skybox_blocks;
        vector<RenderCandidate> candidates;
        shared_ptr<Planet> landed_planet = nullptr;
        shared_ptr<PlanetBackgroundSphere> planet_background_sphere = nullptr;

        // ── Phase 1: Hierarchy population + candidate collection ──
        for (auto& [ch_key, chamber] : universe->chambers) {
            for (auto& [gal_key, galaxy] : chamber->galaxies) {
                double gx, gy, gz;
                double gdist = viewer_dist(view_pos, galaxy->location, gx, gy, gz);

                double g_ang = angular_radius_rad(galaxy->radius_ly, gdist);
                if (g_ang < SKIP_THRESHOLD) continue;

                // Galaxy uses a 1-billion-LY grid for its spiral-arm blocks
                double galaxy_block_size = 1'000'000'000;
                if (g_ang < RECURSE_THRESHOLD) {
                    candidates.push_back({RenderCandidate::GALAXY, g_ang, gx, gy, gz, gdist, galaxy_block_size, galaxy});
                    continue;
                }

                // Walk superclusters (already populated by generate_galaxy)
                if (galaxy->super_clusters.size() == 0)
                    generate_galaxy_super_clusters(galaxy);
                if (galaxy->nebulae.empty())
                    generate_galaxy_nebulae(galaxy);
                for (auto& [sqk, sc] : galaxy->super_clusters) {
                    if (!sc || !sc->location) continue;
                    double sx, sy, sz;
                    double sdist = viewer_dist(view_pos, sc->location, sx, sy, sz);
                    double sc_ang = angular_radius_rad(sc->radius_ly, sdist);
                    if (sc_ang < SKIP_THRESHOLD) continue;

                    double sc_block_size = sc->radius_ly * 0.3;
                    if (sc_ang < RECURSE_THRESHOLD) {
                        candidates.push_back({RenderCandidate::SUPER_CLUSTER, sc_ang, sx, sy, sz, sdist, sc_block_size, galaxy});
                        candidates.back().super_cluster = sc;
                        continue;
                    }

                    if (sc->clusters.size() == 0)
                        generate_super_cluster(sc);
                    for (auto& [cqk, cluster_grid] : sc->clusters) {
                        for (auto& [lyk, cluster] : cluster_grid) {
                            if (!cluster || !cluster->location) continue;
                            double cx, cy, cz;
                            double cdist = viewer_dist(view_pos, cluster->location, cx, cy, cz);
                            double c_ang = angular_radius_rad(cluster->radius_ly, cdist);

                            double c_block_size = cluster->radius_ly * 0.3;
                            if (c_ang < RECURSE_THRESHOLD) {
                                candidates.push_back({RenderCandidate::CLUSTER, c_ang, cx, cy, cz, cdist, c_block_size, galaxy});
                                candidates.back().cluster = cluster;
                                continue;
                            }

                            if (cluster->stellar_cradles.empty())
                                generate_cluster(cluster);
                            for (auto& cradle : cluster->stellar_cradles) {
                                if (!cradle || !cradle->location) continue;
                                double crx, cry, crz;
                                double crdist = viewer_dist(view_pos, cradle->location, crx, cry, crz);
                                double cr_ang = angular_radius_rad(cradle->radius_ly, crdist);

                                double cr_block_size = max(0.5, cradle->radius_ly * 0.3);
                                if (cr_ang < RECURSE_THRESHOLD) {
                                    candidates.push_back({RenderCandidate::STELLAR_CRADLE, cr_ang, crx, cry, crz, crdist, cr_block_size, galaxy});
                                    candidates.back().cradle = cradle;
                                    continue;
                                }

                                if (cradle->solar_systems.empty())
                                    generate_stellar_cradle(cradle);
                                for (auto& system : cradle->solar_systems) {
                                    if (!system || !system->location) continue;
                                    double sysx, sysy, sysz;
                                    double sysdist = viewer_dist(view_pos, system->location, sysx, sysy, sysz);
                                    double sys_ang = angular_radius_rad(system->radius_ly, sysdist);

                                    double sys_block_size = max(0.01, system->radius_ly * 0.3);
                                    if (sys_ang < RECURSE_THRESHOLD) {
                                        candidates.push_back({RenderCandidate::SOLAR_SYSTEM, sys_ang, sysx, sysy, sysz, sysdist, sys_block_size, galaxy});
                                        candidates.back().solar_system = system;
                                        continue;
                                    }

                                    for (auto& star : system->stars) {
                                        if (!star || !star->location) continue;
                                        double sox, soy, soz;
                                        double sdist = viewer_dist(view_pos, star->location, sox, soy, soz);
                                        double st_ang = angular_radius_rad(star->radius_ly, sdist);
                                        double star_block_size = star->radius_ly;
                                        candidates.push_back({RenderCandidate::STAR, st_ang, sox, soy, soz, sdist, star_block_size, galaxy});
                                        candidates.back().star = star;
                                        candidates.back().sphere_radius_blocks =
                                            sphere_radius_blocks(st_ang, MAX_STAR_RADIUS_BLOCKS);
                                    }

                                    for (auto& planet : system->planets) {
                                        if (!planet || !planet->location) continue;
                                        generate_planet_properties(planet);
                                        double pox, poy, poz;
                                        double pdist = viewer_dist(view_pos, planet->location, pox, poy, poz);
                                         double planet_radius_ly = planet->radius * LY_PER_M;
                                         double p_ang = angular_radius_rad(planet_radius_ly, pdist);
                                         double planet_block_size = planet_radius_ly;
                                         if (planet_block_size < 1e-6) planet_block_size = 1e-6;
                                         candidates.push_back({RenderCandidate::PLANET, p_ang, pox, poy, poz, pdist, planet_block_size, galaxy});
                                         candidates.back().planet = planet;
                                          candidates.back().sphere_radius_blocks = 0;
                                    }
                                }
                            }
                        }
                    }
                }

                // Nebulae
                for (auto& nebula : galaxy->nebulae) {
                    if (!nebula || !nebula->center) continue;
                    // Use the nebula's own distance, not galaxy-center dist, so
                    // angular size and sky position stay correct when inside the galaxy.
                    double nx, ny, nz;
                    double ndist = viewer_dist(view_pos, nebula->center, nx, ny, nz);
                    if (ndist < nebula->radius_ly) continue; // viewer inside → skip
                    double n_ang = angular_radius_rad(nebula->radius_ly, ndist);
                    if (n_ang < SKIP_THRESHOLD) continue;
                    double nebula_block_size = max(1.0, nebula->radius_ly * 0.04);
                    candidates.push_back({RenderCandidate::NEBULA, n_ang, nx, ny, nz, ndist, nebula_block_size, galaxy});
                    candidates.back().nebula = nebula;
                }
            }
        }


        // ── Phase 3: Compute unified game scale from nearest object ──
        // Skip objects the viewer is inside of (dist < physical radius) so
        // a galaxy enveloping the viewer doesn't dominate the scale.
        auto view_is_inside = [](const RenderCandidate& c) {
            return c.angular_radius >= M_PI * 0.5 - 1e-9;
        };

        double nearest_dist = INFINITY;
        vector<RenderCandidate> block_candidates;
        vector<RenderCandidate> skygen_candidates;
        for (auto& cand : candidates) {
            // Keep planets in the 3D path even when the viewer is inside their
            // physical radius so landing detection and local rendering run.
            bool keep_in_frame = cand.type == RenderCandidate::PLANET || !view_is_inside(cand);
            if (keep_in_frame) {
                block_candidates.push_back(cand);
                if (!view_is_inside(cand) && cand.dist_ly < nearest_dist)
                    nearest_dist = cand.dist_ly;
            } else {
                skygen_candidates.push_back(cand);
            }
        }
        double game_scale = nearest_dist < INFINITY ? 5000.0 / nearest_dist : 1.0; // is basically how many godot block per light year

        // ── Phase 4: Game-space blocks ──
        // Partition: keep only objects within MAX_RANGE after scaling (planets use World's own scale)
        vector<RenderCandidate> near_candidates;
        for (auto& cand : block_candidates) {
            if (cand.dist_ly * game_scale > MAX_RANGE)
                skygen_candidates.push_back(cand);
            else if (cand.type != RenderCandidate::STAR && cand.type != RenderCandidate::PLANET)
                skygen_candidates.push_back(cand);
            else
                near_candidates.push_back(cand);
        }
        block_candidates.swap(near_candidates);

        // Sort by distance (closest first fills game-space budget)
        sort(block_candidates.begin(), block_candidates.end(),
            [](const RenderCandidate& a, const RenderCandidate& b) {
                return a.dist_ly < b.dist_ly;
            });

        int block_count = 0;
        size_t processed = 0;
        for (auto& cand : block_candidates) {
            
            if (debug_filter_types.contains(cand.type)) {
                if (!SpaceGen::have_shown_debug_warning) cout << "WARNING: space gen debug filter applied, this should not be a published game version: " << cand.type << endl;
                SpaceGen::have_shown_debug_warning = true;
                ++processed;
                continue;
            }

            if (block_count >= MAX_BLOCKS)
                break;

            // -- Determine if player is on a planet
            if (cand.type == RenderCandidate::PLANET) {

                // Player is "on" this planet when their distance to its center is within
                // the surface radius there: base radius + terrain height at the nearest
                // surface point. Hand the planet to the result so terrain gen takes over.
                if (cand.planet) {
                    double px, py, pz;
                    double pdist_ly = viewer_dist(view_pos, cand.planet->location, px, py, pz);
                    double dist_m = pdist_ly / LY_PER_M;

                    // Normalize the planet->player LY offset into the sphere frame
                    // (viewer_dist gives planet minus player), then map that surface
                    // point to the flat world map; its chunk's y is the terrain height.
                    double inv = 1.0 / max(pdist_ly, 1e-12);
                    Vector3 surface_dir(-px * inv, -py * inv, -pz * inv);
                    shared_ptr<Position> nearest_world =
                        CoordinateConversion::sphere_location_to_world_position(surface_dir, cand.planet);
                    double surface_height = cand.planet->maxHeight;

                    if (dist_m <= cand.planet->radius + surface_height) {
                        landed_planet = cand.planet;
                        ++processed;
                        continue;
                    }
                }

            }

            // generate blocks
            vector<shared_ptr<Block>> temp;
            generate_candidate_blocks(cand, view_pos, temp);
            if (cand.type == RenderCandidate::PLANET) {

                // Player is "on" this planet when their distance to its center is within
                // the surface radius there: base radius + terrain height at the nearest
                // surface point. Hand the planet to the result so terrain gen takes over.
                if (cand.planet && cand.planet->rootChunk) {
                    if (!planet_background_sphere) {
                        planet_background_sphere = make_shared<PlanetBackgroundSphere>(
                            make_planet_background_sphere(
                                cand.planet,
                                static_pointer_cast<RootChunk>(cand.planet->rootChunk),
                                cand.ox,
                                cand.oy,
                                cand.oz,
                                render_radius_for_candidate(cand, game_scale),
                                planet_shell_block_size(
                                    cand.planet->radius * LY_PER_M, cand.dist_ly
                                ) *
                                render_radius_for_candidate(cand, game_scale) /
                                    max(cand.planet->radius * LY_PER_M, 1e-12)
                            )
                        );
                    }
                }

            }

            for (auto& b : temp) {
                if (!b || !b->position_double) continue;

                double object_radius_ly = cand.type == RenderCandidate::PLANET
                    ? cand.planet->radius * LY_PER_M
                    : cand.star->radius_ly;
                double object_radius_godot = render_radius_for_candidate(cand, game_scale);
                double local_scale = object_radius_godot / max(object_radius_ly, 1e-12);
                double local_x = b->position_double->x - cand.ox;
                double local_y = b->position_double->y - cand.oy;
                double local_z = b->position_double->z - cand.oz;

                b->position_double->x = cand.ox * game_scale + local_x * local_scale;
                b->position_double->y = cand.oy * game_scale + local_y * local_scale;
                b->position_double->z = cand.oz * game_scale + local_z * local_scale;
                b->render_size = b->size * local_scale;
                out_blocks.push_back(b);
            }
            block_count = (int)out_blocks.size();
            ++processed;
        }
        // Any remaining block_candidates go to sky
        for (size_t i = processed; i < block_candidates.size(); ++i)
            skygen_candidates.push_back(block_candidates[i]);


        // ── Phase 5: Sky blocks for remaining objects ──
        sort(skygen_candidates.begin(), skygen_candidates.end(),
            [](const RenderCandidate& a, const RenderCandidate& b) {
                return a.angular_radius > b.angular_radius;
        });
        for (auto& cand : skygen_candidates) {

            if (debug_filter_types.contains(cand.type)) {
                if (!SpaceGen::have_shown_debug_warning) cout << "WARNING: space gen debug filter applied, this should not be a published game version: " << cand.type << endl;
                SpaceGen::have_shown_debug_warning = true;
                continue;
            }

            // Nebulae carry their own distance-driven LOD density, so always
            // generate blocks instead of the abrupt single-block cliff below
            // BLOCK_THRESHOLD.
            if (cand.type == RenderCandidate::NEBULA) {
                generate_candidate_blocks(cand, view_pos, skybox_blocks);
                continue;
            }

            if (cand.angular_radius >= BLOCK_THRESHOLD) {
                // Deserves blocks but over budget or too far for game space → LY blocks for skybox
                generate_candidate_blocks(cand, view_pos, skybox_blocks);
            } else {
                // Sub-threshold single-block sky representation keeps the
                // candidate's physical LY size for proportional SkyGen splats.
                auto tb = make_space_block(glow_material(cand), cand.ox, cand.oy, cand.oz, cand.block_size_ly);
                skybox_blocks.push_back(tb);
            }
        }

        // ── Phase 6: Render skybox ──
        vector<shared_ptr<Block>> all_for_sky = {};
        if (all_sky_blocks) {
            all_for_sky = out_blocks;
        }
        all_for_sky.insert(all_for_sky.end(), skybox_blocks.begin(), skybox_blocks.end());
        SkyGen::generate_sky_box(image, all_for_sky, 0, 0, 0);



        auto result = make_shared<SpaceGenResult>();
        result->blocks = out_blocks;
        result->sky_box = image;
        result->planet = landed_planet;
        result->planet_background_sphere = planet_background_sphere;
        result->game_scale = game_scale;
        return result;
    }








    
    // TEST
    static void isometric_test(
        vector<shared_ptr<Cosmic_River>> cosmic_rivers,
        Grid<shared_ptr<Galaxy>> galaxies,
        IsoAngle angle,
        double scale,
        string path = "src/TEST/output/RENDER.png"
    ) {
        // Find bounding box across all rivers and galaxies
        int64_t min_x = INT64_MAX, min_y = INT64_MAX, min_z = INT64_MAX;
        int64_t max_x = INT64_MIN, max_y = INT64_MIN, max_z = INT64_MIN;
        bool has_data = false;

        auto expand = [&](int64_t x, int64_t y, int64_t z) {
            if (x < min_x) min_x = x; if (x > max_x) max_x = x;
            if (y < min_y) min_y = y; if (y > max_y) max_y = y;
            if (z < min_z) min_z = z; if (z > max_z) max_z = z;
            has_data = true;
        };

        for (auto& river : cosmic_rivers) {
            expand(river->min_qx, river->min_qy, river->min_qz);
            expand(river->max_qx, river->max_qy, river->max_qz);
        }

        for (auto& [_, galaxy] : galaxies) {
            if (galaxy->location)
                expand(galaxy->location->quadrant_x,
                       galaxy->location->quadrant_y,
                       galaxy->location->quadrant_z);
        }

        if (!has_data) return;

        // Scale to fit within 256 blocks
        int64_t range_x = max_x - min_x;
        int64_t range_y = max_y - min_y;
        int64_t range_z = max_z - min_z;
        int64_t max_range = max({range_x, range_y, range_z, (int64_t)1});
        double block_scale = 255.0 / max_range;

        auto to_block = [&](int64_t v, int64_t lo) -> int {
            return (int)((v - lo) * block_scale);
        };

        Grid<shared_ptr<Block>> block_grid;

        // Basalt blocks along river paths (continuous line between points)
        for (auto& river : cosmic_rivers) {
            auto& pts = river->path_points;
            for (size_t i = 0; i + 1 < pts.size(); i++) {
                int x1 = to_block(pts[i]->quadrant_x,     min_x);
                int y1 = to_block(pts[i]->quadrant_y,     min_y);
                int z1 = to_block(pts[i]->quadrant_z,     min_z);
                int x2 = to_block(pts[i + 1]->quadrant_x, min_x);
                int y2 = to_block(pts[i + 1]->quadrant_y, min_y);
                int z2 = to_block(pts[i + 1]->quadrant_z, min_z);

                int dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
                int steps = max(max(abs(dx), abs(dy)), abs(dz));

                for (int s = 0; s <= steps; s++) {
                    int bx = x1 + dx * s / steps;
                    int by = y1 + dy * s / steps;
                    int bz = z1 + dz * s / steps;
                    Index3 idx{bx, by, bz};
                    if (!block_grid.has(idx)) {
                        auto pos = make_shared<Position>(bx, by, bz);
                        block_grid[idx] = make_shared<Block>(BMaterial::BASALT, pos, nullptr, 1.0);
                    }
                }
            }
        }

        // Andesite blocks for galaxies — overwrite any river blocks
        for (auto& [_, galaxy] : galaxies) {
            if (galaxy->location) {
                int bx = to_block(galaxy->location->quadrant_x, min_x);
                int by = to_block(galaxy->location->quadrant_y, min_y);
                int bz = to_block(galaxy->location->quadrant_z, min_z);
                Index3 idx{bx, by, bz};
                auto pos = make_shared<Position>(bx, by, bz);
                block_grid[idx] = make_shared<Block>(BMaterial::ANDESTITE, pos, nullptr, 1.0);
            }
        }

        vector<shared_ptr<Block>> blocks = block_grid.values();
        IsometricRenderer::render_s(blocks, path, angle, scale);
    }


};
