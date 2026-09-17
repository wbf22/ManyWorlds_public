#pragma once


#include <cmath>

#include "util/Grid.hpp"
#include "util/Random.h"
#include "util/BMaterial.hpp"
#include "../Room.hpp"
#include "../City.hpp"
#include "Styles.hpp"


using namespace std;


struct StyleGen {


    /**
     * 
     * 
     * Categories
     * - framing
     *      + window and door frames
     *      + building corner framing
     *      + floor level framing
     *      + roof framing
     * - jut outs and exterior stairs
     *      + porches with overhang roof and columns/pillars
     *      + balconies
     *      + under window pots and lips
     *      + exterior walkways
     * - roofs
     *      + roof shape
     * - walls, spires, towers
     *      + walls
     *      + towers/spires
     *      + chimneys and exhuast
     *      + fortifications
     * - decoration
     *      + lights
     *      + hanging docorations
     *      + hanging plants
     *      + signs
     *      + wall designs
     *      + materials
     *      + wires and ropes, flags
     */



    // STYLE GEN FUNCTIONS

    static shared_ptr<FrameStyleElement> gen_frame_style(
        int64_t seed_spike,
        int largest_occupant
    ) {
        int thickness = Random::randInt(seed_spike, 1, largest_occupant);

        Grid<bool> cross_section = Grid<bool>(thickness, thickness, thickness);
        int fill_amount = Random::randInt(seed_spike + 2, 1, thickness * thickness * thickness);
        int amount_filled = 0;
        int half_thickness = thickness / 2;
        while (amount_filled < fill_amount) {
            int x = Random::randInt(seed_spike + 3, 0, half_thickness, thickness);
            int z = Random::randInt(seed_spike + 4, 0, half_thickness, thickness);
            int y = 0;
            while (cross_section.in_bounds({x, y, z}) && cross_section[{x, y, z}]) {
                ++y;
            }
            if (cross_section.in_bounds({x, y, z})) {
                cross_section[{x, y, z}] = true;
                ++amount_filled;
            }
            ++seed_spike;
        }

        auto frame = std::make_shared<FrameStyleElement>();
        frame->type = StyleElementType::FRAME;
        frame->cross_section_grid = cross_section;
        return frame;
    }

    static shared_ptr<RoofStyleElement> gen_roof_style(
        shared_ptr<City> city,
        int64_t seed_spike
    ) {
        auto roof = std::make_shared<RoofStyleElement>();
        int avg_pitch = city->snow_amount * 0.8 + city->precipitation * 0.2;
        roof->pitch = Random::randInt(seed_spike, avg_pitch, 100);
        roof->curvature = Random::randInt(seed_spike, 0, 100);
        roof->curvature = Random::randBool(seed_spike + 1, 1) ? roof->curvature : 50;
        roof->overhang = Random::randInt(seed_spike + 2, 0, 3);
        roof->layers = Random::randInt(seed_spike + 3, 1, 1, 3);

        int profile_w = Random::randInt(seed_spike + 4, 1, 4);
        int profile_d = Random::randInt(seed_spike + 5, 1, 4);
        Grid<bool> profile(profile_w, 1, profile_d);
        for (int px = 0; px < profile_w; ++px) {
            for (int pz = 0; pz < profile_d; ++pz) {
                profile[{px, 0, pz}] = Random::randBool(seed_spike + px + pz * profile_w, 2);
            }
        }
        roof->surface_texture_profile = profile;
        roof->type = StyleElementType::ROOF;
        return roof;
    }

