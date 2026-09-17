#pragma once


#include "../blocks/Block.h"
#include "space/Planet.h"
#include "City.hpp"
#include "util/Random.h"
#include "util/Position.h"
#include "../blocks/WorldBlockPool.hpp"
#include "util/Util.hpp"
#include "util/Grid.hpp"
#include "util/BMaterial.hpp"
#include "styles/StyleGen.hpp"
#include "Room.hpp"

#include <memory>
#include <vector>

using namespace std;


struct Window {
    shared_ptr<Position> start;
    shared_ptr<Position> end;
};

/**
 * 
 */
struct BuildingGen {

    inline static double VERTICAL_SCALE_MULT = 0.3;
    inline static double OPEN_ROOM_VERTICAL_SCALE_MULT = VERTICAL_SCALE_MULT * 2;
    /*
    
        ## Properties
        - function (space in a factory for machines, places to cook and eat in restaurant)
        - pathways (where people are going to walk)
        - size
        - climate
        - repeat patterns (windows, columns, arches)
        - some interest (dome, spire, balcony)
        - style (show power for civic, temples for perfection, homes cozy)

    */


    BuildingGen() {}


    static void gen_city_style_groups(
        int64_t seed, 
        int64_t spike,
        shared_ptr<City> city
    ) {
        city->styles = {
            {BuildingType::HOME, {}},
            // {BuildingType::STORE, {}},
            // {BuildingType::RESTAURANT, {}},
            // {BuildingType::OFFICE, {}},
            // {BuildingType::GOVERNMENT, {}},
            // {BuildingType::FACTORY, {}},
            // {BuildingType::PLAY, {}},
            // {BuildingType::HOTEL, {}},
            // {BuildingType::RELIGIOUS, {}},
            // {BuildingType::APARTMENTS, {}}
        };


        for (auto& type_to_styles : city->styles) {
            BuildingType type = type_to_styles.first;
            vector<shared_ptr<StyleGroup>>& styles = type_to_styles.second;

            int iterations = 1;
            if (type == BuildingType::HOME) iterations = 3;

            for (int i = 0; i < iterations; i++) {
                shared_ptr<StyleGroup> group = std::make_shared<StyleGroup>();
                group->frame = StyleGen::gen_frame_style(seed+spike+514, city->largest_citizens);
                group->roof = StyleGen::gen_roof_style(city, seed+spike+12);
                group->column = StyleGen::gen_column_style(seed+spike+514, group);
                group->wall = StyleGen::gen_wall_style(seed+spike+817, group, 1, Random::randBool(seed+spike+4, 40));
                group->jut_out = StyleGen::gen_jut_out_style(seed+spike+929, group, city->largest_citizens/2);
                group->stair = StyleGen::gen_stair_style(seed+spike+42, group, false);
                group->chimney = StyleGen::gen_chimney_style(seed+spike+77, group);
                group->light = StyleGen::gen_light_style(seed+spike+42, group, StyleGen::LightType::CHANDELIER);
                group->light_small = StyleGen::gen_light_style(seed+spike+43, group, StyleGen::LightType::CEILING_SMALL);
                group->sconce = StyleGen::gen_light_style(seed+spike+44, group, StyleGen::LightType::SCONCE);
                group->window = StyleGen::gen_window_style(seed+spike+45, group);
                if (Random::randBool(seed+spike+111, 50)) {
                    int base_size = max(1, (int)round(city->largest_citizens / CoordinateConversion::BLOCK_SCALE));
                    group->wall_design = StyleGen::gen_wall_design_style(seed+spike+1000, base_size);
                }
                styles.push_back(group);
            }
        }

    }