    static shared_ptr<ColumnStyleElement> gen_column_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group
    ) {
        auto column = std::make_shared<ColumnStyleElement>();
        column->round = Random::randBool(seed_spike + 4);
        if (Random::randBool(seed_spike, 25) && style_group->frame) {
            column->frame;
        }
        column->lines = Random::randBool(seed_spike + 1, 25);

        if (Random::randBool(seed_spike + 2)) {
            column->top_column_style_element = std::make_shared<ColumnStyleElement>();
            column->top_column_style_element->round = Random::randBool(seed_spike + 4);
            if (Random::randBool(seed_spike, 25) && style_group->frame) {
                column->top_column_style_element->frame;
            }
            column->top_column_style_element->lines = Random::randBool(seed_spike + 1, 25);

            column->bottom_column_style_element = std::make_shared<ColumnStyleElement>();
            column->bottom_column_style_element->round = Random::randBool(seed_spike + 4);
            if (Random::randBool(seed_spike, 25) && style_group->frame) {
                column->bottom_column_style_element->frame;
            }
            column->bottom_column_style_element->lines = Random::randBool(seed_spike + 1, 25);
        }
        column->type = StyleElementType::COLUMN;
        return column;
    }

    static shared_ptr<WallDesignElement> gen_wall_design_style(int64_t seed_spike, int base_size) {
        auto design = std::make_shared<WallDesignElement>();
        design->type = StyleElementType::WALL_DESIGN;

        int pw = max(2, base_size);
        int ph = max(2, base_size);
        Grid<bool> pattern(pw, 1, ph);

        int num_lines = Random::randInt(seed_spike, 1, max(2, pw / 2));

        int mid_x = pw / 2;
        for (int li = 0; li < num_lines; ++li) {
            int func = Random::randInt(seed_spike + 1 + li * 7, 0, 3);
            switch (func) {
                case 0: { // horizontal line
                    int y = Random::randInt(seed_spike + 2 + li * 7, 0, ph - 1);
                    for (int x = 0; x <= mid_x; ++x)
                        pattern[{x, 0, y}] = true;
                    break;
                }
                case 1: { // vertical line
                    int x = Random::randInt(seed_spike + 3 + li * 7, 0, mid_x);
                    for (int y = 0; y < ph; ++y)
                        pattern[{x, 0, y}] = true;
                    break;
                }
                case 2: { // diagonal: Bresenham line from (0, sy) to (mid_x, ey)
                    int sy = Random::randInt(seed_spike + 4 + li * 7, 0, ph - 1);
                    int ey = Random::randInt(seed_spike + 5 + li * 7, 0, ph - 1);
                    int x0 = 0, y0 = sy, x1 = mid_x, y1 = ey;
                    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
                    int dy = -abs(y1 - y0), sy_dir = y0 < y1 ? 1 : -1;
                    int err = dx + dy;
                    while (true) {
                        if (x0 >= 0 && x0 <= mid_x && y0 >= 0 && y0 < ph) {
                            pattern[{x0, 0, y0}] = true;
                        }
                        if (x0 == x1 && y0 == y1) break;
                        int e2 = 2 * err;
                        if (e2 >= dy) { err += dy; x0 += sx; }
                        if (e2 <= dx) { err += dx; y0 += sy_dir; }
                    }
                    break;
                }
                case 3: { // sine wave: Bresenham segments between samples
                    int amp = Random::randInt(seed_spike + 6 + li * 7, 1, max(1, ph / 3));
                    int freq = Random::randInt(seed_spike + 7 + li * 7, 1, 3);
                    int offset = Random::randInt(seed_spike + 8 + li * 7, 0, ph - 1);
                    for (int x = 0; x < mid_x; ++x) {
                        double t0 = M_PI * freq * x / max(1, mid_x);
                        double t1 = M_PI * freq * (x + 1) / max(1, mid_x);
                        int y0 = offset + (int)round(amp * sin(t0));
                        int y1 = offset + (int)round(amp * sin(t1));
                        int sx0 = x, sy0 = y0, sx1 = x + 1, sy1 = y1;
                        int sdx = abs(sx1 - sx0), ssx = sx0 < sx1 ? 1 : -1;
                        int sdy = -abs(sy1 - sy0), ssy = sy0 < sy1 ? 1 : -1;
                        int serr = sdx + sdy;
                        while (true) {
                            if (sx0 >= 0 && sx0 <= mid_x && sy0 >= 0 && sy0 < ph)
                                pattern[{sx0, 0, sy0}] = true;
                            if (sx0 == sx1 && sy0 == sy1) break;
                            int se2 = 2 * serr;
                            if (se2 >= sdy) { serr += sdy; sx0 += ssx; }
                            if (se2 <= sdx) { serr += sdx; sy0 += ssy; }
                        }
                    }
                    break;
                }
            }
        }

        // Mirror across Y axis (bidirectional — each pair or'd together)
        for (int py = 0; py < ph; ++py) {
            for (int px = 0; px <= mid_x; ++px) {
                int mx = pw - 1 - px;
                if (mx == px) continue;
                if (pattern.has({px, 0, py}) || pattern.has({mx, 0, py})) {
                    pattern[{px, 0, py}] = true;
                    pattern[{mx, 0, py}] = true;
                }
            }
        }

        design->pattern = pattern;
        return design;
    }

    static shared_ptr<WallStyleElement> gen_wall_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group,
        int thickness,
        bool straight_only
    ) {
        auto wall = std::make_shared<WallStyleElement>();
        wall->type = StyleElementType::WALL;

        if (Random::randBool(seed_spike+14)) {
            wall->frame = style_group->frame;
        }

        if (!straight_only) {
            wall->outward_slope = Random::randDouble(seed_spike+234, 0, 0.5);
            wall->inward_slope = Random::randDouble(seed_spike+698, 0, 0.5);
        }
        else {
            wall->outward_slope = 0;
            wall->inward_slope = 0;
        }
        wall->thickness = thickness;

        if (style_group->wall_design && Random::randBool(seed_spike+821, 50)) {
            wall->wall_design = style_group->wall_design;
        }

        return wall;
    }

    static shared_ptr<JutOutStyleElement> gen_jut_out_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group,
        int bottom_support_height
    ) {
        auto jut_out = std::make_shared<JutOutStyleElement>();
        jut_out->type = StyleElementType::JUT_OUT;

        bool has_roof = Random::randBool(seed_spike);
        if (has_roof) {
            jut_out->roof = style_group->roof;
        }

        bool frame_support = Random::randBool(seed_spike+2);
        if (frame_support) {
            jut_out->bottom_support = style_group->frame;
        }
        else {
            jut_out->smoothed_bottom_support = true;
        }
        jut_out->bottom_support_height = bottom_support_height;

        bool has_railing = Random::randBool(seed_spike+1);
        if (has_railing) {
            jut_out->railing_or_wall = style_group->wall;
        }
        return jut_out;
    }

    static shared_ptr<StairStyleElement> gen_stair_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group,
        bool possible_solid_bottom
    ) {
        auto stair = make_shared<StairStyleElement>();
        stair->stair_size_in_blocks = 1;
        stair->solid_bottom = possible_solid_bottom? Random::randBool(seed_spike, 50) : false;
        stair->railing_or_wall = style_group->wall;
        return stair;
    }

    static shared_ptr<ChimneyStyleElement> gen_chimney_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group
    ) {
        auto chimney = make_shared<ChimneyStyleElement>();
        chimney->type = StyleElementType::CHIMNEY;
        chimney->interior_column = style_group->column;
        chimney->exterior_column = style_group->column;
        chimney->fire_place_width = max(3, (int)(style_group->column->materials.size() > 0 ? 4 : 3));
        chimney->fire_place_height = max(2, chimney->fire_place_width * 3 / 4);
        chimney->round = style_group->column->round;
        return chimney;
    }



    // STYLE TO BLOCKS FUNCTIONS

    static void apply_frame_style(
        shared_ptr<StyleElement> style,
        shared_ptr<Position> start,
        shared_ptr<Position> end,
        int side, // 0 = N, 1 = E, 2 = S, 3 = W
        Grid<shared_ptr<Block>>& blocks
    ) {
        shared_ptr<FrameStyleElement> frame = static_pointer_cast<FrameStyleElement>(style);
        int t = max(1, (int)frame->cross_section_grid.width);

        string material_str = "";

        int x0 = start->x, x1 = end->x;
        int y0 = start->y, y1 = end->y;
        int z0 = start->z, z1 = end->z;

        if (side == 0) { // north: wall at z = z1, outward +z
            // Top edge: along x, at y1
            for (int x = x0; x <= x1; ++x) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((x - x0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x, y1 - gx, z1 + 1 + gz, material_str);
                        }
                    }
                }
            }
            // Bottom edge: along x, at y0 (floor level)
            for (int x = x0; x <= x1; ++x) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((x - x0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x, y0 + gx, z1 + 1 + gz, material_str);
                        }
                    }
                }
            }
            // West edge: along y, at x0
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 + gx, y, z1 + 1 + gz, material_str);
                        }
                    }
                }
            }
            // East edge: along y, at x1
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 - gx, y, z1 + 1 + gz, material_str);
                        }
                    }
                }
            }
        }
        else if (side == 1) { // east: wall at x = x1, outward +x
            // Top edge: along z, at y1
            for (int z = z0; z <= z1; ++z) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((z - z0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 + 1 + gx, y1 - gz, z, material_str);
                        }
                    }
                }
            }
            // Bottom edge: along z, at y0 (floor level)
            for (int z = z0; z <= z1; ++z) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((z - z0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 + 1 + gx, y0 + gz, z, material_str);
                        }
                    }
                }
            }
            // South edge: along y, at z0
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 + 1 + gx, y, z0 + gz, material_str);
                        }
                    }
                }
            }
            // North edge: along y, at z1
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 + 1 + gx, y, z1 - gz, material_str);
                        }
                    }
                }
            }
        }
        else if (side == 2) { // south: wall at z = z0, outward -z
            // Top edge: along x, at y1
            for (int x = x0; x <= x1; ++x) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((x - x0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x, y1 - gx, z0 - 1 - gz, material_str);
                        }
                    }
                }
            }
            // Bottom edge: along x, at y0 (floor level)
            for (int x = x0; x <= x1; ++x) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((x - x0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x, y0 + gx, z0 - 1 - gz, material_str);
                        }
                    }
                }
            }
            // West edge: along y, at x0
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 + gx, y, z0 - 1 - gz, material_str);
                        }
                    }
                }
            }
            // East edge: along y, at x1
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x1 - gx, y, z0 - 1 - gz, material_str);
                        }
                    }
                }
            }
        }
        else if (side == 3) { // west: wall at x = x0, outward -x
            // Top edge: along z, at y1
            for (int z = z0; z <= z1; ++z) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((z - z0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 - 1 - gx, y1 - gz, z, material_str);
                        }
                    }
                }
            }
            // Bottom edge: along z, at y0 (floor level)
            for (int z = z0; z <= z1; ++z) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((z - z0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 - 1 - gx, y0 + gz, z, material_str);
                        }
                    }
                }
            }
            // South edge: along y, at z0
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 - 1 - gx, y, z0 + gz, material_str);
                        }
                    }
                }
            }
            // North edge: along y, at z1
            for (int y = y0; y <= y1; ++y) {
                for (int gx = 0; gx < t; ++gx) {
                    for (int gz = 0; gz < t; ++gz) {
                        int gy_idx = ((y - y0) % t + t) % t;
                        if (frame->cross_section_grid.has({gx, gy_idx, gz})) {
                            place_block(blocks, x0 - 1 - gx, y, z1 - gz, material_str);
                        }
                    }
                }
            }
        }
    }

    static void apply_roof_style(
        shared_ptr<RoofStyleElement> style,
        Grid<shared_ptr<Section>> sections,
        Grid<shared_ptr<Block>>& blocks,
        int64_t seed
    ) {
        if (!style) return;

        unordered_set<shared_ptr<Section>> unique_sections;
        for (auto& [idx, s] : sections.data) {
            if (s) unique_sections.insert(s);
        }
        if (unique_sections.empty()) return;

        const auto& const_grid = sections;

        enum RoofType { FLAT, GABLE_EW, GABLE_NS, SHED_N, SHED_S, SHED_E, SHED_W,
                        TENT_NE, TENT_NW, TENT_SE, TENT_SW };

        auto curvature_func = [](double t, double curvature) -> double {
            if (t >= 1.0) return 0.0;
            double f_concave = 1.0 - t * t;
            double f_straight = 1.0 - t;
            double f_dome = sqrt(max(0.0, 1.0 - t * t));
            if (curvature <= 50.0) {
                double a = curvature / 50.0;
                return f_concave * (1.0 - a) + f_straight * a;
            } else {
                double a = (curvature - 50.0) / 50.0;
                return f_straight * (1.0 - a) + f_dome * a;
            }
        };

        auto mod_wrap = [](int64_t a, int64_t m) -> int64_t {
            return ((a % m) + m) % m;
        };

        for (auto& section : unique_sections) {
            int x0 = INT_MAX, x1 = INT_MIN;
            int z0 = INT_MAX, z1 = INT_MIN;
            int y1_top = 0;
            for (auto& room : section->rooms) {
                if (!room->start || !room->end) continue;
                x0 = min(x0, (int)room->start->x);
                x1 = max(x1, (int)room->end->x);
                z0 = min(z0, (int)room->start->z);
                z1 = max(z1, (int)room->end->z);
                y1_top = max(y1_top, (int)room->end->y);
            }
            if (x0 == INT_MAX) continue;

            int sx = section->start->x, sz = section->start->z;
            int s = section->scale, vs = section->vertical_scale;
            int top_grid_y = section->start->y + vs;

            bool has_above = false;
            for (int gx = sx; gx < sx + s && !has_above; ++gx) {
                for (int gz = sz; gz < sz + s && !has_above; ++gz) {
                    auto cell = const_grid[{gx, top_grid_y, gz}];
                    if (cell && cell != section) has_above = true;
                }
            }
            if (has_above) continue;

            bool north = false, east = false, south = false, west = false;
            for (int gx = sx; gx < sx + s; ++gx) {
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{gx, gy, sz + s}];
                    if (c && c != section) north = true;
                    c = const_grid[{gx, gy, sz - 1}];
                    if (c && c != section) south = true;
                }
            }
            for (int gz = sz; gz < sz + s; ++gz) {
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{sx + s, gy, gz}];
                    if (c && c != section) east = true;
                    c = const_grid[{sx - 1, gy, gz}];
                    if (c && c != section) west = true;
                }
            }

            int n_neighbor_actual = -1, s_neighbor_actual = -1;
            int e_neighbor_actual = -1, w_neighbor_actual = -1;
            for (int gx = sx; gx < sx + s; ++gx)
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{gx, gy, sz + s}];
                    if (c && c != section)
                        for (auto& r : c->rooms)
                            if (r && r->end) n_neighbor_actual = max(n_neighbor_actual, (int)r->end->y);
                }
            for (int gx = sx; gx < sx + s; ++gx)
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{gx, gy, sz - 1}];
                    if (c && c != section)
                        for (auto& r : c->rooms)
                            if (r && r->end) s_neighbor_actual = max(s_neighbor_actual, (int)r->end->y);
                }
            for (int gz = sz; gz < sz + s; ++gz)
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{sx + s, gy, gz}];
                    if (c && c != section)
                        for (auto& r : c->rooms)
                            if (r && r->end) e_neighbor_actual = max(e_neighbor_actual, (int)r->end->y);
                }
            for (int gz = sz; gz < sz + s; ++gz)
                for (int gy = section->start->y; gy < section->start->y + vs; ++gy) {
                    auto c = const_grid[{sx - 1, gy, gz}];
                    if (c && c != section)
                        for (auto& r : c->rooms)
                            if (r && r->end) w_neighbor_actual = max(w_neighbor_actual, (int)r->end->y);
                }

            int n_neighbor_y = (n_neighbor_actual >= 0) ? max(n_neighbor_actual, y1_top) : y1_top;
            int s_neighbor_y = (s_neighbor_actual >= 0) ? max(s_neighbor_actual, y1_top) : y1_top;
            int e_neighbor_y = (e_neighbor_actual >= 0) ? max(e_neighbor_actual, y1_top) : y1_top;
            int w_neighbor_y = (w_neighbor_actual >= 0) ? max(w_neighbor_actual, y1_top) : y1_top;

            auto is_ridge = [&](bool has_neighbor, int actual) -> bool {
                return has_neighbor && actual >= y1_top;
            };
            vector<int> free_sides, ridge_sides;
            if (!is_ridge(north, n_neighbor_actual)) free_sides.push_back(0); else ridge_sides.push_back(0);
            if (!is_ridge(east,  e_neighbor_actual)) free_sides.push_back(1); else ridge_sides.push_back(1);
            if (!is_ridge(south, s_neighbor_actual)) free_sides.push_back(2); else ridge_sides.push_back(2);
            if (!is_ridge(west,  w_neighbor_actual)) free_sides.push_back(3); else ridge_sides.push_back(3);

            int oh_n = 0, oh_s = 0, oh_e = 0, oh_w = 0;
            for (int fs : free_sides) {
                if (fs == 0) oh_n = style->overhang;
                else if (fs == 1) oh_e = style->overhang;
                else if (fs == 2) oh_s = style->overhang;
                else if (fs == 3) oh_w = style->overhang;
            }

            double width = x1 - x0 + 1.0;
            double depth = z1 - z0 + 1.0;

            RoofType roof_type = FLAT;

            if (free_sides.empty()) {
                vector<RoofType> types = {FLAT, GABLE_EW, GABLE_NS, SHED_N, SHED_S, SHED_E, SHED_W, TENT_NE, TENT_NW, TENT_SE, TENT_SW};
                roof_type = Random::random_choice(types, seed);
            } else if (free_sides.size() == 1) {
                int fs = free_sides[0];
                if (fs == 0) roof_type = SHED_N;
                else if (fs == 1) roof_type = SHED_E;
                else if (fs == 2) roof_type = SHED_S;
                else if (fs == 3) roof_type = SHED_W;
            } else if (free_sides.size() == 2) {
                bool n_and_s = (free_sides[0] == 0 && free_sides[1] == 2) ||
                               (free_sides[0] == 2 && free_sides[1] == 0);
                bool e_and_w = (free_sides[0] == 1 && free_sides[1] == 3) ||
                               (free_sides[0] == 3 && free_sides[1] == 1);
                if (n_and_s) {
                    roof_type = GABLE_EW;
                } else if (e_and_w) {
                    roof_type = GABLE_NS;
                } else {
                    if (north && east) roof_type = TENT_NE;
                    else if (north && west) roof_type = TENT_NW;
                    else if (east && south) roof_type = TENT_SE;
                    else if (south && west) roof_type = TENT_SW;
                    else roof_type = GABLE_EW;
                }
            } else {
                if (ridge_sides.empty()) {
                    if (depth > width) roof_type = GABLE_EW;
                    else roof_type = GABLE_NS;
                } else {
                    int rs = ridge_sides[0];
                    if (rs == 0) roof_type = SHED_S;
                    else if (rs == 1) roof_type = SHED_W;
                    else if (rs == 2) roof_type = SHED_N;
                    else if (rs == 3) roof_type = SHED_E;
                }
            }

            auto roof_height_at = [&](int x, int z, int lx0, int lx1, int lz0, int lz1,
                                       double lwidth, double ldepth, double base_y,
                                       double lmax_h, int loh_n, int loh_s,
                                       int loh_e, int loh_w, RoofType type, int layer) -> double {
                switch (type) {
                    case FLAT:
                        return base_y;
                    case GABLE_EW: {
                        double ridge_z = (lz0 + lz1) / 2.0;
                        double dist = abs((double)z - ridge_z);
                        double half = ldepth / 2.0;
                        double max_dist = max(half + loh_n, half + loh_s);
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        return base_y + lmax_h * curvature_func(t, style->curvature);
                    }
                    case GABLE_NS: {
                        double ridge_x = (lx0 + lx1) / 2.0;
                        double dist = abs((double)x - ridge_x);
                        double half = lwidth / 2.0;
                        double max_dist = max(half + loh_e, half + loh_w);
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        return base_y + lmax_h * curvature_func(t, style->curvature);
                    }
                    case SHED_N: {
                        double dist = (double)(z - lz0);
                        double max_dist = ldepth - 1.0 + loh_n;
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        double nr;
                        if (s_neighbor_y > y1_top) {
                            double total_rise = s_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            nr = max(0.0, layer_target - base_y);
                        } else {
                            nr = lmax_h;
                        }
                        return base_y + min(lmax_h, nr) * curvature_func(t, style->curvature);
                    }
                    case SHED_S: {
                        double dist = (double)(lz1 - z);
                        double max_dist = ldepth - 1.0 + loh_s;
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        double nr;
                        if (n_neighbor_y > y1_top) {
                            double total_rise = n_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            nr = max(0.0, layer_target - base_y);
                        } else {
                            nr = lmax_h;
                        }
                        return base_y + min(lmax_h, nr) * curvature_func(t, style->curvature);
                    }
                    case SHED_E: {
                        double dist = (double)(x - lx0);
                        double max_dist = lwidth - 1.0 + loh_e;
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        double nr;
                        if (w_neighbor_y > y1_top) {
                            double total_rise = w_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            nr = max(0.0, layer_target - base_y);
                        } else {
                            nr = lmax_h;
                        }
                        return base_y + min(lmax_h, nr) * curvature_func(t, style->curvature);
                    }
                    case SHED_W: {
                        double dist = (double)(lx1 - x);
                        double max_dist = lwidth - 1.0 + loh_w;
                        double t = min(dist / max(max_dist, 1.0), 1.0);
                        double nr;
                        if (e_neighbor_y > y1_top) {
                            double total_rise = e_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            nr = max(0.0, layer_target - base_y);
                        } else {
                            nr = lmax_h;
                        }
                        return base_y + min(lmax_h, nr) * curvature_func(t, style->curvature);
                    }
                    case TENT_NE: {
                        double t1 = (double)(lz1 - z) / max(ldepth - 1.0 + loh_s, 1.0);
                        double t2 = (double)(lx1 - x) / max(lwidth - 1.0 + loh_w, 1.0);
                        t1 = min(t1, 1.0); t2 = min(t2, 1.0);
                        double cap1;
                        if (n_neighbor_y > y1_top) {
                            double total_rise = n_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap1 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap1 = lmax_h;
                        }
                        double cap2;
                        if (e_neighbor_y > y1_top) {
                            double total_rise = e_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap2 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap2 = lmax_h;
                        }
                        double er = min(cap1, cap2);
                        return base_y + er * max(curvature_func(t1, style->curvature), curvature_func(t2, style->curvature));
                    }
                    case TENT_NW: {
                        double t1 = (double)(lz1 - z) / max(ldepth - 1.0 + loh_s, 1.0);
                        double t2 = (double)(x - lx0) / max(lwidth - 1.0 + loh_e, 1.0);
                        t1 = min(t1, 1.0); t2 = min(t2, 1.0);
                        double cap1;
                        if (n_neighbor_y > y1_top) {
                            double total_rise = n_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap1 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap1 = lmax_h;
                        }
                        double cap2;
                        if (w_neighbor_y > y1_top) {
                            double total_rise = w_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap2 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap2 = lmax_h;
                        }
                        double er = min(cap1, cap2);
                        return base_y + er * max(curvature_func(t1, style->curvature), curvature_func(t2, style->curvature));
                    }
                    case TENT_SE: {
                        double t1 = (double)(z - lz0) / max(ldepth - 1.0 + loh_n, 1.0);
                        double t2 = (double)(lx1 - x) / max(lwidth - 1.0 + loh_w, 1.0);
                        t1 = min(t1, 1.0); t2 = min(t2, 1.0);
                        double cap1;
                        if (s_neighbor_y > y1_top) {
                            double total_rise = s_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap1 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap1 = lmax_h;
                        }
                        double cap2;
                        if (e_neighbor_y > y1_top) {
                            double total_rise = e_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap2 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap2 = lmax_h;
                        }
                        double er = min(cap1, cap2);
                        return base_y + er * max(curvature_func(t1, style->curvature), curvature_func(t2, style->curvature));
                    }
                    case TENT_SW: {
                        double t1 = (double)(z - lz0) / max(ldepth - 1.0 + loh_n, 1.0);
                        double t2 = (double)(x - lx0) / max(lwidth - 1.0 + loh_e, 1.0);
                        t1 = min(t1, 1.0); t2 = min(t2, 1.0);
                        double cap1;
                        if (s_neighbor_y > y1_top) {
                            double total_rise = s_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap1 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap1 = lmax_h;
                        }
                        double cap2;
                        if (w_neighbor_y > y1_top) {
                            double total_rise = w_neighbor_y - y1_top;
                            double layer_target = y1_top + total_rise * (layer + 1) / (double)style->layers;
                            cap2 = max(0.0, min(lmax_h, layer_target - base_y));
                        } else {
                            cap2 = lmax_h;
                        }
                        double er = min(cap1, cap2);
                        return base_y + er * max(curvature_func(t1, style->curvature), curvature_func(t2, style->curvature));
                    }
                }
                return base_y;
            };

            auto check_profile = [&](int x, int y, int z, RoofType type) -> bool {
                const auto& profile = style->surface_texture_profile;
                if (profile.size() == 0) return true;
                int pw = max((int)profile.width, 1);
                int ph = max((int)profile.height, 1);
                int pd = max((int)profile.depth, 1);
                int gx = 0, gy = 0, gz = 0;
                switch (type) {
                    case GABLE_EW: {
                        int ridge_z = (z0 + z1) / 2;
                        gx = mod_wrap(x - x0, pw);
                        gy = mod_wrap(y - y1_top, ph);
                        gz = mod_wrap(z - ridge_z, pd);
                        break;
                    }
                    case GABLE_NS: {
                        int ridge_x = (x0 + x1) / 2;
                        gx = mod_wrap(z - z0, pw);
                        gy = mod_wrap(y - y1_top, ph);
                        gz = mod_wrap(x - ridge_x, pd);
                        break;
                    }
                    case SHED_N: case SHED_S: {
                        gx = mod_wrap(x - x0, pw);
                        gy = mod_wrap(y - y1_top, ph);
                        gz = mod_wrap(z - z0, pd);
                        break;
                    }
                    case SHED_E: case SHED_W: {
                        gx = mod_wrap(z - z0, pw);
                        gy = mod_wrap(y - y1_top, ph);
                        gz = mod_wrap(x - x0, pd);
                        break;
                    }
                    default: {
                        gx = mod_wrap(x - x0, pw);
                        gy = mod_wrap(y - y1_top, ph);
                        gz = mod_wrap(z - z0, pd);
                        break;
                    }
                }
                return profile.has({gx, gy, gz});
            };

            string roof_material = "";

            double cumulative_base = 0.0;
            for (int layer = 0; layer < style->layers; ++layer) {
                int inset = layer * 3;
                int lx0 = x0 + (west  ? 0 : inset);
                int lx1 = x1 - (east  ? 0 : inset);
                int lz0 = z0 + (south ? 0 : inset);
                int lz1 = z1 - (north ? 0 : inset);
                if (lx0 >= lx1 || lz0 >= lz1) continue;

                int loh_n = max(oh_n - inset, 0);
                int loh_s = max(oh_s - inset, 0);
                int loh_e = max(oh_e - inset, 0);
                int loh_w = max(oh_w - inset, 0);

                double lwidth = lx1 - lx0 + 1.0;
                double ldepth = lz1 - lz0 + 1.0;
                double lmax_h = (style->pitch / 100.0) * 1.5 * max(lwidth, ldepth);
                int base_y = y1_top + (int)round(cumulative_base);
                cumulative_base += lmax_h * 0.5;

                int xs = lx0 - loh_w, xe = lx1 + loh_e;
                int zs = lz0 - loh_s, ze = lz1 + loh_n;

                Grid<int> tops;
                for (int x = xs; x <= xe; ++x) {
                    for (int z = zs; z <= ze; ++z) {
                        double h = roof_height_at(x, z, lx0, lx1, lz0, lz1,
                                                   lwidth, ldepth, base_y,
                                                   lmax_h, loh_n, loh_s, loh_e, loh_w,
                                                   roof_type, layer);
                        tops[{x, 0, z}] = (h <= base_y) ? -1 : (int)ceil(h);
                    }
                }

                for (int x = xs; x <= xe; ++x) {
                    for (int z = zs; z <= ze; ++z) {
                        int top = tops[{x, 0, z}];
                        if (top <= 0) continue;

                        int fill_from = base_y;
                        if (x > xs) { int nt = tops[{x-1, 0, z}]; if (nt > 0 && nt < top) fill_from = max(fill_from, nt); }
                        if (x < xe) { int nt = tops[{x+1, 0, z}]; if (nt > 0 && nt < top) fill_from = max(fill_from, nt); }
                        if (z > zs) { int nt = tops[{x, 0, z-1}]; if (nt > 0 && nt < top) fill_from = max(fill_from, nt); }
                        if (z < ze) { int nt = tops[{x, 0, z+1}]; if (nt > 0 && nt < top) fill_from = max(fill_from, nt); }

                        for (int y = fill_from + 1; y <= top; ++y) {
                            if (check_profile(x, y, z, roof_type)) {
                                place_block(blocks, x, y, z, roof_material);
                            }
                        }
                    }
                }

                int gx0 = lx0, gx1 = lx1, gz0 = lz0, gz1 = lz1;
                if (layer > 0) {
                    gx0 = lx0 + oh_w;
                    gx1 = lx1 - oh_e;
                    gz0 = lz0 + oh_s;
                    gz1 = lz1 - oh_n;
                }
                switch (roof_type) {
                    case GABLE_EW:
                    case SHED_N:
                    case SHED_S:
                        for (int gz = gz0; gz <= gz1; ++gz) {
                            int t = tops[{gx0, 0, gz}];
                            for (int gy = base_y + 1; gy < t; ++gy) {
                                if (!blocks.has({gx0, gy, gz})) place_block(blocks, gx0, gy, gz, "");
                            }
                            t = tops[{gx1, 0, gz}];
                            for (int gy = base_y + 1; gy < t; ++gy) {
                                if (!blocks.has({gx1, gy, gz})) place_block(blocks, gx1, gy, gz, "");
                            }
                        }
                        break;
                    case GABLE_NS:
                    case SHED_E:
                    case SHED_W:
                        for (int gx = gx0; gx <= gx1; ++gx) {
                            int t = tops[{gx, 0, gz0}];
                            for (int gy = base_y + 1; gy < t; ++gy) {
                                if (!blocks.has({gx, gy, gz0})) place_block(blocks, gx, gy, gz0, "");
                            }
                            t = tops[{gx, 0, gz1}];
                            for (int gy = base_y + 1; gy < t; ++gy) {
                                if (!blocks.has({gx, gy, gz1})) place_block(blocks, gx, gy, gz1, "");
                            }
                        }
                        break;
                    default:
                        break;
                }

                bool has_taller_neighbor = n_neighbor_y > y1_top || s_neighbor_y > y1_top
                    || e_neighbor_y > y1_top || w_neighbor_y > y1_top;
                if (has_taller_neighbor) {
                    int layer_max_y = base_y;
                    for (int x = xs; x <= xe; ++x)
                        for (int z = zs; z <= ze; ++z) {
                            int top = tops[{x, 0, z}];
                            if (top > layer_max_y) layer_max_y = top;
                        }
                    cumulative_base = (double)(layer_max_y - y1_top);
                }
            }
        }
    }

    static Grid<shared_ptr<Block>> make_column(
        shared_ptr<ColumnStyleElement> column_style,
        int width, // or diameter if round
        int height,
        int world_x = 0, int world_y = 0, int world_z = 0
    ) {
        Grid<shared_ptr<Block>> result(width, height, width);

        string material;
        if (!column_style->materials.empty()) {
            material = column_style->materials[0];
        }

        double cx = (width - 1) / 2.0;
        double cz = (width - 1) / 2.0;
        double radius = width / 2.0;
        double inner_r = max(0.0, radius - 1.0);

        // precompute shell cross-section, then duplicate for each y level
        vector<pair<int, int>> shell;
        for (int x = 0; x < width; ++x) {
            for (int z = 0; z < width; ++z) {
                bool on_shell = false;
                if (column_style->round) {
                    double dx = x - cx, dz = z - cz;
                    double dist2 = dx * dx + dz * dz;
                    on_shell = dist2 <= radius * radius + 0.5 &&
                               dist2 >= inner_r * inner_r - 0.5;
                } else {
                    on_shell = (x == 0 || x == width - 1 ||
                                z == 0 || z == width - 1);
                }
                if (on_shell) {
                    shell.emplace_back(x, z);
                }
            }
        }
        for (int y = 0; y < height; ++y) {
            for (auto& [x, z] : shell) {
                auto block = make_shared<Block>();
                block->position = make_shared<Position>(x + world_x, y + world_y, z + world_z);
                block->size = 1;
                block->material = material;
                result[{x + world_x, y + world_y, z + world_z}] = block;
            }
        }

        // lines / fluting (vertical grooves, push inward by 1)
        if (column_style->lines && width > 3 && !column_style->round) {
            auto is_indent = [&](int pos) -> bool {
                if (pos == 0 || pos == width - 1) return false;
                if (width % 2 == 1) return pos % 2 == 1;
                int layer = min(pos, width - 1 - pos);
                return ((width / 2) - layer) % 2 == 1;
            };

            // helper: move block from (sx,y,sz) to (dx,y,dz)
            auto push_block = [&](int sx, int sz, int dx, int dz, int y) {
                int wsx = sx + world_x, wsz = sz + world_z;
                int wdx = dx + world_x, wdz = dz + world_z;
                int wy = y + world_y;
                if (result.has({wsx, wy, wsz})) {
                    auto block = result[{wsx, wy, wsz}];
                    result.data.erase(Index3{wsx, wy, wsz});
                    block->position = make_shared<Position>(wdx, wy, wdz);
                    result[{wdx, wy, wdz}] = block;
                }
            };

            for (int y = 0; y < height; ++y) {
                for (int i = 1; i < width - 1; ++i) {
                    if (!is_indent(i)) continue;
                    push_block(i, width - 1, i, width - 2, y); // north
                    push_block(i, 0, i, 1, y);                 // south
                    push_block(width - 1, i, width - 2, i, y); // east
                    push_block(0, i, 1, i, y);                 // west
                }
            }
        }

        // frame
        if (column_style->frame) {
            auto s = make_shared<Position>(0 + world_x, 0 + world_y, 0 + world_z);
            auto e = make_shared<Position>(width - 1 + world_x, height - 1 + world_y, width - 1 + world_z);
            apply_frame_style(column_style->frame, s, e, 0, result);
            apply_frame_style(column_style->frame, s, e, 1, result);
            apply_frame_style(column_style->frame, s, e, 2, result);
            apply_frame_style(column_style->frame, s, e, 3, result);
        }

        // top / bottom caps
        int end_type_width = width+2;
        if (column_style->bottom_column_style_element) {
            int end_type_height = max(1.0, height*0.3);
            auto bottom_style = static_pointer_cast<ColumnStyleElement>(
                column_style->bottom_column_style_element);
            auto bottom_blocks = make_column(bottom_style, end_type_width, end_type_height);
            int ox = (width - end_type_width) / 2;
            int oz = (width - end_type_width) / 2;
            for (auto& [idx, block] : bottom_blocks.data) {
                int nx = idx.x + ox;
                int nz = idx.z + oz;
                block->position = make_shared<Position>(nx + world_x, idx.y, nz + world_z);
                result[{nx + world_x, idx.y, nz + world_z}] = block;
            }
        }

        if (column_style->top_column_style_element) {
            int end_type_height = max(1.0, height*0.1);
            auto top_style = static_pointer_cast<ColumnStyleElement>(
                column_style->top_column_style_element);
            auto top_blocks = make_column(top_style, end_type_width, end_type_height);
            int ox = (width - end_type_width) / 2;
            int oz = (width - end_type_width) / 2;
            int ny_offset = height - end_type_height;
            for (auto& [idx, block] : top_blocks.data) {
                int nx = idx.x + ox;
                int nz = idx.z + oz;
                int ny = idx.y + ny_offset;
                block->position = make_shared<Position>(nx + world_x, ny + world_y, nz + world_z);
                result[{nx + world_x, ny + world_y, nz + world_z}] = block;
            }
        }

        return result;
    }

    struct WallCutout {
        int64_t lo = 0, hi = 0, lo_y = 0, hi_y = 0;
    };

    static Grid<shared_ptr<Block>> make_wall(
        WallAngle angle,
        shared_ptr<WallStyleElement> wall_style,
        shared_ptr<Position> start,
        int length,
        int height,
        const vector<WallCutout>& cutouts = {}
    ) {
        int run_dx = 0, run_dz = 0, outward_dx = 0, outward_dz = 0;
        int shift_dx = 0, shift_dz = 0;
        bool is_diagonal = false;

        switch (angle) {
            case WallAngle::N:  run_dx=0;  run_dz=1;  outward_dx=-1; outward_dz=0;  break;
            case WallAngle::E:  run_dx=1;  run_dz=0;  outward_dx= 0; outward_dz=1;  break;
            case WallAngle::S:  run_dx=0;  run_dz=-1; outward_dx= 1; outward_dz=0;  break;
            case WallAngle::W:  run_dx=-1; run_dz=0;  outward_dx= 0; outward_dz=-1; break;
            case WallAngle::NE: run_dx=0;  run_dz=1;  outward_dx=-1; outward_dz=0;  shift_dx=1;  shift_dz=0;  is_diagonal=true; break;
            case WallAngle::SE: run_dx=1;  run_dz=0;  outward_dx= 0; outward_dz=1;  shift_dx=0;  shift_dz=-1; is_diagonal=true; break;
            case WallAngle::SW: run_dx=0;  run_dz=-1; outward_dx= 1; outward_dz=0;  shift_dx=-1; shift_dz=0;  is_diagonal=true; break;
            case WallAngle::NW: run_dx=-1; run_dz=0;  outward_dx= 0; outward_dz=-1; shift_dx=0;  shift_dz=1;  is_diagonal=true; break;
        }

        string material = "";
        if (!wall_style->materials.empty()) {
            material = wall_style->materials[0];
        }

        Grid<shared_ptr<Block>> result;

        int h = max(1, height);
        int t = max(1, wall_style->thickness);
        int i_slop = -wall_style->inward_slope * h;
        int o_slop = wall_style->outward_slope * h;
        int inner_off = -(t - 1) / 2 + i_slop;
        int outer_off = t / 2 + o_slop;

        // --- Step 1: Place wall blocks (cardinal base) ---
        for (int i = 0; i < length; ++i) {
            for (int hi = 0; hi < h; ++hi) {
                // check if this block is within any cutout
                bool cut = false;
                if (!cutouts.empty()) {
                    int coord = (run_dx != 0) ? (start->x + i * run_dx) : (start->z + i * run_dz);
                    int wy = start->y + hi;
                    for (auto& c : cutouts) {
                        if (coord >= c.lo && coord <= c.hi && wy >= c.lo_y && wy <= c.hi_y) {
                            cut = true;
                            break;
                        }
                    }
                }
                if (cut) continue;

                double in_shift = wall_style->inward_slope * hi;
                double out_shift = wall_style->outward_slope * hi;

                int min_t = (int)ceil((double)inner_off + in_shift);
                int max_t = (int)floor((double)outer_off - out_shift);
                if (min_t > max_t) continue;

                for (int ti = min_t; ti <= max_t; ++ti) {
                    int px = start->x + i * run_dx + ti * outward_dx;
                    int py = start->y + hi;
                    int pz = start->z + i * run_dz + ti * outward_dz;
                    place_block(result, px, py, pz, material);
                }
            }
        }

        // --- Step 1b: Wall design (decorative blocks on exterior face) ---
        if (wall_style->wall_design && wall_style->wall_design->pattern.size() > 0) {
            auto& design = *wall_style->wall_design;
            int pw = design.pattern.width;
            int ph = design.pattern.depth;
            if (pw > 0 && ph > 0) {
                for (int i = 0; i < length; ++i) {
                    for (int hi = 0; hi < h; ++hi) {
                        int px = i % pw;
                        int py = hi % ph;

                        if (!design.pattern.has({px, 0, py})) continue;

                        double out_shift = wall_style->outward_slope * hi;
                        int deco_ti = (int)floor((double)outer_off - out_shift) + 1;
                        int dx = start->x + i * run_dx + deco_ti * outward_dx;
                        int dy = start->y + hi;
                        int dz = start->z + i * run_dz + deco_ti * outward_dz;
                        place_block(result, dx, dy, dz, "");
                    }
                }
            }
        }

        // --- Step 2: Frame for cardinal base ---
        if (wall_style->frame && t > 0 && length > 0 && h > 0) {
            auto fs = static_pointer_cast<FrameStyleElement>(wall_style->frame);
            int ft = max(1, (int)fs->cross_section_grid.width);
            auto& grid = fs->cross_section_grid;

            string fmaterial = "";

            int ox = start->x + outer_off * outward_dx;
            int oz = start->z + outer_off * outward_dz;
            int y0 = start->y;
            int y1 = start->y + h - 1;

            bool has_outward_slope = wall_style->outward_slope != 0.0;
            bool has_inward_slope = wall_style->inward_slope != 0.0;

            if (!has_outward_slope && !has_inward_slope) {
                int fx0 = ox, fz0 = oz;
                int fx1 = ox + (length - 1) * run_dx;
                int fz1 = oz + (length - 1) * run_dz;

                int side = -1;
                if (outward_dx == -1 && outward_dz == 0) side = 3;
                else if (outward_dx == 0 && outward_dz == 1) side = 0;
                else if (outward_dx == 1 && outward_dz == 0) side = 1;
                else if (outward_dx == 0 && outward_dz == -1) side = 2;

                auto fs_start = make_shared<Position>(min(fx0, fx1), y0, min(fz0, fz1));
                auto fs_end = make_shared<Position>(max(fx0, fx1), y1, max(fz0, fz1));
                apply_frame_style(wall_style->frame, fs_start, fs_end, side, result);

            } else {
                // Sloping cardinal: custom frame using same outer-face computation as wall blocks.
                auto outer_pos_at_height = [&](int hi, int& ox, int& oz) {
                    double out_shift = wall_style->outward_slope * hi;
                    int max_t = (int)floor((double)outer_off - out_shift);
                    ox = start->x + max_t * outward_dx;
                    oz = start->z + max_t * outward_dz;
                };

                // Helper: place frame cross-section on an edge.
                // Each outward direction uses a different pattern matching apply_frame_style.
                // Bottom edge at y0
                int bx0, bz0;
                outer_pos_at_height(0, bx0, bz0);
                int z0_bot = bz0, z1_bot = bz0 + (length - 1) * run_dz;
                int x0_bot = bx0, x1_bot = bx0 + (length - 1) * run_dx;
                if (z0_bot > z1_bot) swap(z0_bot, z1_bot);
                if (x0_bot > x1_bot) swap(x0_bot, x1_bot);

                if (outward_dx == -1 && outward_dz == 0) { // west
                    for (int z = z0_bot; z <= z1_bot; ++z) {
                        for (int gx = 0; gx < ft; ++gx) {
                            for (int gz = 0; gz < ft; ++gz) {
                                int gy_idx = ((z - z0_bot) % ft + ft) % ft;
                                if (grid.has({gx, gy_idx, gz}))
                                    place_block(result, bx0 - 1 - gx, y0 + gz, z, fmaterial);
                            }
                        }
                    }
                } else if (outward_dx == 0 && outward_dz == 1) { // north
                    for (int x = x0_bot; x <= x1_bot; ++x) {
                        for (int gx = 0; gx < ft; ++gx) {
                            for (int gz = 0; gz < ft; ++gz) {
                                int gy_idx = ((x - x0_bot) % ft + ft) % ft;
                                if (grid.has({gx, gy_idx, gz}))
                                    place_block(result, x, y0 + gx, bz0 + 1 + gz, fmaterial);
                            }
                        }
                    }
                } else if (outward_dx == 1 && outward_dz == 0) { // east
                    for (int z = z0_bot; z <= z1_bot; ++z) {
                        for (int gx = 0; gx < ft; ++gx) {
                            for (int gz = 0; gz < ft; ++gz) {
                                int gy_idx = ((z - z0_bot) % ft + ft) % ft;
                                if (grid.has({gx, gy_idx, gz}))
                                    place_block(result, bx0 + 1 + gx, y0 + gz, z, fmaterial);
                            }
                        }
                    }
                } else if (outward_dx == 0 && outward_dz == -1) { // south
                    for (int x = x0_bot; x <= x1_bot; ++x) {
                        for (int gx = 0; gx < ft; ++gx) {
                            for (int gz = 0; gz < ft; ++gz) {
                                int gy_idx = ((x - x0_bot) % ft + ft) % ft;
                                if (grid.has({gx, gy_idx, gz}))
                                    place_block(result, x, y0 + gx, bz0 - 1 - gz, fmaterial);
                            }
                        }
                    }
                }

                // Side edges: at each height, place left and right end blocks
                int left_z0 = bz0, left_z1 = bz0;  // left end = step 0
                int right_z0 = bz0 + (length - 1) * run_dz;
                int right_z1 = right_z0;
                if (left_z0 > left_z1) swap(left_z0, left_z1);
                if (right_z0 > right_z1) swap(right_z0, right_z1);

                for (int hi = 0; hi < h; ++hi) {
                    int cx, cz;
                    outer_pos_at_height(hi, cx, cz);
                    int lx = cx, lz = cz;
                    int rx = cx + (length - 1) * run_dx;
                    int rz = cz + (length - 1) * run_dz;

                    for (int gx = 0; gx < ft; ++gx) {
                        for (int gz = 0; gz < ft; ++gz) {
                            int gy_idx = (hi % ft + ft) % ft;
                            int fy = start->y + hi;
                            if (outward_dx == -1 && outward_dz == 0) {
                                if (grid.has({gx, gy_idx, gz})) { place_block(result, lx - 1 - gx, fy, lz + gz, fmaterial); place_block(result, rx - 1 - gx, fy, rz - gz, fmaterial); }
                            } else if (outward_dx == 0 && outward_dz == 1) {
                                if (grid.has({gx, gy_idx, gz})) { place_block(result, lx, fy, lz + 1 + gx, fmaterial); place_block(result, rx, fy, rz - 1 - gx, fmaterial); }
                            } else if (outward_dx == 1 && outward_dz == 0) {
                                if (grid.has({gx, gy_idx, gz})) { place_block(result, lx + 1 + gx, fy, lz - gz, fmaterial); place_block(result, rx + 1 + gx, fy, rz + gz, fmaterial); }
                            } else if (outward_dx == 0 && outward_dz == -1) {
                                if (grid.has({gx, gy_idx, gz})) { place_block(result, lx, fy, lz - 1 - gx, fmaterial); place_block(result, rx, fy, rz + 1 - gx, fmaterial); }
                            }
                        }
                    }
                }

                // Top edge at y1
                if (h > 1) {
                    int tx0, tz0;
                    outer_pos_at_height(h - 1, tx0, tz0);
                    int z0_top = tz0, z1_top = tz0 + (length - 1) * run_dz;
                    int x0_top = tx0, x1_top = tx0 + (length - 1) * run_dx;
                    if (z0_top > z1_top) swap(z0_top, z1_top);
                    if (x0_top > x1_top) swap(x0_top, x1_top);

                    if (outward_dx == -1 && outward_dz == 0) { // west
                        for (int z = z0_top; z <= z1_top; ++z) {
                            for (int gx = 0; gx < ft; ++gx) {
                                for (int gz = 0; gz < ft; ++gz) {
                                    int gy_idx = ((z - z0_top) % ft + ft) % ft;
                                    if (grid.has({gx, gy_idx, gz}))
                                        place_block(result, tx0 - 1 - gx, y1 - gz, z, fmaterial);
                                }
                            }
                        }
                    } else if (outward_dx == 0 && outward_dz == 1) { // north
                        for (int x = x0_top; x <= x1_top; ++x) {
                            for (int gx = 0; gx < ft; ++gx) {
                                for (int gz = 0; gz < ft; ++gz) {
                                    int gy_idx = ((x - x0_top) % ft + ft) % ft;
                                    if (grid.has({gx, gy_idx, gz}))
                                        place_block(result, x, y1 - gx, tz0 + 1 + gz, fmaterial);
                                }
                            }
                        }
                    } else if (outward_dx == 1 && outward_dz == 0) { // east
                        for (int z = z0_top; z <= z1_top; ++z) {
                            for (int gx = 0; gx < ft; ++gx) {
                                for (int gz = 0; gz < ft; ++gz) {
                                    int gy_idx = ((z - z0_top) % ft + ft) % ft;
                                    if (grid.has({gx, gy_idx, gz}))
                                        place_block(result, tx0 + 1 + gx, y1 - gz, z, fmaterial);
                                }
                            }
                        }
                    } else if (outward_dx == 0 && outward_dz == -1) { // south
                        for (int x = x0_top; x <= x1_top; ++x) {
                            for (int gx = 0; gx < ft; ++gx) {
                                for (int gz = 0; gz < ft; ++gz) {
                                    int gy_idx = ((x - x0_top) % ft + ft) % ft;
                                    if (grid.has({gx, gy_idx, gz}))
                                        place_block(result, x, y1 - gx, tz0 - 1 - gz, fmaterial);
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Step 3: For diagonal walls, shift all blocks by step index ---
        if (is_diagonal) {
            // Build shifted blocks in a new grid, then replace result
            Grid<shared_ptr<Block>> shifted;
            for (auto& [idx, block] : result.data) {
                int i = (idx.z - start->z) * run_dz + (idx.x - start->x) * run_dx;
                // run_dx/dz are 0 or ±1, so this extracts i from the run-direction coordinate.
                // For wall blocks this is exact (the run coordinate is pure).
                // For frame side-edge blocks, the coordinate has small ±gz offsets,
                // which may cause i to be off by 0–2 blocks — acceptable for small cross-sections.
                int nx = idx.x + i * shift_dx;
                int nz = idx.z + i * shift_dz;
                block->position = make_shared<Position>(nx, idx.y, nz);
                shifted[{nx, idx.y, nz}] = block;
            }
            result = move(shifted);
        }

        return result;
    }


    static Grid<shared_ptr<Block>> make_jut_out(
        shared_ptr<JutOutStyleElement> jut_out_style,
        shared_ptr<Section> section_pegged_onto,
        int is_top_floor,
        int side_nesw,
        int width,
        int jut_out_dist,
        int largest_occupant
    ) {
        Grid<shared_ptr<Block>> result;

        int x0 = section_pegged_onto->start->x;
        int y0 = section_pegged_onto->start->y;
        int z0 = section_pegged_onto->start->z;
        int x1 = section_pegged_onto->end->x;
        int y1 = section_pegged_onto->end->y;
        int z1 = section_pegged_onto->end->z;

        string material = "";
        if (!jut_out_style->materials.empty()) {
            material = jut_out_style->materials[0];
        }

        // ---- Floor rectangle ----
        int floor_sx, floor_sz, floor_ex, floor_ez;
        int along_dx = 0, along_dz = 0, outward_dx = 0, outward_dz = 0;

        switch (side_nesw) {
            case 0: // N: along +x, outward +z
                floor_sx = x0 + (x1 - x0 + 1 - width) / 2;
                floor_sz = z1 + 1;
                floor_ex = floor_sx + width - 1;
                floor_ez = z1 + jut_out_dist;
                along_dx = 1; along_dz = 0;
                outward_dx = 0; outward_dz = 1;
                break;
            case 1: // E: along +z, outward +x
                floor_sz = z0 + (z1 - z0 + 1 - width) / 2;
                floor_sx = x1 + 1;
                floor_ez = floor_sz + width - 1;
                floor_ex = x1 + jut_out_dist;
                along_dx = 0; along_dz = 1;
                outward_dx = 1; outward_dz = 0;
                break;
            case 2: // S: along +x, outward -z
                floor_sx = x0 + (x1 - x0 + 1 - width) / 2;
                floor_sz = z0 - jut_out_dist;
                floor_ex = floor_sx + width - 1;
                floor_ez = z0 - 1;
                along_dx = 1; along_dz = 0;
                outward_dx = 0; outward_dz = -1;
                break;
            case 3: // W: along +z, outward -x
                floor_sz = z0 + (z1 - z0 + 1 - width) / 2;
                floor_sx = x0 - jut_out_dist;
                floor_ez = floor_sz + width - 1;
                floor_ex = x0 - 1;
                along_dx = 0; along_dz = 1;
                outward_dx = -1; outward_dz = 0;
                break;
        }

        int fnear_z = (outward_dz >= 0) ? min(floor_sz, floor_ez) : max(floor_sz, floor_ez);
        int ffar_z  = (outward_dz >= 0) ? max(floor_sz, floor_ez) : min(floor_sz, floor_ez);
        int fnear_x = (outward_dx >= 0) ? min(floor_sx, floor_ex) : max(floor_sx, floor_ex);
        int ffar_x  = (outward_dx >= 0) ? max(floor_sx, floor_ex) : min(floor_sx, floor_ex);

        // ---- Floor (iterate from near edge outward) ----
        for (int w = 0; w < width; ++w) {
            for (int o = 0; o < jut_out_dist; ++o) {
                int px = fnear_x + w * along_dx + o * outward_dx;
                int pz = fnear_z + w * along_dz + o * outward_dz;
                place_block(result, px, y0, pz, material);
            }
        }

        // ---- Bottom support ----
        bool has_bottom_support = jut_out_style->bottom_support || jut_out_style->smoothed_bottom_support;
        if (has_bottom_support && jut_out_style->bottom_support_height > 0) {
            int bh = jut_out_style->bottom_support_height;
            int base_y = y0 - bh;

            auto frame = jut_out_style->bottom_support;
            string fmaterial = "";

            for (int dy = 1; dy <= bh; ++dy) {
                int y = base_y + dy;
                double t = (double)dy / bh;
                int offset = (int)ceil((t - 1.0) * (t - 1.0) * jut_out_dist);

                if (jut_out_style->smoothed_bottom_support) {
                    for (int w = 0; w < width; ++w) {
                        int wx = fnear_x + w * along_dx;
                        int wz = fnear_z + w * along_dz;
                        for (int o = 0; o < offset; ++o) {
                            place_block_unique(result, wx + o * outward_dx, base_y-y, wz + o * outward_dz, fmaterial);
                        }
                    }
                } else if (jut_out_style->bottom_support) {
                    int ft = max(1, (int)frame->cross_section_grid.width);
                    for (int side = 0; side < 2; ++side) {
                        int w = (side == 0) ? 0 : (width - 1);
                        int wx = fnear_x + w * along_dx;
                        int wz = fnear_z + w * along_dz;
                        int bx = wx + offset * outward_dx;
                        int bz = wz + offset * outward_dz;

                        for (int gx = 0; gx < ft; ++gx) {
                            for (int gz = 0; gz < ft; ++gz) {
                                if (frame->cross_section_grid.has({gx, dy % ft, gz})) {
                                    place_block(result, bx + gx - ft / 2, y, bz + gz - ft / 2, fmaterial);
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- Floor frame (bottom support frame on free edges) ----
        if (jut_out_style->bottom_support) {
            auto frame = jut_out_style->bottom_support;
            int ft = max(1, (int)frame->cross_section_grid.width);

            string fmaterial = "";
            
            auto far_start = make_shared<Position>(min(ffar_x, fnear_x), y0, min(ffar_z, fnear_z));
            auto far_end   = make_shared<Position>(max(ffar_x, fnear_x), y0, max(ffar_z, fnear_z));

            int far_side = -1;
            if (side_nesw == 0) far_side = 0;
            else if (side_nesw == 1) far_side = 1;
            else if (side_nesw == 2) far_side = 2;
            else if (side_nesw == 3) far_side = 3;

            apply_frame_style(jut_out_style->bottom_support, far_start, far_end, far_side, result);

            int left_side = -1, right_side = -1;
            int left_start_x, left_start_z, right_start_x, right_start_z;

            switch (side_nesw) {
                case 0: // N: far_side = 0 (north)
                    left_side = 3; right_side = 1;
                    left_start_x = fnear_x; left_start_z = fnear_z; right_start_x = ffar_x; right_start_z = fnear_z;
                    break;
                case 1: // E: far_side = 1 (east)
                    left_side = 2; right_side = 0;
                    left_start_x = fnear_x; left_start_z = fnear_z; right_start_x = fnear_x; right_start_z = ffar_z;
                    break;
                case 2: // S: far_side = 2 (south)
                    left_side = 1; right_side = 3;
                    left_start_x = ffar_x; left_start_z = fnear_z; right_start_x = fnear_x; right_start_z = fnear_z;
                    break;
                case 3: // W: far_side = 3 (west)
                    left_side = 0; right_side = 2;
                    left_start_x = fnear_x; right_start_x = fnear_x;
                    left_start_z = ffar_z; right_start_z = fnear_z;
                    break;
            }

            auto left_start  = make_shared<Position>(min(left_start_x, fnear_x), y0, min(left_start_z, fnear_z));
            auto left_end    = make_shared<Position>(max(left_start_x, ffar_x), y0, max(left_start_z, ffar_z));
            auto right_start = make_shared<Position>(min(right_start_x, fnear_x), y0, min(right_start_z, fnear_z));
            auto right_end   = make_shared<Position>(max(right_start_x, ffar_x), y0, max(right_start_z, ffar_z));

            apply_frame_style(jut_out_style->bottom_support, left_start, left_end, left_side, result);
            apply_frame_style(jut_out_style->bottom_support, right_start, right_end, right_side, result);
        }

        // ---- Railings / walls around free edges ----
        if (jut_out_style->railing_or_wall) {
            struct WallDef { WallAngle angle; int sx, sz; int len; };
            vector<WallDef> walls;
            int wall_height = round(largest_occupant / 1.5);

            switch (side_nesw) {
                case 0:
                    walls.push_back({WallAngle::E, floor_sx, floor_ez, width});
                    walls.push_back({WallAngle::N, floor_sx, floor_sz, jut_out_dist});
                    walls.push_back({WallAngle::S, floor_ex, floor_ez, jut_out_dist});
                    break;
                case 1:
                    walls.push_back({WallAngle::S, floor_ex, floor_ez, width});
                    walls.push_back({WallAngle::W, floor_ex, floor_sz, jut_out_dist});
                    walls.push_back({WallAngle::E, floor_sx, floor_ez, jut_out_dist});
                    break;
                case 2:
                    walls.push_back({WallAngle::W, floor_ex, floor_sz, width});
                    walls.push_back({WallAngle::S, floor_sx, floor_ez, jut_out_dist});
                    walls.push_back({WallAngle::N, floor_ex, floor_sz, jut_out_dist});
                    break;
                case 3:
                    walls.push_back({WallAngle::N, floor_sx, floor_sz, width});
                    walls.push_back({WallAngle::E, floor_sx, floor_ez, jut_out_dist});
                    walls.push_back({WallAngle::W, floor_ex, floor_sz, jut_out_dist});
                    break;
            }

            for (auto& w : walls) {
                auto start = make_shared<Position>(w.sx, y0, w.sz);
                auto wall_grid = make_wall(w.angle, jut_out_style->railing_or_wall, start, w.len, wall_height);
                merge_blocks(result, wall_grid);
            }
        }

        // ---- Columns at corners ----
        if (jut_out_style->column && !is_top_floor) {
            int cw = max(1.0, width * 0.1);
            int ch = y1 - y0 + 1;
            int co = 1 + max(0, cw / 2);
            int max_co = width - cw;
            if (max_co < 0) max_co = 0;
            co = min(co, max_co);

            int cx[4], cz[4];

            // Near-edge columns [0,1] use full `co` offset from side edges.
            // Far-edge columns [2,3] use just `cw/2` from side edges (no default margin).
            int co_far = max(1, cw / 2);

            // All four corners: column occupies [cx, cx+cw-1] × [cz, cz+cw-1]
            // Far-edge offset by 1 block in; near edge butts against building wall.
            switch (side_nesw) {
                case 0: // N: along +x, outward +z. near_z=floor_sz, far_z=floor_ez
                    cx[0] = floor_sx + co;               cz[0] = floor_sz - (cw > 1 ? 1 : 0);
                    cx[1] = floor_ex - cw + 1 - co;      cz[1] = floor_sz - (cw > 1 ? 1 : 0);
                    cx[2] = floor_sx + co_far;           cz[2] = floor_ez - cw;
                    cx[3] = floor_ex - cw + 1 - co_far;  cz[3] = floor_ez - cw;
                    break;
                case 1: // E: along +z, outward +x. near_x=floor_sx, far_x=floor_ex
                    cx[0] = floor_sx - (cw > 1 ? 1 : 0);                cz[0] = floor_sz + co;
                    cx[1] = floor_sx - (cw > 1 ? 1 : 0);                cz[1] = floor_ez - cw + 1 - co;
                    cx[2] = floor_ex - cw;               cz[2] = floor_sz + co_far;
                    cx[3] = floor_ex - cw;               cz[3] = floor_ez - cw + 1 - co_far;
                    break;
                case 2: // S: along +x, outward -z. near_z=floor_ez, far_z=floor_sz
                    cx[0] = floor_sx + co;               cz[0] = floor_ez - cw + (cw > 1 ? 2 : 1);
                    cx[1] = floor_ex - cw + 1 - co;      cz[1] = floor_ez - cw + (cw > 1 ? 2 : 1);
                    cx[2] = floor_sx + co_far;           cz[2] = floor_sz + 1;
                    cx[3] = floor_ex - cw + 1 - co_far;  cz[3] = floor_sz + 1;
                    break;
                case 3: // W: along +z, outward -x. near_x=floor_ex, far_x=floor_sx
                    cx[0] = floor_ex - cw + (cw > 1 ? 2 : 1);           cz[0] = floor_sz + co;
                    cx[1] = floor_ex - cw + (cw > 1 ? 2 : 1);           cz[1] = floor_ez - cw + 1 - co;
                    cx[2] = floor_sx + 1;                cz[2] = floor_sz + co_far;
                    cx[3] = floor_sx + 1;                cz[3] = floor_ez - cw + 1 - co_far;
                    break;
            }

            for (int i = 0; i < 4; ++i) {
                auto col_grid = make_column(jut_out_style->column, cw, ch, cx[i], y0, cz[i]);
                merge_blocks(result, col_grid);
            }
        }

        // ---- Roof ----
        if (jut_out_style->roof && !is_top_floor) {
            auto jut_out_room = make_shared<Room>();
            jut_out_room->start = make_shared<Position>(min(floor_sx, floor_ex), y0, min(floor_sz, floor_ez));
            jut_out_room->end   = make_shared<Position>(max(floor_sx, floor_ex), y1, max(floor_sz, floor_ez));

            auto jut_out_section = make_shared<Section>();
            jut_out_section->rooms = {jut_out_room};
            jut_out_section->start = make_shared<Position>(0, 0, 0);
            jut_out_section->end   = make_shared<Position>(1, 1, 1);
            jut_out_section->scale = 1;
            jut_out_section->vertical_scale = 1;

            Grid<shared_ptr<Section>> sg(1, 1, 1);
            sg[{0, 0, 0}] = jut_out_section;
            apply_roof_style(jut_out_style->roof, sg, result, 0);
        }

        return result;
    }

    static void make_stair(
        shared_ptr<StairStyleElement> stair_style,
        shared_ptr<Section> lower_section,
        shared_ptr<Section> upper_section,
        int lower_side,
        Grid<shared_ptr<Block>>& blocks
    ) {
        if (!stair_style || !lower_section || !upper_section) return;
        if (lower_section->rooms.empty() || upper_section->rooms.empty()) return;

        int upper_side = (lower_side + 2) % 4;

        // --- Compute section world-space bounds for both sections ---
        auto section_bounds = [](const shared_ptr<Section>& sec) -> tuple<int,int,int,int> {
            int wmin_x = INT_MAX, wmax_x = INT_MIN;
            int wmin_z = INT_MAX, wmax_z = INT_MIN;
            for (auto& room : sec->rooms) {
                if (!room->start || !room->end) continue;
                wmin_x = min(wmin_x, (int)room->start->x);
                wmax_x = max(wmax_x, (int)room->end->x);
                wmin_z = min(wmin_z, (int)room->start->z);
                wmax_z = max(wmax_z, (int)room->end->z);
            }
            return {wmin_x, wmax_x, wmin_z, wmax_z};
        };

        auto [l_min_x, l_max_x, l_min_z, l_max_z] = section_bounds(lower_section);
        auto [u_min_x, u_max_x, u_min_z, u_max_z] = section_bounds(upper_section);

        // --- Find connection hallway rooms matching the connection side ---
        auto find_hallway_for_side = [](const shared_ptr<Section>& sec, int side,
                                        int wmin_x, int wmax_x, int wmin_z, int wmax_z) -> shared_ptr<Room> {
            shared_ptr<Room> fallback = nullptr;
            for (auto& room : sec->rooms) {
                if (!room->start || !room->end) continue;
                if (room->start->y != room->end->y) {
                    if (!fallback) fallback = room;
                    continue;
                }
                switch (side) {
                    case 0: if (room->start->z == wmin_z) return room; break;
                    case 1: if (room->start->x == wmin_x) return room; break;
                    case 2: if (room->end->z == wmax_z) return room; break;
                    case 3: if (room->end->x == wmax_x) return room; break;
                }
            }
            return fallback;
        };

        auto lower_pt = find_hallway_for_side(lower_section, lower_side, l_min_x, l_max_x, l_min_z, l_max_z);
        auto upper_pt = find_hallway_for_side(upper_section, upper_side, u_min_x, u_max_x, u_min_z, u_max_z);
        if (!lower_pt || !upper_pt) return;

        int sx = (lower_pt->start->x + lower_pt->end->x) / 2;
        int sy = lower_pt->start->y;
        int sz = (lower_pt->start->z + lower_pt->end->z) / 2;

        int ex = (upper_pt->start->x + upper_pt->end->x) / 2;
        int ey = upper_pt->start->y;
        int ez = (upper_pt->start->z + upper_pt->end->z) / 2;

        int height_diff = ey - sy;
        if (height_diff <= 0) return;

        int hw = lower_pt->end->x - lower_pt->start->x + 1;
        int hz = lower_pt->end->z - lower_pt->start->z + 1;
        int stair_width = min(hw, hz);
        int half_width = max(1, stair_width / 2);

        string material = stair_style->materials.empty() ? "" : stair_style->materials[0];
        int step_size = max(1, stair_style->stair_size_in_blocks);

        int dx = ex - sx;
        int dz = ez - sz;

        // --- Doorway obstruction check ---
        int bb_x0 = min(sx, ex) - half_width;
        int bb_x1 = max(sx, ex) + half_width;
        int bb_z0 = min(sz, ez) - half_width;
        int bb_z1 = max(sz, ez) + half_width;
        int bb_y0 = sy;
        int bb_y1 = ey;

        bool blocks_doorway = false;
        auto check_doors = [&](const shared_ptr<Section>& sec) -> bool {
            for (auto& room : sec->rooms) {
                if (!room->start || !room->end) continue;
                if (room->start->y == room->end->y) continue;
                for (int d = 0; d < 4; ++d) {
                    auto& hole = room->holes[d];
                    if (!hole.active) continue;
                    if (hole.lo_y > bb_y1 || hole.hi_y < bb_y0) continue;
                    if (d == 0) {
                        int fz = room->start->z;
                        if (fz >= bb_z0 && fz <= bb_z1 && hole.lo <= bb_x1 && hole.hi >= bb_x0)
                            return true;
                    } else if (d == 2) {
                        int fz = room->end->z;
                        if (fz >= bb_z0 && fz <= bb_z1 && hole.lo <= bb_x1 && hole.hi >= bb_x0)
                            return true;
                    } else if (d == 1) {
                        int fx = room->start->x;
                        if (fx >= bb_x0 && fx <= bb_x1 && hole.lo <= bb_z1 && hole.hi >= bb_z0)
                            return true;
                    } else if (d == 3) {
                        int fx = room->end->x;
                        if (fx >= bb_x0 && fx <= bb_x1 && hole.lo <= bb_z1 && hole.hi >= bb_z0)
                            return true;
                    }
                }
            }
            return false;
        };

        if (check_doors(lower_section) || check_doors(upper_section)) {
            blocks_doorway = true;
        }

        // --- Place stairs ---
        if (blocks_doorway) {
            // ---- Spiral staircase ----
            // Center at the section exit point for the connection side
            int radius = max(1, half_width);
            int cx = sx, cz = sz;
            switch (lower_side) {
                case 0: cz = l_min_z + radius; break;
                case 1: cx = l_min_x + radius; break;
                case 2: cz = l_max_z - radius; break;
                case 3: cx = l_max_x - radius; break;
            }
            int num_steps = max(1, height_diff / step_size);

            for (int step = 0; step <= num_steps; ++step) {
                int py = sy + min(step * step_size, height_diff);
                int wedge = step % 12;

                for (int ox = -radius; ox <= radius; ++ox) {
                    for (int oz = -radius; oz <= radius; ++oz) {
                        int bw = -1;
                        if (ox >= 0 && oz > 0) {
                            if (ox * 2 <= oz) bw = 0;
                            else if (oz * 2 <= ox) bw = 2;
                            else bw = 1;
                        } else if (ox > 0 && oz <= 0) {
                            int az = -oz;
                            if (az * 2 <= ox) bw = 3;
                            else if (ox * 2 <= az) bw = 5;
                            else bw = 4;
                        } else if (ox <= 0 && oz < 0) {
                            int ax = -ox, az = -oz;
                            if (ax * 2 <= az) bw = 6;
                            else if (az * 2 <= ax) bw = 8;
                            else bw = 7;
                        } else if (ox < 0 && oz >= 0) {
                            int ax = -ox;
                            if (oz * 2 <= ax) bw = 9;
                            else if (ax * 2 <= oz) bw = 11;
                            else bw = 10;
                        }
                        if (bw != wedge) continue;

                        place_block(blocks, cx + ox, py, cz + oz, material);
                    }
                }
            }

            // center pole: diameter = 1/4 of stair bounding box or 1, whichever is larger
            int pole_diameter = max(1, (2 * radius + 1) / 4);
            int pole_radius = pole_diameter / 2;
            for (int px = cx - pole_radius; px <= cx + pole_radius; ++px) {
                for (int pz = cz - pole_radius; pz <= cz + pole_radius; ++pz) {
                    for (int py = sy; py <= ey; ++py) {
                        place_block(blocks, px, py, pz, material);
                    }
                }
            }
        } else {
            // ---- Straight stair ----
            bool run_x = abs(dx) >= abs(dz);
            int num_steps = max(1, height_diff / step_size);

            for (int step = 0; step <= num_steps; ++step) {
                float t = num_steps > 0 ? (float)step / num_steps : 0;
                int py = sy + (int)round(t * height_diff);
                int px = sx + (int)round(t * dx);
                int pz = sz + (int)round(t * dz);

                for (int w = -half_width; w <= half_width; ++w) {
                    int bx = run_x ? px : px + w;
                    int bz = run_x ? pz + w : pz;
                    for (int dy = 0; dy < step_size; ++dy) {
                        place_block(blocks, bx, py + dy, bz, material);
                    }
                }
            }

            if (stair_style->railing_or_wall) {
                int rail_h = max(1, height_diff / 2);
                int len = (int)round(sqrt(dx * dx + dz * dz)) + stair_width;
                if (run_x) {
                    auto left_start  = make_shared<Position>(min(sx, ex), sy, sz - half_width - 1);
                    auto left_end    = make_shared<Position>(max(sx, ex), sy + rail_h - 1, sz - half_width - 1);
                    auto right_start = make_shared<Position>(min(sx, ex), sy, sz + half_width + 1);
                    auto right_end   = make_shared<Position>(max(sx, ex), sy + rail_h - 1, sz + half_width + 1);
                    if (abs(sx - ex) > 0) {
                        auto w1 = make_wall(WallAngle::W, stair_style->railing_or_wall, left_start, abs(sx - ex) + 1, rail_h, {});
                        auto w2 = make_wall(WallAngle::E, stair_style->railing_or_wall, right_start, abs(sx - ex) + 1, rail_h, {});
                        merge_blocks(blocks, w1);
                        merge_blocks(blocks, w2);
                    }
                } else {
                    auto front_start = make_shared<Position>(sx - half_width - 1, sy, min(sz, ez));
                    auto front_end   = make_shared<Position>(sx - half_width - 1, sy + rail_h - 1, max(sz, ez));
                    auto back_start  = make_shared<Position>(sx + half_width + 1, sy, min(sz, ez));
                    auto back_end    = make_shared<Position>(sx + half_width + 1, sy + rail_h - 1, max(sz, ez));
                    if (abs(sz - ez) > 0) {
                        auto w1 = make_wall(WallAngle::S, stair_style->railing_or_wall, front_start, abs(sz - ez) + 1, rail_h, {});
                        auto w2 = make_wall(WallAngle::N, stair_style->railing_or_wall, back_start, abs(sz - ez) + 1, rail_h, {});
                        merge_blocks(blocks, w1);
                        merge_blocks(blocks, w2);
                    }
                }
            }
        }
    }


    static void make_chimney(
        shared_ptr<ChimneyStyleElement> chimney_style,
        shared_ptr<Room> room,
        int side,
        Grid<shared_ptr<Block>>& blocks
    ) {
        if (!chimney_style || !room || !room->start || !room->end) return;

        string material = chimney_style->materials.empty() ? "" : chimney_style->materials[0];
        int fw = chimney_style->fire_place_width;
        int fh = chimney_style->fire_place_height;
        int depth = fw;
        int floor_y = room->start->y;

        int cx = (room->start->x + room->end->x) / 2;
        int cz = (room->start->z + room->end->z) / 2;

        int fx0, fx1, fz0, fz1, face_x0, face_x1, face_z0, face_z1;

        if (side == 2) {
            fx0 = cx - fw / 2; fx1 = fx0 + fw - 1;
            fz0 = room->end->z - depth + 1; fz1 = room->end->z;
            face_x0 = fx0; face_x1 = fx1; face_z0 = fz0; face_z1 = fz0;
        } else if (side == 0) {
            fx0 = cx - fw / 2; fx1 = fx0 + fw - 1;
            fz0 = room->start->z; fz1 = room->start->z + depth - 1;
            face_x0 = fx0; face_x1 = fx1; face_z0 = fz1; face_z1 = fz1;
        } else if (side == 3) {
            fz0 = cz - fw / 2; fz1 = fz0 + fw - 1;
            fx0 = room->end->x - depth + 1; fx1 = room->end->x;
            face_x0 = fx0; face_x1 = fx0; face_z0 = fz0; face_z1 = fz1;
        } else {
            fz0 = cz - fw / 2; fz1 = fz0 + fw - 1;
            fx0 = room->start->x; fx1 = room->start->x + depth - 1;
            face_x0 = fx1; face_x1 = fx1; face_z0 = fz0; face_z1 = fz1;
        }

        int col_cx = (fx0 + fx1) / 2;
        int col_cz = (fz0 + fz1) / 2;

        // --- Check door overlap ---
        auto& hole = room->holes[side];
        if (hole.active) {
            bool overlaps = false;
            if (side == 0 || side == 2) {
                overlaps = fx0 <= hole.hi && fx1 >= hole.lo &&
                           floor_y <= hole.hi_y && floor_y + fh >= hole.lo_y;
            } else {
                overlaps = fz0 <= hole.hi && fz1 >= hole.lo &&
                           floor_y <= hole.hi_y && floor_y + fh >= hole.lo_y;
            }
            if (overlaps) return;
        }

        // --- Place fireplace box ---
        for (int x = fx0; x <= fx1; ++x)
            for (int z = fz0; z <= fz1; ++z)
                for (int y = floor_y; y <= floor_y + fh; ++y)
                    place_block(blocks, x, y, z, material);

        // --- Opening in the room-facing face ---
        int open_w = fw * 3 / 5;
        int open_h = fh * 3 / 5;
        int open_x0 = col_cx - open_w / 2;
        int open_x1 = open_x0 + open_w - 1;
        int open_z0 = col_cz - open_w / 2;
        int open_z1 = open_z0 + open_w - 1;
        int open_y0 = floor_y + 1;
        int open_y1 = open_y0 + open_h - 1;

        if (side == 2 || side == 0) {
            int fz = face_z0;
            for (int x = open_x0; x <= open_x1; ++x)
                for (int y = open_y0; y <= open_y1; ++y)
                    blocks.data.erase({x, y, fz});
        } else {
            int fx = face_x0;
            for (int z = open_z0; z <= open_z1; ++z)
                for (int y = open_y0; y <= open_y1; ++y)
                    blocks.data.erase({fx, y, z});
        }

        int chimney_top = floor_y + fh + 1;
        int chimney_width = fw;

        // --- Scan upward to find roof surface ---
        int roof_surface = chimney_top - 1;
        for (int y = chimney_top; y < chimney_top + 200; ++y) {
            if (blocks.has({col_cx, y, col_cz})) {
                roof_surface = y;
            }
        }

        // --- Flue column (fireplace top to roof surface) ---
        int flue_height = roof_surface - chimney_top + 1;
        if (flue_height > 0 && chimney_style->interior_column) {
            int half = chimney_width / 2;
            auto col = make_column(chimney_style->interior_column, chimney_width, flue_height, col_cx - half, chimney_top, col_cz - half);
            merge_blocks(blocks, col);
        }

        // --- Exterior stack (above roof) ---
        int stack_height = max(3, chimney_width);
        if (chimney_style->exterior_column) {
            int half = chimney_width / 2;
            auto col = make_column(chimney_style->exterior_column, chimney_width, stack_height, col_cx - half, roof_surface + 1, col_cz - half);
            merge_blocks(blocks, col);
        }
    }

    enum class LightType { CHANDELIER, CEILING_SMALL, SCONCE };

    static shared_ptr<LightStyleElement> gen_light_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group,
        LightType type
    ) {
        auto light = make_shared<LightStyleElement>();
        light->type = StyleElementType::LIGHT;

        vector<string> light_materials = {
            BMaterial::GOLD, BMaterial::SILVER, BMaterial::COPPER,
            BMaterial::IRON, BMaterial::TITANIUM, BMaterial::PLATINUM
        };
        light->light_block_material = light_materials[Random::randInt(seed_spike, 0, light_materials.size())];

        bool has_frame = false;
        switch (type) {
            case LightType::CHANDELIER:    has_frame = Random::randBool(seed_spike + 1, 50); break;
            case LightType::CEILING_SMALL: has_frame = Random::randBool(seed_spike + 2, 30); break;
            case LightType::SCONCE:        has_frame = Random::randBool(seed_spike + 3, 20); break;
        }
        if (has_frame && style_group->frame && !style_group->frame->materials.empty()) {
            light->frame_material = style_group->frame->materials[0];
        }

        bool round = Random::randBool(seed_spike + 4);

        int w, h, d;
        switch (type) {
            case LightType::CHANDELIER:
                w = Random::randInt(seed_spike + 5, 3, 7);
                d = Random::randInt(seed_spike + 6, 3, 7);
                h = 1;
                break;
            case LightType::CEILING_SMALL:
                w = Random::randInt(seed_spike + 7, 1, 3);
                d = Random::randInt(seed_spike + 8, 1, 3);
                h = 1;
                break;
            case LightType::SCONCE:
                w = Random::randInt(seed_spike + 9, 1, 3);
                h = Random::randInt(seed_spike + 10, 1, 3);
                d = 1;
                break;
        }

        Grid<string> shape(w, h, d);
        int cx = (w - 1) / 2;
        int cy = (h - 1) / 2;
        int cz = (d - 1) / 2;

        if (type == LightType::SCONCE) {
            for (int x = 0; x < w; ++x)
                for (int y = 0; y < h; ++y) {
                    if (round) {
                        double dx = x - cx, dy = y - cy;
                        if (sqrt(dx * dx + dy * dy) <= max(w, h) / 2.0 + 0.5)
                            shape[{x, y, 0}] = light->light_block_material;
                    } else {
                        shape[{x, y, 0}] = light->light_block_material;
                    }
                }
        } else {
            for (int x = 0; x < w; ++x)
                for (int z = 0; z < d; ++z) {
                    if (round) {
                        double dx = x - cx, dz = z - cz;
                        if (sqrt(dx * dx + dz * dz) <= max(w, d) / 2.0 + 0.5)
                            shape[{x, 0, z}] = light->light_block_material;
                    } else {
                        shape[{x, 0, z}] = light->light_block_material;
                    }
                }
        }

        if (!light->frame_material.empty()) {
            for (int x = 0; x < w; ++x)
                for (int y = 0; y < h; ++y)
                    for (int z = 0; z < d; ++z) {
                        if (shape[{x, y, z}].empty()) continue;
                        bool perimeter = false;
                        if (type == LightType::SCONCE)
                            perimeter = (x == 0 || x == w - 1 || y == 0 || y == h - 1);
                        else
                            perimeter = (x == 0 || x == w - 1 || z == 0 || z == d - 1);
                        if (perimeter)
                            shape[{x, y, z}] = light->frame_material;
                    }
        }

        light->shape = shape;
        return light;
    }

    static void make_light(
        shared_ptr<LightStyleElement> style,
        int cx, int cy, int cz,
        int side,
        Grid<shared_ptr<Block>>& blocks
    ) {
        if (!style) return;
        auto& shape = style->shape;
        if (shape.size() == 0) return;

        int hw = shape.width / 2;
        int hh = shape.height / 2;
        int hd = shape.depth / 2;

        if (side < 0) {
            for (int sx = 0; sx < shape.width; ++sx)
                for (int sz = 0; sz < shape.depth; ++sz) {
                    string mat = shape[{sx, 0, sz}];
                    if (mat.empty()) continue;
                    place_block(blocks, cx - hw + sx, cy, cz - hd + sz, mat);
                }
        } else {
            for (int sx = 0; sx < shape.width; ++sx)
                for (int sy = 0; sy < shape.height; ++sy) {
                    string mat = shape[{sx, sy, 0}];
                    if (mat.empty()) continue;
                    int ox = sx - hw;
                    int oy = sy - hh;
                    if (side == 0 || side == 2)
                        place_block(blocks, cx + ox, cy + oy, cz, mat);
                    else
                        place_block(blocks, cx, cy + oy, cz + ox, mat);
                }
        }
    }

    static shared_ptr<WindowStyleElement> gen_window_style(
        int64_t seed_spike,
        shared_ptr<StyleGroup> style_group
    ) {
        auto win = make_shared<WindowStyleElement>();
        win->type = StyleElementType::WINDOW;
        win->min_width = 3;
        win->max_width = 12;
        win->min_height = 2;
        win->max_height = 8;
        win->sill_height = 1;

        if (style_group->frame && !style_group->frame->materials.empty())
            win->frame_material = style_group->frame->materials[0];
        else
            win->frame_material = BMaterial::DEFAULT;

        return win;
    }

    static void place_perimeter_frame(
        const string& material,
        int lo, int hi, int lo_y, int hi_y,
        int side, int wall_pos,
        bool include_sill,
        bool at_wall_face,
        Grid<shared_ptr<Block>>& grid
    ) {
        // at_wall_face=true: place at wall plane (inside the opening — left/right jambs only)
        // at_wall_face=false: place at exterior offset (door frames that shouldn't narrow passage)
        int pos = at_wall_face ? wall_pos : (wall_pos);
        if (at_wall_face) {
            // clear room blocks at the entire opening (room get_blocks doesn't know about windows)
            if (side == 0 || side == 2) {
                for (int x = lo; x <= hi; ++x)
                    for (int y = lo_y; y <= hi_y; ++y)
                        grid.data.erase({x, y, pos});
            } else {
                for (int z = lo; z <= hi; ++z)
                    for (int y = lo_y; y <= hi_y; ++y)
                        grid.data.erase({pos, y, z});
            }
            // place vertical jambs at edges of opening
            if (side == 0 || side == 2) {
                for (int y = lo_y; y <= hi_y; ++y) {
                    place_block(grid, lo, y, pos, material);
                    place_block(grid, hi, y, pos, material);
                }
            } else {
                for (int y = lo_y; y <= hi_y; ++y) {
                    place_block(grid, pos, y, lo, material);
                    place_block(grid, pos, y, hi, material);
                }
            }
        } else {
            // exterior placement: full perimeter (jambs + header + optional sill)
            if (side == 0 || side == 2) {
                for (int y = lo_y; y <= hi_y; ++y) {
                    place_block(grid, lo, y, pos, material);
                    place_block(grid, hi, y, pos, material);
                }
                for (int x = lo; x <= hi; ++x)
                    place_block(grid, x, hi_y, pos, material);
                if (include_sill)
                    for (int x = lo; x <= hi; ++x)
                        place_block(grid, x, lo_y, pos, material);
            } else {
                for (int y = lo_y; y <= hi_y; ++y) {
                    place_block(grid, pos, y, lo, material);
                    place_block(grid, pos, y, hi, material);
                }
                for (int z = lo; z <= hi; ++z)
                    place_block(grid, pos, hi_y, z, material);
                if (include_sill)
                    for (int z = lo; z <= hi; ++z)
                        place_block(grid, pos, lo_y, z, material);
            }
        }
    }

    // UTILITY

    static void place_block(Grid<shared_ptr<Block>>& grid, int px, int py, int pz, const string& material) {
        auto block = make_shared<Block>();
        block->position = make_shared<Position>(px, py, pz);
        block->size = 1;
        block->material = material;
        grid[{px, py, pz}] = block;
    }

    static void place_block_unique(Grid<shared_ptr<Block>>& grid, int px, int py, int pz, const string& material) {
        if (!grid.has({px, py, pz})) {
            place_block(grid, px, py, pz, material);
        }
    }

    static void merge_blocks(Grid<shared_ptr<Block>>& dest, Grid<shared_ptr<Block>>& src) {
        for (auto& [idx, block] : src.data) {
            if (!dest.has(idx)) {
                dest[idx] = block;
            }
        }
    }

    // Place a prefab (Grid<string> shape) against a wall in a room.
    // The prefab coordinate system: lx = along wall, ly = vertical, lz = into room from wall.
    // Tries all 4 wall sides in shuffled order, skips if it blocks a door hole.
    // floor_offset: how many blocks above room floor ly=0 maps to (1 for beds, 0 for counters).
    // centered: if true, prefab is centered along the wall; if false, flush to the wall's min corner.
    static void place_prefab_against_wall(
        const Grid<string>& prefab,
        shared_ptr<Room> room,
        Grid<shared_ptr<Block>>& blocks,
        int floor_offset = 1,
        bool centered = true
    ) {
        if (!room || !room->start || !room->end) return;
        if (prefab.width <= 0 || prefab.height <= 0 || prefab.depth <= 0) return;

        int rx0 = room->start->x, rx1 = room->end->x;
        int rz0 = room->start->z, rz1 = room->end->z;
        int ry0 = room->start->y;

        vector<int> sides = {0, 1, 2, 3};
        for (int i = 0; i < 4; ++i) {
            int j = (rx0 * 7 + rz0 * 31 + i * 5) % (4 - i);
            std::swap(sides[i], sides[i + j]);
        }

        for (int side : sides) {
            int cx = (rx0 + rx1) / 2;
            int cz = (rz0 + rz1) / 2;

            int fx0, fx1, fz0, fz1;

            if (side == 0 || side == 2) {
                fx0 = centered ? (cx - prefab.width / 2) : rx0;
                fx1 = fx0 + prefab.width - 1;
                fz0 = (side == 0) ? rz0 : rz1 - prefab.depth + 1;
                fz1 = fz0 + prefab.depth - 1;
            } else {
                fz0 = centered ? (cz - prefab.width / 2) : rz0;
                fz1 = fz0 + prefab.width - 1;
                fx0 = (side == 1) ? rx0 : rx1 - prefab.depth + 1;
                fx1 = fx0 + prefab.depth - 1;
            }

            if (fx0 < rx0 || fx1 > rx1 || fz0 < rz0 || fz1 > rz1)
                continue;

            bool blocked = false;
            for (int d = 0; d < 4; ++d) {
                if (!room->holes[d].active) continue;
                auto& h = room->holes[d];

                int door_x0, door_x1, door_z0, door_z1;
                if (d == 0 || d == 2) {
                    door_x0 = h.lo; door_x1 = h.hi;
                    door_z0 = door_z1 = (d == 0) ? rz0 : rz1;
                } else {
                    door_z0 = h.lo; door_z1 = h.hi;
                    door_x0 = door_x1 = (d == 1) ? rx0 : rx1;
                }

                int furn_y1 = ry0 + prefab.height - 1 + floor_offset;
                if (fx1 >= door_x0 && fx0 <= door_x1 &&
                    fz1 >= door_z0 && fz0 <= door_z1 &&
                    furn_y1 >= h.lo_y && ry0 <= h.hi_y) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            {
                int furn_top = ry0 + floor_offset + prefab.height - 1;
                if (furn_top > room->end->y) continue;
            }

            {
                bool overlap = false;
                for (int lx = 0; lx < prefab.width && !overlap; ++lx) {
                    for (int ly = 0; ly < prefab.height && !overlap; ++ly) {
                        for (int lz = 0; lz < prefab.depth && !overlap; ++lz) {
                            if (prefab[{lx, ly, lz}].empty()) continue;
                            int wx, wy, wz;
                            wy = ry0 + ly + floor_offset;
                            if (side == 0 || side == 2) {
                                wx = fx0 + lx;
                                wz = fz0 + lz;
                            } else {
                                wx = fx0 + lz;
                                wz = fz0 + lx;
                            }
                            if (wy == ry0) continue;
                            if (wx == rx0 || wx == rx1 || wz == rz0 || wz == rz1) continue;
                            if (blocks.has({wx, wy, wz})) {
                                overlap = true;
                            }
                        }
                    }
                }
                if (overlap) continue;
            }

            for (int lx = 0; lx < prefab.width; ++lx) {
                for (int ly = 0; ly < prefab.height; ++ly) {
                    for (int lz = 0; lz < prefab.depth; ++lz) {
                        string mat = prefab[{lx, ly, lz}];
                        if (mat.empty()) continue;

                        int wx, wy, wz;
                        wy = ry0 + ly + floor_offset;
                        if (side == 0 || side == 2) {
                            wx = fx0 + lx;
                            wz = fz0 + lz;
                        } else {
                            wx = fx0 + lz;
                            wz = fz0 + lx;
                        }
                        place_block(blocks, wx, wy, wz, "");
                    }
                }
            }
            return;
        }
    }

    // Place a prefab centered in the room (for tables, etc.).
    // Skips if it would block any active door hole or overlap existing blocks.
    static void place_prefab_centered(
        const Grid<string>& prefab,
        shared_ptr<Room> room,
        Grid<shared_ptr<Block>>& blocks,
        int floor_offset = 1
    ) {
        if (!room || !room->start || !room->end) return;
        if (prefab.width <= 0 || prefab.height <= 0 || prefab.depth <= 0) return;

        int rx0 = room->start->x, rx1 = room->end->x;
        int rz0 = room->start->z, rz1 = room->end->z;
        int ry0 = room->start->y;

        int cx = (rx0 + rx1) / 2;
        int cz = (rz0 + rz1) / 2;

        int sx = cx - prefab.width / 2;
        int sz = cz - prefab.depth / 2;
        int ex = sx + prefab.width - 1;
        int ez = sz + prefab.depth - 1;

        if (sx < rx0 || ex > rx1 || sz < rz0 || ez > rz1)
            return;

        {
            int furn_top = ry0 + floor_offset + prefab.height - 1;
            if (furn_top > room->end->y) return;
        }

        for (int d = 0; d < 4; ++d) {
            if (!room->holes[d].active) continue;
            auto& h = room->holes[d];

            int door_x0, door_x1, door_z0, door_z1;
            if (d == 0 || d == 2) {
                door_x0 = h.lo; door_x1 = h.hi;
                door_z0 = door_z1 = (d == 0) ? rz0 : rz1;
            } else {
                door_z0 = h.lo; door_z1 = h.hi;
                door_x0 = door_x1 = (d == 1) ? rx0 : rx1;
            }

            int furn_y1 = ry0 + prefab.height - 1 + floor_offset;
            if (ex >= door_x0 && sx <= door_x1 &&
                ez >= door_z0 && sz <= door_z1 &&
                furn_y1 >= h.lo_y && ry0 <= h.hi_y) {
                return;
            }
        }

        {
            bool overlap = false;
            for (int lx = 0; lx < prefab.width && !overlap; ++lx) {
                for (int ly = 0; ly < prefab.height && !overlap; ++ly) {
                    for (int lz = 0; lz < prefab.depth && !overlap; ++lz) {
                        if (prefab[{lx, ly, lz}].empty()) continue;
                        int wx = sx + lx;
                        int wy = ry0 + ly + floor_offset;
                        int wz = sz + lz;
                        if (wy == ry0) continue;
                        if (wx == rx0 || wx == rx1 || wz == rz0 || wz == rz1) continue;
                        if (blocks.has({wx, wy, wz})) {
                            overlap = true;
                        }
                    }
                }
            }
            if (overlap) return;
        }

        for (int lx = 0; lx < prefab.width; ++lx) {
            for (int ly = 0; ly < prefab.height; ++ly) {
                for (int lz = 0; lz < prefab.depth; ++lz) {
                    string mat = prefab[{lx, ly, lz}];
                    if (mat.empty()) continue;

                    int wx = sx + lx;
                    int wy = ry0 + ly + floor_offset;
                    int wz = sz + lz;
                    place_block(blocks, wx, wy, wz, "");
                }
            }
        }
    }

    // --- Prefab furniture generators ---

    static Grid<string> make_prefab_bed() {
        Grid<string> g(4, 2, 2);
        g[{0, 0, 0}] = "x";
        g[{0, 0, 1}] = "x";
        g[{3, 0, 0}] = "x";
        g[{3, 0, 1}] = "x";
        for (int lx = 0; lx < 4; ++lx)
            for (int lz = 0; lz < 2; ++lz)
                g[{lx, 1, lz}] = "x";
        return g;
    }

    static Grid<string> make_prefab_table_with_chairs() {
        Grid<string> g(5, 3, 5);
        // Table legs at ly=0
        g[{1, 0, 1}] = "x";
        g[{1, 0, 3}] = "x";
        g[{3, 0, 1}] = "x";
        g[{3, 0, 3}] = "x";
        // Table top at ly=1
        for (int lx = 1; lx <= 3; ++lx)
            for (int lz = 1; lz <= 3; ++lz)
                g[{lx, 1, lz}] = "x";
        // Chair south (lz=0)
        g[{2, 0, 0}] = "x";
        g[{2, 1, 0}] = "x";
        g[{2, 2, 0}] = "x";
        // Chair north (lz=4)
        g[{2, 0, 4}] = "x";
        g[{2, 1, 4}] = "x";
        g[{2, 2, 4}] = "x";
        return g;
    }

    static Grid<string> make_prefab_counter(int length, int occ_blocks) {
        int height = max(1, occ_blocks / 2);
        int depth = height;
        Grid<string> g(length, height, depth);
        for (int lx = 0; lx < length; ++lx)
            for (int ly = 0; ly < height; ++ly)
                for (int lz = 0; lz < depth; ++lz)
                    g[{lx, ly, lz}] = "x";
        return g;
    }

    static Grid<string> make_prefab_sink(int occ_blocks) {
        int height = max(1, occ_blocks / 2);
        int depth = height;
        Grid<string> g(2, height, depth);
        // solid base
        for (int lx = 0; lx < 2; ++lx)
            for (int ly = 0; ly < height; ++ly)
                for (int lz = 0; lz < depth; ++lz)
                    g[{lx, ly, lz}] = "x";
        // basin depression at top center (remove the center block at the top)
        if (height >= 2 && depth >= 2) {
            g[{0, height - 1, depth / 2}] = "";
            g[{1, height - 1, depth / 2}] = "";
        }
        return g;
    }

    static Grid<string> make_prefab_chair() {
        Grid<string> g(1, 2, 1);
        g[{0, 0, 0}] = "x"; // seat
        g[{0, 1, 0}] = "x"; // back
        return g;
    }

    static Grid<string> make_prefab_cupboard(int occ_blocks) {
        int w = max(1, occ_blocks / 2);
        int h = occ_blocks;
        int d = w;
        Grid<string> g(w, h, d);
        for (int lx = 0; lx < w; ++lx)
            for (int ly = 0; ly < h; ++ly)
                for (int lz = 0; lz < d; ++lz)
                    g[{lx, ly, lz}] = "x";
        return g;
    }

};