    static vector<shared_ptr<Block>> gen_building(
        int64_t seed, 
        int64_t spike,      
        BuildingType type, 
        int64_t plot_width, 
        int64_t plot_depth, 
        double area_density, 
        shared_ptr<Planet> planet,
        shared_ptr<City> city
    ) {



        // SHAPE
        /*
            - homes: have bedrooms and kitchen, possibly bathrooms in or outside, and other living spaces
            - stores: one large room, storage room in back, checkout counters kind of things
            - restaurant: multiple large rooms with tables and such, kitchen, register perhaps
            - office: many small rooms, or large rooms divided into smaller offices
            - government: strong looking, large entrances, large council areas, small offices, service counters
            - factories: are more open and have one or two large rooms with maybe some small rooms, smokestacks
            - play: anything, though probably large open area with fun things
            - hotel: entrance and checkin counter, lots of small bedrooms
            - religious: large meeting hall, possible small rooms
        */

        double occupent_size = city->largest_citizens;
        double wealth = Random::randDouble(seed+spike+1, 0, 100);
        Grid<shared_ptr<Section>> sections;
        if (type == BuildingType::HOME) {
            sections = BuildingGen::home_layout(
                seed, 
                spike, 
                plot_width / CoordinateConversion::BLOCK_SCALE, 
                plot_depth / CoordinateConversion::BLOCK_SCALE, 
                area_density, 
                planet, 
                city, 
                occupent_size / CoordinateConversion::BLOCK_SCALE, 
                wealth
            );
        }

        // STYLES AND ORNIMANTATION
        vector<shared_ptr<StyleGroup>> building_styles = Util::copy(city->styles[type]);
        shared_ptr<StyleGroup> style_group = building_styles[Random::randInt(seed+spike, 0, building_styles.size())];

        // generate blocks from rooms, walls, and jut outs (single pass)
        Grid<shared_ptr<Block>> block_grid;
        unordered_set<shared_ptr<Section>> seen_section;

        shared_ptr<JutOutStyleElement> jut_out_style = style_group->jut_out;
        bool has_walls = (style_group->wall != nullptr);
        bool has_jut_outs = (jut_out_style != nullptr);
        bool skip_ground = has_jut_outs ? Random::randBool(seed + spike + 983, 30) : false;

        for (shared_ptr<Section> section : sections.values()) {
            if (section == nullptr || seen_section.find(section) != seen_section.end()) continue;
            seen_section.insert(section);

            // determine jut out placement
            int x0 = INT_MAX, x1 = INT_MIN;
            int z0 = INT_MAX, z1 = INT_MIN;
            int y0 = INT_MAX, y1 = INT_MIN;
            for (shared_ptr<Room> room : section->rooms) {
                if (room->start && room->end) {
                    x0 = min(x0, (int)room->start->x);
                    x1 = max(x1, (int)room->end->x);
                    z0 = min(z0, (int)room->start->z);
                    z1 = max(z1, (int)room->end->z);
                    y0 = min(y0, (int)room->start->y);
                    y1 = max(y1, (int)room->end->y);
                }
            }

            int sx = 0, sz = 0, sy = 0, s = 0, vs = 0, height = 0;
            bool north_neighbor = false, east_neighbor = false;
            bool south_neighbor = false, west_neighbor = false;
            bool top_floor = false;
            bool south_below = false, north_below = false, east_below = false, west_below = false;
            bool skip_jut = true;
            int jut_out_width = 3, jut_out_dist = 1;
            bool place_south = false, place_north = false, place_east = false, place_west = false;

            if (x0 != INT_MAX) {
                sx = section->start->x; sz = section->start->z;
                sy = section->start->y;
                s = section->scale; vs = section->vertical_scale;
                height = y1 - y0 + 1;

                for (int gx = sx; gx < sx + s && !north_neighbor; ++gx)
                    for (int gy = sy; gy < sy + vs && !north_neighbor; ++gy) {
                        auto c = sections[{gx, gy, sz + s}];
                        if (c && c != section) north_neighbor = true;
                    }
                for (int gz = sz; gz < sz + s && !east_neighbor; ++gz)
                    for (int gy = sy; gy < sy + vs && !east_neighbor; ++gy) {
                        auto c = sections[{sx + s, gy, gz}];
                        if (c && c != section) east_neighbor = true;
                    }
                for (int gx = sx; gx < sx + s && !south_neighbor; ++gx)
                    for (int gy = sy; gy < sy + vs && !south_neighbor; ++gy) {
                        auto c = sections[{gx, gy, sz - 1}];
                        if (c && c != section) south_neighbor = true;
                    }
                for (int gz = sz; gz < sz + s && !west_neighbor; ++gz)
                    for (int gy = sy; gy < sy + vs && !west_neighbor; ++gy) {
                        auto c = sections[{sx - 1, gy, gz}];
                        if (c && c != section) west_neighbor = true;
                    }

                top_floor = true;
                for (int gx = sx; gx < sx + s && top_floor; ++gx)
                    for (int gz = sz; gz < sz + s && top_floor; ++gz) {
                        auto c = sections[{gx, sy + vs, gz}];
                        if (c && c != section) top_floor = false;
                    }

                if (sy > 0) {
                    for (int gx = sx; gx < sx + s && !south_below; ++gx)
                        if (sections[{gx, sy - 1, sz - 1}]) south_below = true;
                    for (int gx = sx; gx < sx + s && !north_below; ++gx)
                        if (sections[{gx, sy - 1, sz + s}]) north_below = true;
                    for (int gz = sz; gz < sz + s && !east_below; ++gz)
                        if (sections[{sx + s, sy - 1, gz}]) east_below = true;
                    for (int gz = sz; gz < sz + s && !west_below; ++gz)
                        if (sections[{sx - 1, sy - 1, gz}]) west_below = true;
                }

                skip_jut = (skip_ground && sy == 0);

                if (has_jut_outs && !skip_jut) {
                    int face_width = max(x1 - x0 + 1, z1 - z0 + 1);
                    jut_out_width = max(3, face_width / 3);
                    jut_out_dist = Random::randInt(seed + spike + 116, occupent_size/2, 2*occupent_size);

                    place_south = !south_neighbor && !south_below && Random::randBool(seed + spike + 102, 80);
                    place_north = !north_neighbor && !north_below && Random::randBool(seed + spike + 103, 30);
                    place_east  = !east_neighbor  && !east_below  && Random::randBool(seed + spike + 104, 30);
                    place_west  = !west_neighbor  && !west_below  && Random::randBool(seed + spike + 105, 30);
                }

            }

            // --- add jut out door holes ---
            if (x0 != INT_MAX && has_jut_outs && !skip_jut) {
                int cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
                for (auto& room : section->rooms) {
                    int hw = room->door_width / 2;
                    int door_loy = room->start->y + 1;
                    int door_hiy = door_loy + room->door_height;
                    if (place_south && room->start->z == z0 && !room->holes[0].active
                        && room->start->x <= cx && room->end->x >= cx){
                        room->holes[0] = {true, cx - hw, cx + hw, door_loy, door_hiy};
                        room->doors.push_back(0);
                    }
                    if (place_north && room->end->z == z1 && !room->holes[2].active
                        && room->start->x <= cx && room->end->x >= cx){
                        room->holes[2] = {true, cx - hw, cx + hw, door_loy, door_hiy};
                        room->doors.push_back(2);
                    }
                    if (place_east && room->end->x == x1 && !room->holes[3].active
                        && room->start->z <= cz && room->end->z >= cz) {
                        room->holes[3] = {true, cz - hw, cz + hw, door_loy, door_hiy};
                        room->doors.push_back(3);
                    }
                    if (place_west && room->start->x == x0 && !room->holes[1].active
                        && room->start->z <= cz && room->end->z >= cz){
                        room->holes[1] = {true, cz - hw, cz + hw, door_loy, door_hiy};
                        room->doors.push_back(1);
                    }
                }
            }

            // generate rooms
            for (auto& room : section->rooms) {
                room->get_blocks(block_grid, "");
            }

            // --- lights ---
            if (style_group && x0 != INT_MAX) {
                for (auto& room : section->rooms) {
                    if (!room->start || !room->end) continue;
                    if (room->start->y == room->end->y) continue;

                    int cx = (room->start->x + room->end->x) / 2;
                    int cz = (room->start->z + room->end->z) / 2;
                    int cy = room->end->y - 1;
                    int rw = room->end->x - room->start->x + 1;
                    int rd = room->end->z - room->start->z + 1;

                    bool small_room = (rw <= 4 || rd <= 4);
                    auto light_style = small_room ? style_group->light_small : style_group->light;
                    if (light_style) {
                        StyleGen::make_light(light_style, cx, cy, cz, -1, block_grid);
                    }
                }
            }

            // --- furniture ---
            if (x0 != INT_MAX) {
                int occ_blocks = max(1, (int)round(city->largest_citizens / CoordinateConversion::BLOCK_SCALE));
                for (auto& room : section->rooms) {
                    if (!room->start || !room->end) continue;
                    if (room->start->y == room->end->y) continue;

                    if (room->types.find(RoomType::BEDROOM) != room->types.end()) {
                        auto prefab = StyleGen::make_prefab_bed();
                        StyleGen::place_prefab_against_wall(prefab, room, block_grid, 1, true);
                        auto wardrobe = StyleGen::make_prefab_cupboard(occ_blocks);
                        StyleGen::place_prefab_against_wall(wardrobe, room, block_grid, 0, true);
                    }

                    if (room->types.find(RoomType::KITCHEN) != room->types.end()) {
                        int rxl = room->end->x - room->start->x + 1;
                        int rzl = room->end->z - room->start->z + 1;
                        int counter_len = min(max(rxl, rzl), occ_blocks);
                        auto counter = StyleGen::make_prefab_counter(counter_len, occ_blocks);
                        StyleGen::place_prefab_against_wall(counter, room, block_grid, 0, false);
                        auto sink = StyleGen::make_prefab_sink(occ_blocks);
                        StyleGen::place_prefab_against_wall(sink, room, block_grid, 0, true);
                        auto cupboard = StyleGen::make_prefab_cupboard(occ_blocks);
                        StyleGen::place_prefab_against_wall(cupboard, room, block_grid, 0, true);
                    }

                    if (room->types.find(RoomType::DINING) != room->types.end()) {
                        auto table = StyleGen::make_prefab_table_with_chairs();
                        StyleGen::place_prefab_centered(table, room, block_grid, 1);
                    }

                    if (room->types.find(RoomType::BATHROOM) != room->types.end()) {
                        auto sink = StyleGen::make_prefab_sink(occ_blocks);
                        StyleGen::place_prefab_against_wall(sink, room, block_grid, 0, true);
                    }

                    if (room->types.find(RoomType::DINING) == room->types.end()) {
                        auto chair = StyleGen::make_prefab_chair();
                        StyleGen::place_prefab_against_wall(chair, room, block_grid, 1, true);
                    }
                }
            }

            // --- wall generation with cutouts ---
            if (has_walls && x0 != INT_MAX) {
                string frame_mat = (style_group->window && !style_group->window->frame_material.empty())
                    ? style_group->window->frame_material : BMaterial::DEFAULT;

                if (!north_neighbor) {
                    vector<StyleGen::WallCutout> cutouts;
                    vector<array<int,5>> win_frames; // lo, hi, lo_y, hi_y, side

                    for (auto& room : section->rooms) {
                        if (room->holes[2].active && room->end->z == z1)
                            cutouts.push_back({room->holes[2].lo, room->holes[2].hi, room->holes[2].lo_y, room->holes[2].hi_y});
                    }
                    for (auto& room : section->rooms) {
                        if (room->start->y == room->end->y && room->end->z == z1)
                            cutouts.push_back({room->start->x, room->end->x, y0, y1});
                    }
                    // windows on north face
                    auto place_north_windows = [&](int sg_lo, int sg_hi, int off, int rkey) {
                        if (sg_hi <= sg_lo) return;
                        int sg_len = sg_hi - sg_lo + 1;
                        auto& win = style_group->window;
                        int avail_h = (y1 - 1) - (y0 + win->sill_height) + 1;
                        if (sg_len < win->min_width || avail_h < win->min_height) return;
                        if (!Random::randBool(seed + spike + 301 + off, 80)) return;
                        int raw_w = (int)(sg_len * (0.40 + (double)Random::randInt(seed + spike + 302 + off + rkey, 0, 25) / 100.0));
                        int win_w = max((int)win->min_width, min((int)win->max_width, min(sg_len, raw_w)));
                        int raw_h = (int)(avail_h * (0.55 + (double)Random::randInt(seed + spike + 303 + off + rkey, 0, 25) / 100.0));
                        int win_h = max((int)win->min_height, min((int)win->max_height, min(avail_h, raw_h)));
                        int win_lo = sg_lo + (sg_len > win_w ? Random::randInt(seed + spike + 304 + off + rkey, 0, sg_len - win_w) : 0);
                        int win_hi = win_lo + win_w - 1;
                        int win_lo_y = y0 + 1 + (avail_h - win_h) / 2;
                        int win_hi_y = win_lo_y + win_h - 1;
                        cutouts.push_back({win_lo, win_hi, win_lo_y, win_hi_y});
                        win_frames.push_back({win_lo, win_hi, win_lo_y, win_hi_y, 2});
                    };
                    if (style_group && style_group->window) {
                        for (auto& room : section->rooms) {
                            if (!room->start || !room->end) continue;
                            if (room->start->y == room->end->y) continue;
                            if (room->end->z != z1) continue;
                            int wall_lo = max(x0 + 1, (int)room->start->x + 1);
                            int wall_hi = min(x1 - 1, (int)room->end->x - 1);
                            if (wall_hi <= wall_lo) continue;
                            int rkey = (room->end->x - room->start->x) * 1000 + (room->end->z - room->start->z);
                            if (room->holes[2].active) {
                                place_north_windows(wall_lo, room->holes[2].lo - 1, 0, rkey);
                                place_north_windows(room->holes[2].hi + 1, wall_hi, 1, rkey);
                            } else {
                                place_north_windows(wall_lo, wall_hi, 2, rkey);
                            }
                            room->window_sides |= (1 << 2);
                        }
                    }

                    auto start = make_shared<Position>(x0, y0, z1);
                    auto wall_blocks = StyleGen::make_wall(WallAngle::E, style_group->wall, start, x1 - x0 + 1, height, cutouts);
                    StyleGen::merge_blocks(block_grid, wall_blocks);
                    for (auto& wf : win_frames) {
                        for (int x = wf[0]; x <= wf[1]; ++x)
                            for (int y = wf[2]; y <= wf[3]; ++y)
                                block_grid.data.erase({x, y, z1});
                        StyleGen::place_perimeter_frame(frame_mat, wf[0], wf[1], wf[2], wf[3], wf[4], z1 + 1, false, false, block_grid);
                    }
                    for (auto& room : section->rooms) {
                        if (room->holes[2].active && room->end->z == z1) {
                            auto& h = room->holes[2];
                            StyleGen::place_perimeter_frame(frame_mat, h.lo, h.hi, h.lo_y, h.hi_y, 2, z1 + 1, false, false, block_grid);
                        }
                    }
                }
                if (!east_neighbor) {
                    vector<StyleGen::WallCutout> cutouts;
                    vector<array<int,5>> win_frames;

                    for (auto& room : section->rooms) {
                        if (room->holes[3].active && room->end->x == x1)
                            cutouts.push_back({room->holes[3].lo, room->holes[3].hi, room->holes[3].lo_y, room->holes[3].hi_y});
                    }
                    for (auto& room : section->rooms) {
                        if (room->start->y == room->end->y && room->end->x == x1)
                            cutouts.push_back({room->start->z, room->end->z, y0, y1});
                    }
                    auto place_east_windows = [&](int sg_lo, int sg_hi, int off, int rkey) {
                        if (sg_hi <= sg_lo) return;
                        int sg_len = sg_hi - sg_lo + 1;
                        auto& win = style_group->window;
                        int avail_h = (y1 - 1) - (y0 + win->sill_height) + 1;
                        if (sg_len < win->min_width || avail_h < win->min_height) return;
                        if (!Random::randBool(seed + spike + 311 + off, 80)) return;
                        int raw_w = (int)(sg_len * (0.40 + (double)Random::randInt(seed + spike + 312 + off + rkey, 0, 25) / 100.0));
                        int win_w = max((int)win->min_width, min((int)win->max_width, min(sg_len, raw_w)));
                        int raw_h = (int)(avail_h * (0.55 + (double)Random::randInt(seed + spike + 313 + off + rkey, 0, 25) / 100.0));
                        int win_h = max((int)win->min_height, min((int)win->max_height, min(avail_h, raw_h)));
                        int win_lo = sg_lo + (sg_len > win_w ? Random::randInt(seed + spike + 314 + off + rkey, 0, sg_len - win_w) : 0);
                        int win_hi = win_lo + win_w - 1;
                        int win_lo_y = y0 + 1 + (avail_h - win_h) / 2;
                        int win_hi_y = win_lo_y + win_h - 1;
                        cutouts.push_back({win_lo, win_hi, win_lo_y, win_hi_y});
                        win_frames.push_back({win_lo, win_hi, win_lo_y, win_hi_y, 3});
                    };
                    if (style_group && style_group->window) {
                        for (auto& room : section->rooms) {
                            if (!room->start || !room->end) continue;
                            if (room->start->y == room->end->y) continue;
                            if (room->end->x != x1) continue;
                            int wall_lo = max(z0 + 1, (int)room->start->z + 1);
                            int wall_hi = min(z1 - 1, (int)room->end->z - 1);
                            if (wall_hi <= wall_lo) continue;
                            int rkey = (room->end->x - room->start->x) * 1000 + (room->end->z - room->start->z);
                            if (room->holes[3].active) {
                                place_east_windows(wall_lo, room->holes[3].lo - 1, 0, rkey);
                                place_east_windows(room->holes[3].hi + 1, wall_hi, 1, rkey);
                            } else {
                                place_east_windows(wall_lo, wall_hi, 2, rkey);
                            }
                            room->window_sides |= (1 << 3);
                        }
                    }

                    auto start = make_shared<Position>(x1, y0, z1);
                    auto wall_blocks = StyleGen::make_wall(WallAngle::S, style_group->wall, start, z1 - z0 + 1, height, cutouts);
                    StyleGen::merge_blocks(block_grid, wall_blocks);
                    for (auto& wf : win_frames) {
                        for (int z = wf[0]; z <= wf[1]; ++z)
                            for (int y = wf[2]; y <= wf[3]; ++y)
                                block_grid.data.erase({x1, y, z});
                        StyleGen::place_perimeter_frame(frame_mat, wf[0], wf[1], wf[2], wf[3], wf[4], x1 + 1, false, false, block_grid);
                    }
                    for (auto& room : section->rooms) {
                        if (room->holes[3].active && room->end->x == x1) {
                            auto& h = room->holes[3];
                            StyleGen::place_perimeter_frame(frame_mat, h.lo, h.hi, h.lo_y, h.hi_y, 3, x1 + 1, false, false, block_grid);
                        }
                    }
                }
                if (!south_neighbor) {
                    vector<StyleGen::WallCutout> cutouts;
                    vector<array<int,5>> win_frames;

                    for (auto& room : section->rooms) {
                        if (room->holes[0].active && room->start->z == z0)
                            cutouts.push_back({room->holes[0].lo, room->holes[0].hi, room->holes[0].lo_y, room->holes[0].hi_y});
                    }
                    for (auto& room : section->rooms) {
                        if (room->start->y == room->end->y && room->start->z == z0)
                            cutouts.push_back({room->start->x, room->end->x, y0, y1});
                    }
                    auto place_south_windows = [&](int sg_lo, int sg_hi, int off, int rkey) {
                        if (sg_hi <= sg_lo) return;
                        int sg_len = sg_hi - sg_lo + 1;
                        auto& win = style_group->window;
                        int avail_h = (y1 - 1) - (y0 + win->sill_height) + 1;
                        if (sg_len < win->min_width || avail_h < win->min_height) return;
                        if (!Random::randBool(seed + spike + 321 + off, 80)) return;
                        int raw_w = (int)(sg_len * (0.40 + (double)Random::randInt(seed + spike + 322 + off + rkey, 0, 25) / 100.0));
                        int win_w = max((int)win->min_width, min((int)win->max_width, min(sg_len, raw_w)));
                        int raw_h = (int)(avail_h * (0.55 + (double)Random::randInt(seed + spike + 323 + off + rkey, 0, 25) / 100.0));
                        int win_h = max((int)win->min_height, min((int)win->max_height, min(avail_h, raw_h)));
                        int win_lo = sg_lo + (sg_len > win_w ? Random::randInt(seed + spike + 324 + off + rkey, 0, sg_len - win_w) : 0);
                        int win_hi = win_lo + win_w - 1;
                        int win_lo_y = y0 + 1 + (avail_h - win_h) / 2;
                        int win_hi_y = win_lo_y + win_h - 1;
                        cutouts.push_back({win_lo, win_hi, win_lo_y, win_hi_y});
                        win_frames.push_back({win_lo, win_hi, win_lo_y, win_hi_y, 0});
                    };
                    if (style_group && style_group->window) {
                        for (auto& room : section->rooms) {
                            if (!room->start || !room->end) continue;
                            if (room->start->y == room->end->y) continue;
                            if (room->start->z != z0) continue;
                            int wall_lo = max(x0 + 1, (int)room->start->x + 1);
                            int wall_hi = min(x1 - 1, (int)room->end->x - 1);
                            if (wall_hi <= wall_lo) continue;
                            int rkey = (room->end->x - room->start->x) * 1000 + (room->end->z - room->start->z);
                            if (room->holes[0].active) {
                                place_south_windows(wall_lo, room->holes[0].lo - 1, 0, rkey);
                                place_south_windows(room->holes[0].hi + 1, wall_hi, 1, rkey);
                            } else {
                                place_south_windows(wall_lo, wall_hi, 2, rkey);
                            }
                            room->window_sides |= (1 << 0);
                        }
                    }

                    auto start = make_shared<Position>(x1, y0, z0);
                    auto wall_blocks = StyleGen::make_wall(WallAngle::W, style_group->wall, start, x1 - x0 + 1, height, cutouts);
                    StyleGen::merge_blocks(block_grid, wall_blocks);
                    for (auto& wf : win_frames) {
                        for (int x = wf[0]; x <= wf[1]; ++x)
                            for (int y = wf[2]; y <= wf[3]; ++y)
                                block_grid.data.erase({x, y, z0});
                        StyleGen::place_perimeter_frame(frame_mat, wf[0], wf[1], wf[2], wf[3], wf[4], z0 - 1, false, false, block_grid);
                    }
                    for (auto& room : section->rooms) {
                        if (room->holes[0].active && room->start->z == z0) {
                            auto& h = room->holes[0];
                            StyleGen::place_perimeter_frame(frame_mat, h.lo, h.hi, h.lo_y, h.hi_y, 0, z0 - 1, false, false, block_grid);
                        }
                    }
                }
                if (!west_neighbor) {
                    vector<StyleGen::WallCutout> cutouts;
                    vector<array<int,5>> win_frames;

                    for (auto& room : section->rooms) {
                        if (room->holes[1].active && room->start->x == x0)
                            cutouts.push_back({room->holes[1].lo, room->holes[1].hi, room->holes[1].lo_y, room->holes[1].hi_y});
                    }
                    for (auto& room : section->rooms) {
                        if (room->start->y == room->end->y && room->start->x == x0)
                            cutouts.push_back({room->start->z, room->end->z, y0, y1});
                    }
                    auto place_west_windows = [&](int sg_lo, int sg_hi, int off, int rkey) {
                        if (sg_hi <= sg_lo) return;
                        int sg_len = sg_hi - sg_lo + 1;
                        auto& win = style_group->window;
                        int avail_h = (y1 - 1) - (y0 + win->sill_height) + 1;
                        if (sg_len < win->min_width || avail_h < win->min_height) return;
                        if (!Random::randBool(seed + spike + 331 + off, 80)) return;
                        int raw_w = (int)(sg_len * (0.40 + (double)Random::randInt(seed + spike + 332 + off + rkey, 0, 25) / 100.0));
                        int win_w = max((int)win->min_width, min((int)win->max_width, min(sg_len, raw_w)));
                        int raw_h = (int)(avail_h * (0.55 + (double)Random::randInt(seed + spike + 333 + off + rkey, 0, 25) / 100.0));
                        int win_h = max((int)win->min_height, min((int)win->max_height, min(avail_h, raw_h)));
                        int win_lo = sg_lo + (sg_len > win_w ? Random::randInt(seed + spike + 334 + off + rkey, 0, sg_len - win_w) : 0);
                        int win_hi = win_lo + win_w - 1;
                        int win_lo_y = y0 + 1 + (avail_h - win_h) / 2;
                        int win_hi_y = win_lo_y + win_h - 1;
                        cutouts.push_back({win_lo, win_hi, win_lo_y, win_hi_y});
                        win_frames.push_back({win_lo, win_hi, win_lo_y, win_hi_y, 1});
                    };
                    if (style_group && style_group->window) {
                        for (auto& room : section->rooms) {
                            if (!room->start || !room->end) continue;
                            if (room->start->y == room->end->y) continue;
                            if (room->start->x != x0) continue;
                            int wall_lo = max(z0 + 1, (int)room->start->z + 1);
                            int wall_hi = min(z1 - 1, (int)room->end->z - 1);
                            if (wall_hi <= wall_lo) continue;
                            int rkey = (room->end->x - room->start->x) * 1000 + (room->end->z - room->start->z);
                            if (room->holes[1].active) {
                                place_west_windows(wall_lo, room->holes[1].lo - 1, 0, rkey);
                                place_west_windows(room->holes[1].hi + 1, wall_hi, 1, rkey);
                            } else {
                                place_west_windows(wall_lo, wall_hi, 2, rkey);
                            }
                            room->window_sides |= (1 << 1);
                        }
                    }

                    auto start = make_shared<Position>(x0, y0, z0);
                    auto wall_blocks = StyleGen::make_wall(WallAngle::N, style_group->wall, start, z1 - z0 + 1, height, cutouts);
                    StyleGen::merge_blocks(block_grid, wall_blocks);
                    for (auto& wf : win_frames) {
                        for (int z = wf[0]; z <= wf[1]; ++z)
                            for (int y = wf[2]; y <= wf[3]; ++y)
                                block_grid.data.erase({x0, y, z});
                        StyleGen::place_perimeter_frame(frame_mat, wf[0], wf[1], wf[2], wf[3], wf[4], x0 - 1, false, false, block_grid);
                    }
                    for (auto& room : section->rooms) {
                        if (room->holes[1].active && room->start->x == x0) {
                            auto& h = room->holes[1];
                            StyleGen::place_perimeter_frame(frame_mat, h.lo, h.hi, h.lo_y, h.hi_y, 1, x0 - 1, false, false, block_grid);
                        }
                    }
                }
            }

            // --- jut out placement ---
            if (has_jut_outs && x0 != INT_MAX && !skip_jut) {
                auto block_section = make_shared<Section>();
                block_section->start = make_shared<Position>(x0, y0, z0);
                block_section->end = make_shared<Position>(x1, y1, z1);
                block_section->rooms = section->rooms;

                if (place_south) {
                    auto jb = StyleGen::make_jut_out(jut_out_style, block_section, top_floor ? 1 : 0, 2, jut_out_width, jut_out_dist, city->largest_citizens);
                    StyleGen::merge_blocks(block_grid, jb);
                }
                if (place_north) {
                    auto jb = StyleGen::make_jut_out(jut_out_style, block_section, top_floor ? 1 : 0, 0, jut_out_width, jut_out_dist, city->largest_citizens);
                    StyleGen::merge_blocks(block_grid, jb);
                }
                if (place_east) {
                    auto jb = StyleGen::make_jut_out(jut_out_style, block_section, top_floor ? 1 : 0, 1, jut_out_width, jut_out_dist, city->largest_citizens);
                    StyleGen::merge_blocks(block_grid, jb);
                }
                if (place_west) {
                    auto jb = StyleGen::make_jut_out(jut_out_style, block_section, top_floor ? 1 : 0, 3, jut_out_width, jut_out_dist, city->largest_citizens);
                    StyleGen::merge_blocks(block_grid, jb);
                }
            }

            // --- stair generation from section->stairs field ---
            if (style_group && style_group->stair && !section->stairs.empty() && x0 != INT_MAX) {
                for (auto& sc : section->stairs) {
                    if (sc.target && seen_section.find(sc.target) == seen_section.end()) {
                        StyleGen::make_stair(style_group->stair, section, sc.target, sc.side, block_grid);
                        
                        

                        Grid<shared_ptr<Block>> test;
                        StyleGen::make_stair(style_group->stair, section, sc.target, sc.side, test);
                        render_block_grid(test);
                    }
                }
            }

            // --- sconces (after walls/juts/stairs so they overwrite wall blocks) ---
            if (style_group && style_group->sconce && x0 != INT_MAX) {
                auto place_sconces = [&](int lo, int hi, int mid_y, int side, int wall_pos) {
                    auto& s = style_group->sconce;
                    if (side == 0 || side == 2) {
                        StyleGen::make_light(s, lo - 1, mid_y, wall_pos, side, block_grid);
                        StyleGen::make_light(s, hi + 1, mid_y, wall_pos, side, block_grid);
                    } else {
                        StyleGen::make_light(s, wall_pos, mid_y, lo - 1, side, block_grid);
                        StyleGen::make_light(s, wall_pos, mid_y, hi + 1, side, block_grid);
                    }
                };

                for (auto& room : section->rooms) {
                    if (!room->start || !room->end) continue;

                    if (!south_neighbor && room->holes[0].active && room->start->z == z0) {
                        auto& h = room->holes[0];
                        place_sconces(h.lo, h.hi, (h.lo_y + h.hi_y) / 2, 0, room->start->z - 1);
                    }
                    if (!west_neighbor && room->holes[1].active && room->start->x == x0) {
                        auto& h = room->holes[1];
                        place_sconces(h.lo, h.hi, (h.lo_y + h.hi_y) / 2, 1, room->start->x - 1);
                    }
                    if (!north_neighbor && room->holes[2].active && room->end->z == z1) {
                        auto& h = room->holes[2];
                        place_sconces(h.lo, h.hi, (h.lo_y + h.hi_y) / 2, 2, room->end->z + 1);
                    }
                    if (!east_neighbor && room->holes[3].active && room->end->x == x1) {
                        auto& h = room->holes[3];
                        place_sconces(h.lo, h.hi, (h.lo_y + h.hi_y) / 2, 3, room->end->x + 1);
                    }

                    if (room->start->y == room->end->y) {
                        int mid_y = (y0 + y1) / 2;
                        if (!south_neighbor && room->start->z == z0)
                            place_sconces(room->start->x, room->end->x, mid_y, 0, room->start->z - 1);
                        if (!west_neighbor && room->start->x == x0)
                            place_sconces(room->start->z, room->end->z, mid_y, 1, room->start->x - 1);
                        if (!north_neighbor && room->end->z == z1)
                            place_sconces(room->start->x, room->end->x, mid_y, 2, room->end->z + 1);
                        if (!east_neighbor && room->end->x == x1)
                            place_sconces(room->start->z, room->end->z, mid_y, 3, room->end->x + 1);
                    }
                }
            }

        }

        // roofs
        shared_ptr<RoofStyleElement> roof_style = style_group->roof;
        // StyleGen::apply_roof_style(roof_style, sections, block_grid, seed);

        // --- chimney generation ---
        if (style_group && style_group->chimney) {
            for (shared_ptr<Section> section : sections.values()) {
                if (!section || section->rooms.empty()) continue;

                int x0 = INT_MAX, x1 = INT_MIN;
                int z0 = INT_MAX, z1 = INT_MIN;
                for (auto& room : section->rooms) {
                    if (room->start && room->end) {
                        x0 = min(x0, (int)room->start->x);
                        x1 = max(x1, (int)room->end->x);
                        z0 = min(z0, (int)room->start->z);
                        z1 = max(z1, (int)room->end->z);
                    }
                }
                if (x0 == INT_MAX) continue;

                int room_idx = 0;
                for (auto& room : section->rooms) {
                    if (!room->start || !room->end) continue;
                    if (room->start->y == room->end->y) continue;
                    if (!Random::randBool(seed + spike + 989 + room_idx, 50)) { ++room_idx; continue; }

                    int wall_side = -1;
                    if (room->end->z == z1) wall_side = 2;
                    else if (room->start->z == z0) wall_side = 0;
                    else if (room->end->x == x1) wall_side = 3;
                    else if (room->start->x == x0) wall_side = 1;

                    if (wall_side == -1) { ++room_idx; continue; }

                    // window takes precedence over chimney
                    if (room->window_sides & (1 << wall_side)) { ++room_idx; continue; }

                    StyleGen::make_chimney(style_group->chimney, room, wall_side, block_grid);
                    ++room_idx;
                }
            }
        }


        return block_grid.values();

    }


    // LAYOUTS
    static Grid<shared_ptr<Section>> home_layout(
        int64_t seed, 
        int64_t spike, 
        int64_t plot_width, 
        int64_t plot_depth, 
        double area_density, // 0 - 100
        shared_ptr<Planet> planet, 
        shared_ptr<City> city,
        double occupent_size,
        int wealth // 0-100
    ) {
        vector<shared_ptr<Room>> rooms;


        // ROOMS

        // kitchen and eating area
        // seperate
        if (wealth > 50) {
            shared_ptr<Room> kitchen = std::make_shared<Room>();
            kitchen->scale = 1;
            kitchen->types = {RoomType::KITCHEN};
            rooms.push_back(kitchen);
            shared_ptr<Room> dining_room = std::make_shared<Room>();
            dining_room->scale = 1;
            dining_room->types = {RoomType::DINING};
            rooms.push_back(dining_room);
        }
        // combined
        else {
            shared_ptr<Room> kitchen = std::make_shared<Room>();
            kitchen->scale = 2;
            kitchen->types = {RoomType::KITCHEN, RoomType::DINING};
            rooms.push_back(kitchen);
        }
        
        // make bathrooms 
        int num_bedrooms = Random::randInt(seed + spike, 1, 10);
        int num_bathrooms = max(num_bedrooms * 0.4, 1.0);
        for (int i = 0; i < num_bathrooms; ++i) {
            shared_ptr<Room> bathroom = std::make_shared<Room>();
            bathroom->scale = 1;
            bathroom->types = {RoomType::BATHROOM};
            rooms.push_back(bathroom);
        }

        // make bedrooms
        for (int i = 0; i < num_bedrooms; ++i) {
            shared_ptr<Room> bedroom = std::make_shared<Room>();
            bedroom->scale = 1;
            bedroom->types = {RoomType::BEDROOM};
            rooms.push_back(bedroom);
        }
        
        // make vehicle area depending on city advancement
        double home_tech_advance = (city->technological_advancement + wealth) / 2.0;
        if (home_tech_advance > 50) {
            shared_ptr<Room> vehicle_area = std::make_shared<Room>();
            vehicle_area->scale = 1;
            vehicle_area->types = {RoomType::VEHICLE_AREA};
            rooms.push_back(vehicle_area);
        }

        // if more space make more large resting areas
        shared_ptr<Room> living_room = std::make_shared<Room>();
        living_room->scale = -1;
        living_room->types = {RoomType::FUN};
        rooms.push_back(living_room);


        // BUILDING SIZE

        // determine floors
        int desired_avg_floors = 6 * (area_density + wealth) / 200.0;
 
        // determine max building dimensions
        int building_width = ceil(Random::randDouble(seed+spike+1, plot_width * area_density / 100.0, plot_width));
        int building_depth = ceil(Random::randDouble(seed+spike+2, plot_depth * area_density / 100.0, plot_width));
        int max_dim = max(plot_width, plot_depth);
        int building_height = ceil(Random::randDouble(seed+spike+3, max_dim * area_density / 100.0, max_dim));
        int num_floors = building_height / (occupent_size * 1.2);


        return layout_space(
            BuildingType::HOME,
            area_density,
            building_width,
            building_depth,
            building_height,
            num_floors,
            occupent_size,
            rooms,
            (int) max(4.0, occupent_size),
            seed + spike
        );
    }

    static void restaurant_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        // make kitchen

        // make front counter or serving counter/bar

        // make a variety of open eating areas

        // make a bathroom or outhouse

        // make utility closet or worker area

        // make vehicle parking if lots of space is available
    }

    static void office_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        // make lobby

        // make floors with small rooms or cubicles or just desks

        // make some meeting areas of various sizes
    }

    static void government_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        // fancy lobby and entry way

        // possible large meeting area

        // offices

        // other meeting areas

        // public service areas (back room and service counter, waiting area)

        // if military building or castle add fortifications and armouries

        // possibly prisons as well
    }

    static void factory_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {

        // large warehouse areas

        // storage racks if needed

        // bathroom, rest rooms, offices
    }

    static void play_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {

        // different rooms (stages, arcades, jungle gyms, sport areas, movie areas)

        // bathrooms, employee areas
    }

    static void hotel_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        // fancy lobby

        // rooms

        // exericse, pool, restaurants
    }

    static void religious_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        // large gathering room

        // small rooms for meetings or clergy
    }

    static void apartment_layout(int64_t seed, int64_t spike, int64_t plot_size, double area_density, shared_ptr<Planet> planet, shared_ptr<City> city) {
        
        // multi room divisions 

        // exericse, pool, restaurants 
    }


      
    // HELPER FUNCTIONS
    /**
     * Creates the layout for a building. 
     * 
     * Rooms is a list of rooms, with the first room being the base room that all other rooms and hallways are connected to.
     * 
     * 
     */
    static Grid<shared_ptr<Section>> layout_space(
        BuildingType type,
        double area_density, // 0 - 100
        int building_width,
        int building_depth,
        int building_height,
        int num_floors,
        double largest_occupent,
        vector<shared_ptr<Room>>& rooms,
        int hallway_width,
        int seed_spike
    ) {

        // base size
        int base_size = max(1.0, largest_occupent);
        int room_size = base_size * 5;
        int building_width_in_rooms = max(1, building_width / room_size);
        int building_depth_in_rooms = max(1, building_depth / room_size);
        int building_height_in_rooms = max(1, building_height / room_size);

        // make grid and corner lists
        Grid<shared_ptr<Section>> grid(building_width_in_rooms, building_height_in_rooms, building_depth_in_rooms);
        vector<shared_ptr<Room>> end_rooms;


        // DETERMINE SECTIONS

        // make base room
        vector<int> possibles_scales = get_possibles_scales(building_width_in_rooms);
        int scale = possibles_scales[Random::randInt(seed_spike, 0, possibles_scales.size())];
        int start_x = Random::randInt(seed_spike+1, 0, building_width_in_rooms - scale);
        int start_z = Random::randInt(seed_spike+2, 0, building_depth_in_rooms - scale);
        shared_ptr<Section> base_section = std::make_shared<Section>();
        base_section->pattern = RoomPattern::OPEN_ROOM;
        base_section->start = std::make_shared<Position>(start_x, 0, start_z);
        base_section->scale = scale;
        base_section->vertical_scale = Random::randInt(seed_spike+3, 1, building_height_in_rooms);
        base_section->end = std::make_shared<Position>(
            base_section->start->x+base_section->scale,
            base_section->start->y+base_section->vertical_scale,
            base_section->start->z+base_section->scale
        );
        add_to_grid(
            building_width_in_rooms,
            building_depth_in_rooms,
            grid,
            base_section
        );
        base_section->pattern = RoomPattern::OPEN_ROOM;
        base_section->doors.push_back(0); // front


        // add sections to layout
        int total_rooms = 1;
        vector<shared_ptr<Section>> sections = {base_section};
        int spike = seed_spike;
        int rooms_this_floor = 0;
        int current_floor_level = 0;
        int floor_size = building_width_in_rooms * building_depth_in_rooms;
        int floor_vertical_scale = max(1, building_height_in_rooms / num_floors);
        double building_density = Random::randDouble(seed_spike + spike + 3, 0, area_density, 100);
        double floor_density = building_density;
        vector<shared_ptr<Section>> randomized_sections = Random::get_random_copy_list(sections, seed_spike);
        int r_s = 0;
        while(total_rooms < rooms.size() && r_s < randomized_sections.size()) {

            // add onto random section
            shared_ptr<Section> section = randomized_sections[r_s];
            ++r_s;
            vector<int> p_doors = option_door(section);
            vector<int> possible_doors = Random::get_random_copy_list(p_doors, seed_spike+r_s);
            if (!possible_doors.empty()) {
                for (int door_index : possible_doors) {
                    
                    vector<int> r_scales = Random::get_random_copy_list(possibles_scales, seed_spike+r_s);
                    for (int new_scale : r_scales) {
                        shared_ptr<Section> new_section = std::make_shared<Section>();
                        new_section->pattern = RoomPattern::HALLWAY;
                        new_section->scale = new_scale;
                        new_section->vertical_scale = new_scale;
                        new_section->start = section_position(new_section, section, door_index);
                        new_section->start->y = current_floor_level;
                        new_section->end = std::make_shared<Position>(
                            new_section->start->x+new_section->scale,
                            new_section->start->y+new_section->vertical_scale,
                            new_section->start->z+new_section->scale
                        );
                        

                        if (
                            spot_valid(
                                grid,
                                new_section->vertical_scale,
                                new_section->scale,
                                new_section->scale,
                                building_height_in_rooms,
                                building_width_in_rooms,
                                building_depth_in_rooms,
                                new_section->start->x,
                                new_section->start->y,
                                new_section->start->z
                            )
                        ) {
                            add_to_grid(
                                building_width_in_rooms,
                                building_depth_in_rooms,
                                grid,
                                new_section
                            );
                            sections.push_back(new_section);
                            randomized_sections = Random::get_random_copy_list(sections, seed_spike);
                            r_s = 0;
                            total_rooms += 3;
                            rooms_this_floor += new_section->scale * new_section->scale;
                            section->doors.push_back(door_index);
                            new_section->doors.push_back(opposite_door(door_index));
                            if (new_section->start->y != section->start->y) {
                                section->stairs.push_back({door_index, new_section});
                                new_section->stairs.push_back({opposite_door(door_index), section});
                            }
                            new_section->pattern = RoomPattern::NONE;
                        }
                    }
                }
            }


            // advance to next floor if we're getting full
            bool next_floor = false;
            double current_desntiry = 100.0 * (double) rooms_this_floor / (double) floor_size;
            if (current_desntiry >= floor_density) {
                next_floor = true;
            }

            if (next_floor) {
                current_floor_level += floor_vertical_scale;
                rooms_this_floor = 0;;
                floor_density *= Random::randDouble(seed_spike+spike+3, 0.95, 1.0);
                r_s = 0;
            }



            ++spike;
        }



        // SPLIT SECTIONS INTO ROOMS
        
    
        // sort rooms by size
        sort(
            rooms.begin(), 
            rooms.end(), 
            [](shared_ptr<Room> a, shared_ptr<Room> b) {
                return a->scale > b->scale;
            }
        );

        // sort sections by size
        sort(
            sections.begin(), 
            sections.end(), 
            [](shared_ptr<Section> a, shared_ptr<Section> b) {
                return (a->scale * a->scale * a->vertical_scale) > (b->scale * b->scale * b->vertical_scale);
            }
        );

        // get total building size
        float total_building_size = 0;
        for (shared_ptr<Section> section : sections) {
            int section_size = section->scale * section->scale * section->vertical_scale;
            total_building_size += section_size;
        }

        // assign rooms to sections
        for (int r = 0; r < rooms.size(); ++r) {
            shared_ptr<Room> room = rooms[r];
            
            /*
                determine if how full section is by comparing section size to total room size

                if space add to section

                else try next section
            */
            bool was_added = false;
            for (int s = 0; s < sections.size(); ++s) {
                shared_ptr<Section> section = sections[s];
                if (room->scale <= section->scale && section->rooms.size() < 4) {

                    // make sure open sections only have one room
                    if (section->pattern == RoomPattern::OPEN_ROOM && section->rooms.size() > 0) continue;

                    // add room to section
                    section->rooms.push_back(room);
                    was_added = true;

                    end_rooms.push_back(room);
                    break;
                }
            }

            // if you can't add it to a section, then combine with another room
            if (!was_added) {
                if (end_rooms.empty()) {
                    shared_ptr<Section> section = sections[Random::randInt(seed_spike + spike, 0, sections.size())];
                    section->rooms.push_back(room);
                    end_rooms.push_back(room);
                }
                else {
                    shared_ptr<Room> room_to_combine_with = end_rooms[Random::randInt(seed_spike + spike, 0, end_rooms.size())];
                    room_to_combine_with->types.insert(room->types.begin(), room->types.end());
                    spike++;
                }
            }
        }

        // define sizes of rooms in each sub section
        double DOOR_WIDTH = 0.6;
        int s = 0;
        for (shared_ptr<Section> section : sections) {

            s++;
            // if (s-1 != 3) continue;

            /*
                figure out how many rooms are in the section

                assign to random positions 0-3 back-left back-right front-left front-right

                combine empty room spaces into rooms
            
            */

            int section_start_x = section->start->x * room_size;
            int section_start_y = section->start->y * room_size * VERTICAL_SCALE_MULT; // vertical scale is half
            int section_start_z = section->start->z * room_size;
            int hallway_size = section->scale * base_size;
            int section_size = room_size * section->scale;
            int section_room_size = section_size / 2 - hallway_size / 2;
            int section_height = room_size * section->vertical_scale * VERTICAL_SCALE_MULT;
            if (section->pattern != RoomPattern::OPEN_ROOM) {

                // set room bounds
                unordered_map<int, shared_ptr<Room>> room_positions;
                unordered_map<int, shared_ptr<Room>> empty_rooms;
                vector<int> positions = {0,1,2,3};
                for (int i = 0; i < 4; ++i) {


                    int pos_index = Random::randInt(seed_spike+spike, 0, positions.size());
                    int position = positions[pos_index];
                    Util::swap_pop_remove_at(positions, pos_index);

                    shared_ptr<Room> room;
                    if (i < section->rooms.size()) {
                        room = section->rooms[i];
                    }
                    else {
                        room = std::make_shared<Room>();
                        room->types = {RoomType::EMPTY};
                        empty_rooms[position] = room;
                    }

                    room->door_width = max(1, (int)(base_size * DOOR_WIDTH));
                    room->door_height = max(1, min((int)(largest_occupent * 1.2), section_height - 1));
                    if (position == 0) {
                        room->doors = {3};
                        // if back hallway collapsed, also add south door to center hallway
                        if (!Util::contains(section->doors, 2)) {
                            room->doors.push_back(0);
                        }
                    } else if (position == 1) {
                        room->doors = {1};
                    } else if (position == 2) {
                        room->doors = {3, 2};
                    } else if (position == 3) {
                        room->doors = {1, 2};
                    }

                    room->end = nullptr;
                    // doors; // 0=front, 1=left, 2=back, 3=right
                    if (position == 0) { // back-left
                        room->start = std::make_shared<Position>(
                            section_start_x,
                            section_start_y,
                            section_start_z + section_room_size + hallway_size
                        );

                        // extend room if door isn't present
                        if (!Util::contains(section->doors, 2)) {
                            room->end = std::make_shared<Position>(
                                room->start->x + section_room_size + hallway_size,
                                room->start->y + section_height,
                                room->start->z + section_room_size
                            );
                        }
                    }
                    else if (position == 1) { // back-right
                        int z_mod = Util::contains(section->doors, 3)? hallway_size : 0;
                        room->start = std::make_shared<Position>(
                            section_start_x + section_room_size + hallway_size,
                            section_start_y,
                            section_start_z + section_room_size + z_mod
                        );

                        // extend room if door isn't present
                        if (!Util::contains(section->doors, 3)) {
                            room->end = std::make_shared<Position>(
                                room->start->x + section_room_size,
                                room->start->y + section_height,
                                room->start->z + section_room_size + hallway_size
                            );
                        }
                    }
                    else if (position == 2) { // front-left
                        room->start = std::make_shared<Position>(
                            section_start_x,
                            section_start_y,
                            section_start_z
                        );

                        // extend room if door isn't present
                        if (!Util::contains(section->doors, 1)) {
                            room->end = std::make_shared<Position>(
                                room->start->x + section_room_size,
                                room->start->y + section_height,
                                room->start->z + section_room_size + hallway_size
                            );
                        }
                    }
                    else if (position == 3) { // front-right
                        int x_mod = Util::contains(section->doors, 0)? hallway_size : 0;
                        room->start = std::make_shared<Position>(
                            section_start_x + section_room_size + x_mod,
                            section_start_y,
                            section_start_z
                        );

                        // extend room if door isn't present
                        if (!Util::contains(section->doors, 0)) {
                            room->end = std::make_shared<Position>(
                                room->start->x + section_room_size + hallway_size,
                                room->start->y + section_height,
                                room->start->z + section_room_size
                            );
                        }
                    }

                    if (room->end == nullptr) {
                        room->end = std::make_shared<Position>(
                            room->start->x + section_room_size,
                            room->start->y + section_height,
                            room->start->z + section_room_size
                        );
                    }

                    // compute hallway-aligned door holes for this room
                    if (!room->doors.empty()) {
                        int door_loy = room->start->y + 1;
                        int door_hiy = door_loy + room->door_height;
                        int dw = room->door_width;
                        int hz_lo = section_start_z + section_room_size;
                        int hz_hi = hz_lo + hallway_size;
                        int hx_lo = section_start_x + section_room_size;
                        int hx_hi = hx_lo + hallway_size;

                        for (int d : room->doors) {
                            // only align with hallway if the wall is on the near side of it
                            bool wall_faces_hallway = false;
                            switch (d) {
                                case 0: wall_faces_hallway = (room->start->z == hz_hi); break;
                                case 1: wall_faces_hallway = (room->start->x == hx_hi); break;
                                case 2: wall_faces_hallway = (room->end->z == hz_lo); break;
                                case 3: wall_faces_hallway = (room->end->x == hx_lo); break;
                            }
                            if (!wall_faces_hallway) continue;

                            int a_min, a_max, h_lo, h_hi;
                            switch (d) {
                                case 0: a_min = room->start->x; a_max = room->end->x; h_lo = hx_lo; h_hi = hx_hi; break;
                                case 1: a_min = room->start->z; a_max = room->end->z; h_lo = hz_lo; h_hi = hz_hi; break;
                                case 2: a_min = room->start->x; a_max = room->end->x; h_lo = hx_lo; h_hi = hx_hi; break;
                                case 3: a_min = room->start->z; a_max = room->end->z; h_lo = hz_lo; h_hi = hz_hi; break;
                            }
                            int lo, hi;
                            if (a_min >= h_hi) {
                                lo = a_min + 1; hi = lo + dw;
                            } else if (a_max <= h_lo) {
                                hi = a_max - 1; lo = hi - dw;
                            } else {
                                lo = max(h_lo, a_min + 1); hi = lo + dw;
                            }
                            // keep door at least 1 block from both corners
                            if (lo < a_min + 1) lo = a_min + 1;
                            if (hi > a_max - 1) hi = a_max - 1;
                            room->holes[d] = {true, lo, hi, door_loy, door_hiy};
                        }
                    }

                    room_positions[position] = room;
                }

                // combine any leftover spaces into rooms if possible
                unordered_set<int> cancelled;
                for (pair<int, shared_ptr<Room>> position_room : empty_rooms) {

                    int pos = position_room.first;

                    if (cancelled.find(pos) != cancelled.end()) continue;
                    /*
                        try to combine if possible.

                        positions; // 0=back-left, 1=back-right, 2=front-left, 3=front-right

                        doors; // 0=front, 1=left, 2=back, 3=right
                    */
                    if (pos == 0) { // back-left

                        // combine with back-right if: no back door, right door exists
                        if (!Util::contains(section->doors, 2) && Util::contains(section->doors, 3)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[1]; // back-right
                            cancelled.insert(1);
                            cancelled.insert(pos);
                            room_to_combine_with->start->x = section_start_x;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                        // combine with front-left if: no left door, and back door exists
                        else if (!Util::contains(section->doors, 1) && Util::contains(section->doors, 2)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[2]; // front-left
                            cancelled.insert(2);
                            cancelled.insert(pos);
                            room_to_combine_with->end->z = section_start_z+section_size;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                    }
                    else if (pos == 1) { // back-right
                        // combine with back-left if: no back door, and right door exists
                        if (!Util::contains(section->doors, 2) && Util::contains(section->doors, 3)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[0]; // back-left
                            cancelled.insert(0);
                            cancelled.insert(pos);
                            room_to_combine_with->end->x = section_start_x+section_size;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                        // combine with front-right if: no right door, and front door exists
                        else if (!Util::contains(section->doors, 3) && Util::contains(section->doors, 0)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[3]; // front-right
                            cancelled.insert(3);
                            cancelled.insert(pos);
                            room_to_combine_with->end->z = section_start_z+section_size;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }

                    }
                    else if (pos == 2) { // front-left
                        // combine with front-right if: no front door, and left door exists
                        if (!Util::contains(section->doors, 0) && Util::contains(section->doors, 1)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[3]; // front-right
                            cancelled.insert(3);
                            cancelled.insert(pos);
                            room_to_combine_with->start->x = section_start_x;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                        // combine with back-left if: no left door, and back door exists
                        else if (!Util::contains(section->doors, 1) && Util::contains(section->doors, 2)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[0]; // back-left
                            cancelled.insert(0);
                            cancelled.insert(pos);
                            room_to_combine_with->start->z = section_start_z;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }

                    }
                    else if (pos == 3) { // front-right
                        // combine with front-left if: no front door, and left door exists
                        if (!Util::contains(section->doors, 0) && Util::contains(section->doors, 1)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[2]; // front-left
                            cancelled.insert(2);
                            cancelled.insert(pos);
                            room_to_combine_with->start->x = section_start_x;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                        // combine with back-right if: no right door, and front door exists
                        else if (!Util::contains(section->doors, 3) && Util::contains(section->doors, 0)) {
                            shared_ptr<Room> room_to_combine_with = room_positions[1]; // back-right
                            cancelled.insert(1);
                            cancelled.insert(pos);
                            room_to_combine_with->start->z = section_start_z;
                            if (empty_rooms.find(pos) != empty_rooms.end()) {
                                end_rooms.push_back(room_to_combine_with);
                                section->rooms.push_back(room_to_combine_with);
                            }
                        }
                    }

                }
            

                // otherwise leave as empty rooms
                for (pair<int, shared_ptr<Room>> position_room : empty_rooms) {

                    int pos = position_room.first;

                    if (cancelled.find(pos) != cancelled.end()) continue;
                
                    shared_ptr<Room> room = position_room.second;
                    end_rooms.push_back(room);
                    section->rooms.push_back(room);
                }

 
                // add hallway floors as rooms
                // doors; // 0=front, 1=left, 2=back, 3=right
                shared_ptr<Room> center_floor = std::make_shared<Room>();
                center_floor->start = std::make_shared<Position>(
                    section_start_x + section_room_size,
                    section_start_y,
                    section_start_z + section_room_size
                );
                center_floor->end = std::make_shared<Position>(
                    section_start_x + section_room_size+base_size,
                    section_start_y,
                    section_start_z + section_room_size+base_size
                );
                end_rooms.push_back(center_floor);
                section->rooms.push_back(center_floor);


                // front hallway
                if (Util::contains(section->doors, 0)) {
                    shared_ptr<Room> front_floor = std::make_shared<Room>();
                    front_floor->start = std::make_shared<Position>(
                        section_start_x + section_room_size,
                        section_start_y,
                        section_start_z
                    );
                    front_floor->end = std::make_shared<Position>(
                        section_start_x + section_room_size+base_size,
                        section_start_y,
                        section_start_z + section_room_size
                    );
                    end_rooms.push_back(front_floor);
                    section->rooms.push_back(front_floor);
                }

                // left hallway
                if (Util::contains(section->doors, 1)) {
                    shared_ptr<Room> left_floor = std::make_shared<Room>();
                    left_floor->start = std::make_shared<Position>(
                        section_start_x,
                        section_start_y,
                        section_start_z + section_room_size
                    );
                    left_floor->end = std::make_shared<Position>(
                        section_start_x + section_room_size,
                        section_start_y,
                        section_start_z + section_room_size+base_size
                    );
                    end_rooms.push_back(left_floor);
                    section->rooms.push_back(left_floor);
                }

                // back hallway
                if (Util::contains(section->doors, 2)) {
                    shared_ptr<Room> back_floor = std::make_shared<Room>();
                    back_floor->start = std::make_shared<Position>(
                        section_start_x + section_room_size,
                        section_start_y,
                        section_start_z + section_room_size+base_size
                    );
                    back_floor->end = std::make_shared<Position>(
                        section_start_x + section_room_size+base_size,
                        section_start_y,
                        section_start_z + section_size
                    );
                    end_rooms.push_back(back_floor);
                    section->rooms.push_back(back_floor);
                }

                // right hallway
                if (Util::contains(section->doors, 3)) {
                    shared_ptr<Room> right_floor = std::make_shared<Room>();
                    right_floor->start = std::make_shared<Position>(
                        section_start_x + section_room_size+base_size,
                        section_start_y,
                        section_start_z + section_room_size
                    );
                    right_floor->end = std::make_shared<Position>(
                        section_start_x + section_size,
                        section_start_y,
                        section_start_z + section_room_size+base_size
                    );
                    end_rooms.push_back(right_floor);
                    section->rooms.push_back(right_floor);
                }

            }
            else {
                if (!section->rooms.empty()) {
                    section->rooms[0]->start = std::make_shared<Position>(section_start_x, section_start_y, section_start_z);
                    int open_room_h = room_size * section->vertical_scale * OPEN_ROOM_VERTICAL_SCALE_MULT;
                    section->rooms[0]->end = std::make_shared<Position>(
                        section_start_x + section_size, 
                        section_start_y + open_room_h, 
                        section_start_z + section_size
                    );
                    shared_ptr<Room> open_room = section->rooms[0];
                    open_room->door_width = max(1, (int)(base_size * DOOR_WIDTH));
                    open_room->door_height = max(1, min((int)(largest_occupent * 1.2), open_room_h - 1));
                    open_room->doors = section->doors;

                    // compute centered door holes (no hallway context)
                    if (!open_room->doors.empty()) {
                        int cx = (open_room->start->x + open_room->end->x) / 2;
                        int cz = (open_room->start->z + open_room->end->z) / 2;
                        int hw = open_room->door_width / 2;
                        int door_loy = open_room->start->y + 1;
                        int door_hiy = door_loy + open_room->door_height;
                        for (int d : open_room->doors) {
                            switch (d) {
                                case 0:
                                    open_room->holes[d] = {true, cx - hw, cx + hw, door_loy, door_hiy}; break;
                                case 1:
                                    open_room->holes[d] = {true, cz - hw, cz + hw, door_loy, door_hiy}; break;
                                case 2:
                                    open_room->holes[d] = {true, cx - hw, cx + hw, door_loy, door_hiy}; break;
                                case 3:
                                    open_room->holes[d] = {true, cz - hw, cz + hw, door_loy, door_hiy}; break;
                            }
                        }
                    }
                }
            }

            
        }

        return grid;
    }



    static void add_to_grid(
        int building_width,
        int building_depth,
        Grid<shared_ptr<Section>>& grid,
        shared_ptr<Section> section
    ) {
        
        // add to grid
        for (int x = section->start->x; x < section->end->x; ++x) {
            for (int y = section->start->y; y < section->end->y; ++y) {
                for (int z = section->start->z; z < section->end->z; ++z) {
                    grid[{x,y,z}] = section;
                }
            }
        }
    }


    static vector<int> get_possibles_scales(int building_size_in_rooms) {
        vector<int> scales;
        int scale = 1;
        while(scale <= building_size_in_rooms) {
            scales.push_back(scale);
            scale+=2;
        }
        return scales;
    }

    static vector<int> option_door(shared_ptr<Section> section) {
        vector<int> possible_doors = {0, 1, 2, 3};
        for (int existing_door : section->doors) {
            possible_doors.erase(
                remove(possible_doors.begin(), possible_doors.end(), existing_door),    
                possible_doors.end()
            );
        }
        return possible_doors;
    }

    static int opposite_door(int door) {
        // 0=front, 1=left, 2=back, 3=right
        if (door == 0) {
            return 2;
        }
        else if (door == 1) {
            return 3;
        }
        else if (door == 2) {
            return 0;
        }
        else if (door == 3) {
            return 1;
        }
        return -1;
    }

    static shared_ptr<Position> section_position(
        shared_ptr<Section> section,
        shared_ptr<Section> other_section,
        int other_section_door
    ) {
        shared_ptr<Position> pos = std::make_shared<Position>(0, 0, 0);


        int half_scale = section->scale / 2;
        int other_half_scale = other_section->scale / 2;

        if (other_section_door == 0) { // front
            pos->x = other_section->start->x - half_scale + other_half_scale;
            pos->z = other_section->start->z - other_section->scale;
        }
        else if (other_section_door == 1) { // left
            pos->x = other_section->start->x - other_section->scale;
            pos->z = other_section->start->z - half_scale + other_half_scale;
        }
        else if (other_section_door == 2) { // back
            pos->x = other_section->start->x - half_scale + other_half_scale;
            pos->z = other_section->start->z + other_section->scale;
        }
        else if (other_section_door == 3) { // right
            pos->x = other_section->start->x + other_section->scale;
            pos->z = other_section->start->z - half_scale + other_half_scale;
        }

        return pos;
    }


    struct Corner {
        shared_ptr<Position> pos;
        shared_ptr<Room> room;
    };

    static vector<shared_ptr<Corner>> get_bottom_corners(shared_ptr<Room> room) {
        shared_ptr<Corner> corner1 = std::make_shared<Corner>();
        shared_ptr<Corner> corner2 = std::make_shared<Corner>();
        shared_ptr<Corner> corner3 = std::make_shared<Corner>();
        shared_ptr<Corner> corner4 = std::make_shared<Corner>();

        corner1->pos = std::make_shared<Position>(room->start->x, room->start->y, room->start->z);
        corner2->pos =  std::make_shared<Position>(room->end->x, room->start->y, room->start->z);
        corner3->pos =  std::make_shared<Position>(room->start->x, room->start->y, room->end->z);
        corner4->pos =  std::make_shared<Position>(room->end->x, room->start->y, room->end->z); 

        corner1->room = room;
        corner2->room = room;
        corner3->room = room;
        corner4->room = room;

        return {corner1, corner2, corner3, corner4};
    }


    static bool spot_valid(
        Grid<shared_ptr<Section>>& grid,
        int room_height,
        int room_width,
        int room_depth,
        int building_height,
        int building_width,
        int building_depth,
        int start_x, 
        int start_y,
        int start_z
    ) {

        // check bounds
        if (!grid.in_bounds({start_x, start_y, start_z})) {
            return false;
        }


        // check for overlaps
        for (int x = start_x; x < start_x+room_width; ++x) {
            for (int y = start_y; y < start_y+room_height; ++y) {
                for (int z = start_z; z < start_z+room_depth; ++z) {
                    if (!grid.in_bounds({x,y,z})) {
                        return false;
                    }
                    shared_ptr<Section> existing_section = grid[{x,y,z}];
                    if (existing_section != nullptr) {
                        return false;
                    }

                }
            }
        }

        return true;
    }

    // OLD


    // TEST

    static void render_block_grid(Grid<shared_ptr<Block>> blocks) {
        IsometricRenderer::render_s(blocks.values(), "src/TEST/output/RENDER.png", IsoAngle::NE, 16);
    }

};