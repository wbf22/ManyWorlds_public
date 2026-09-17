#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <limits>
#include <unistd.h>

#include "../blocks/Block.h"
#include "../world/World.hpp"
#include "../world/maps/IsometricRenderer.hpp"
#include "util/BMaterial.hpp"
#include "util/PositionDouble.h"
#include "npcs/AlienGen.hpp"
#include "../buildings_and_cities/BuildingGen.hpp"
#include "util/Grapher.hpp"
#include "../world/World.hpp"
#include "../world/maps/Maps.hpp"
#include "space/SpaceGen.hpp"
#include "space/SkyGen.hpp"
#include "../server/Server.hpp"
#include "util/StellarCoordinate.hpp"
#include "util/Grid.hpp"
#ifndef MANYWORLDS_API
#define MANYWORLDS_API
#endif
#include "../server/NetworkClient.hpp"
#include "../server/Settings.hpp"
#include <filesystem>
#include <thread>
#include "../economies/Economies.hpp"
#include "vehicles/VehicleGen.hpp"
#include "../plants/PlantGen.hpp"

using namespace std;

// util

template<typename Expected, typename Actual>
inline void assert_eq_fail(
    const char* expected_expr,
    const char* actual_expr,
    const Expected& expected,
    const Actual& actual,
    const char* file,
    int line,
    const char* func,
    const string& msg = "")
{
    ostringstream oss;

    oss << "\nASSERTION FAILED\n"
        << "Expected   : " << expected_expr
        << " = " << expected << '\n'
        << "Actual     : " << actual_expr
        << " = " << actual << '\n'
        << "File       : " << file << '\n'
        << "Line       : " << line << '\n'
        << "Function   : " << func << '\n';

    if (!msg.empty())
        oss << "Message    : " << msg << '\n';

    cerr << oss.str() << endl;

    throw runtime_error(oss.str());
}


#define ASSERT(expected, actual)                                    \
    (((expected) == (actual)) ? (void)0 : assert_eq_fail(              \
        #expected,                                                      \
        #actual,                                                        \
        (expected),                                                     \
        (actual),                                                       \
        __FILE__,                                                       \
        __LINE__,                                                       \
        __func__))

void print_pid() {

    // print pid for looking on memory usage in system monitor
    pid_t pid = getpid();
    cout << "PID: " << pid << endl;
}

shared_ptr<Block> make_block(shared_ptr<PositionDouble> pos, string texture) {
    shared_ptr<Block> b1 = make_shared<Block>();
    b1->size = 1;
    b1->material = texture;
    b1->position_double = pos;

    return b1;
}

void add_origin_reference(vector<shared_ptr<Block>>& blocks, int cross_length) {

    for (int x = -cross_length; x < cross_length; x++) {
        blocks.push_back(
            make_block(make_shared<PositionDouble>(x, 0, 0), BMaterial::ICE)
        );
    }
    for (int y = -cross_length; y < cross_length; y++) {
        blocks.push_back(
            make_block(make_shared<PositionDouble>(0, y, 0), BMaterial::ICE)
        );
    }
    for (int z = -cross_length; z < cross_length; z++) {
        blocks.push_back(
            make_block(make_shared<PositionDouble>(0, 0, z), BMaterial::ICE)
        );
    }
}

shared_ptr<Planet> make_planet() {
    shared_ptr<Planet> planet = std::make_shared<Planet>(56635234455L, std::make_shared<StellarCoordinate>());
    planet->atmosphericPressure = 2;
    return planet;
}


// tests
void grid_test() {
    // normal inbounds
    Grid<int> grid(2, 2, 2);
    Index3 a{0, 0, 0};
    Index3 b{1, 1, 1};
    Index3 c{-1, 0, 0};
    Index3 d{2, 2, 2};
    grid[a] = 1;
    grid[b] = 2;
    grid[c] = 3;
    grid[d] = 4;

    ASSERT(grid[a], 1);
    ASSERT(grid[b], 2);
    ASSERT(grid[c], 3);
    ASSERT(grid[d], 4);

    // out of bounds
    Grid<bool> bool_grid(1, 1, 1);
    Index3 e{0, 0, 0};
    Index3 f{3, 0, 0};
    bool_grid[e] = true;
    bool_grid[f] = true;

    ASSERT(bool_grid[e], true);
    ASSERT(bool_grid[f], true);

    // hard exercise with multiple resizes
    Grid<int> bool_grid_2(1, 1, 1);
    for (int i = 0; i < 1000; i++) {
        int x = Random::randInt(i, -i, i);
        int y = Random::randInt(i+1, -i, i);
        int z = Random::randInt(i+2, -i, i);

        if (bool_grid_2[{x,y,z}] == 0) {
            bool_grid_2[{x,y,z}] = i;
        }
        else {
            cout << "collision " << i << " with " << bool_grid_2[{x,y,z}] << endl;
        }
    }
    int x,y,z,i;
    Index3 ind;
    i = 0;
    x = Random::randInt(i, -i, i);
    y = Random::randInt(i+1, -i, i);
    z = Random::randInt(i+2, -i, i);
    ASSERT((bool_grid_2[{x,y,z}]), i);
    i = 25;
    x = Random::randInt(i, -i, i);
    y = Random::randInt(i+1, -i, i);
    z = Random::randInt(i+2, -i, i);
    ASSERT((bool_grid_2[{x,y,z}]), i);
    i = 50;
    x = Random::randInt(i, -i, i);
    y = Random::randInt(i+1, -i, i);
    z = Random::randInt(i+2, -i, i);
    ASSERT((bool_grid_2[{x,y,z}]), i);
    i = 500;
    x = Random::randInt(i, -i, i);
    y = Random::randInt(i+1, -i, i);
    z = Random::randInt(i+2, -i, i);
    ASSERT((bool_grid_2[{x,y,z}]), i);
    i = 999;
    x = Random::randInt(i, -i, i);
    y = Random::randInt(i+1, -i, i);
    z = Random::randInt(i+2, -i, i);
    ASSERT((bool_grid_2[{x,y,z}]), i);
}

void proximity_grid_test() {
    ProximityGrid<shared_ptr<int>> grid({8, 4, 1, 4, 0});
    auto add = [&](Index3 position, int64_t id) {
        auto value = make_shared<int>((int)id);
        grid.add(position, id, value);
    };

    add({0, 0, 0}, 1);
    add({2, 0, 0}, 2);
    add({1, 1, 0}, 3);
    add({3, 0, 0}, 4);
    add({-3, 0, 0}, 5);

    ASSERT(grid.levels.size(), (size_t)3);
    ASSERT(grid.levels[0], 1);
    ASSERT(grid.size(), (size_t)5);
    ASSERT(grid.values().size(), (size_t)5);
    ASSERT(grid.has({-3, 0, 0}), true);
    ASSERT(grid.has({-1, 0, 0}), false);

    auto nearby = grid.query({0, 0, 0}, 2);
    bool found_one = false, found_two = false, found_three = false;
    bool found_four = false, found_five = false;
    for (const auto& object : nearby) {
        found_one |= *object->obj == 1;
        found_two |= *object->obj == 2;
        found_three |= *object->obj == 3;
        found_four |= *object->obj == 4;
        found_five |= *object->obj == 5;
    }
    ASSERT(nearby.size(), (size_t)3);
    ASSERT(found_one, true);
    ASSERT(found_two, true);
    ASSERT(found_three, true);
    ASSERT(found_four, false);
    ASSERT(found_five, false);

    grid.update({3, 0, 0}, {1, 0, 0}, 4);
    ASSERT(grid.has({3, 0, 0}), false);
    ASSERT(grid.has({1, 0, 0}), true);
    ASSERT(grid.size(), (size_t)5);

    add({9, 0, 0}, 2);
    ASSERT(grid.size(), (size_t)5);
    ASSERT(grid.has({2, 0, 0}), false);
    ASSERT(grid.has({9, 0, 0}), true);

    grid.clear();
    ASSERT(grid.size(), (size_t)0);
    ASSERT(grid.query({0, 0, 0}, 10).empty(), true);
}

void renderer_test() {
    unordered_map<string, uint8_t*> textures;
    int width, height, n;
    string path = BMaterial::path(BMaterial::ICE);
    textures[BMaterial::ICE] = stbi_load(path.c_str(), &width, &height, &n, 4);
    path = BMaterial::path(BMaterial::DIRT);
    textures[BMaterial::DIRT] = stbi_load(path.c_str(), &width, &height, &n, 4);

    vector<shared_ptr<Block>> blocks;
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 0.0, 0.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 1.0, 0.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 3.0, 0.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(1.0, 0.0, 0.0), BMaterial::DIRT)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 0.0, -1.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 0.0, -5.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 0.0, 5.0), BMaterial::ICE)
    );
    blocks.push_back(
        make_block(make_shared<PositionDouble>(0.0, 8.0, 5.0), BMaterial::ICE)
    );

    
    int image_size;
    double scale = 128.0;
    uint8_t* rendered_image = IsometricRenderer::render(
        blocks,
        textures,
        IsoAngle::NE,
        image_size,
        scale
    );
    cout << image_size << endl;

    stbi_write_png("src/TEST/output/RENDER.png", image_size, image_size, 4, rendered_image, 0);

}

void contour_test() {
    unordered_map<string, uint8_t*> textures;
    int width, height, n;
    string path = BMaterial::path(BMaterial::ICE);
    textures[BMaterial::ICE] = stbi_load(path.c_str(), &width, &height, &n, 4);
    path = BMaterial::path(BMaterial::DIRT);
    textures[BMaterial::DIRT] = stbi_load(path.c_str(), &width, &height, &n, 4);

    for (int a = 0; a <= 360; a+=15) {
        // make part
        shared_ptr<BodyPart> part = make_shared<BodyPart>();
        part->length = 15;
        vector<PartDim> dims = {
            {0, 3, -4, 0, 0.7},
            {part->length * 0.5, 4, -4, 0, 0.8},
            {part->length, 6, -4, 0, 1.0},
        };
        AlienGen::interpolate_part_contour(
            part, 
            dims,
            1,
            a,
            "z"
        );                    
        
        // make reference
        add_origin_reference(part->blocks, 12);
        
        IsometricRenderer::render_and_save_png(part->blocks, textures, IsoAngle::NE, "src/TEST/output/NE.png", 16);
        cout << a << endl;
    }
}

void alien_gen_test() {

    // contour_test();

    unordered_map<string, uint8_t*> textures;
    int width, height, n;
    string path = BMaterial::path(BMaterial::ICE);
    textures[BMaterial::ICE] = stbi_load(path.c_str(), &width, &height, &n, 4);
    path = BMaterial::path(BMaterial::DIRT);
    textures[BMaterial::DIRT] = stbi_load(path.c_str(), &width, &height, &n, 4);


    shared_ptr<Planet> home_world = make_planet();
    home_world->evolutionStage = 5;
    shared_ptr<Alien> alien = AlienGen::gen_alien(home_world, 0);
    vector<shared_ptr<Block>> blocks;
    for (shared_ptr<BodyPart> part : alien->body->parts) {
        for (shared_ptr<Block> block : part->blocks) {
            shared_ptr<Block> position_adjusted_block = make_shared<Block>();
            position_adjusted_block->position_double = part->start_pos->add(block->position_double);
            position_adjusted_block->size = block->size;
            position_adjusted_block->material = block->material;

            blocks.push_back(position_adjusted_block);
        }
    }

    
    int image_size;
    double scale = 8.0;
    uint8_t* rendered_image = IsometricRenderer::render(
        blocks,
        textures,
        IsoAngle::NE,
        image_size,
        scale
    );
    cout << image_size << endl;

    stbi_write_png("src/TEST/output/RENDER.png", image_size, image_size, 4, rendered_image, 0);

}

void style_gen_test() {

    // textures
    unordered_map<string, uint8_t*> textures;
    int width, height, n;
    string path = BMaterial::path(BMaterial::ANDESTITE);
    textures[BMaterial::ANDESTITE] = stbi_load(path.c_str(), &width, &height, &n, 4);

    // make two adjacent rooms with all exterior sides
    shared_ptr<Room> room_a = make_shared<Room>();
    room_a->start = make_shared<Position>(0, 0, 0);
    room_a->end = make_shared<Position>(15, 10, 15);
    room_a->exterior_sides = {0, 1, 2, 3};

    shared_ptr<Room> room_b = make_shared<Room>();
    room_b->start = make_shared<Position>(16, 0, 0);
    room_b->end = make_shared<Position>(25, 10, 15);
    room_b->exterior_sides = {0, 1, 2, 3};

    // make roof style with specific parameters for predictable output
    shared_ptr<RoofStyleElement> roof = make_shared<RoofStyleElement>();
    roof->type = StyleElementType::ROOF;
    roof->pitch = 50;
    roof->curvature = 50;
    roof->overhang = 2;
    roof->layers = 1;
    Grid<bool> profile(2, 1, 2);
    profile[{0, 0, 0}] = true;
    profile[{1, 0, 0}] = true;
    profile[{0, 0, 1}] = true;
    profile[{1, 0, 1}] = true;
    roof->surface_texture_profile = profile;

    Grid<shared_ptr<Block>> grid_blocks;
    room_a->get_blocks(grid_blocks, "");
    // room_b->get_blocks(grid_blocks, "");

    auto test_section = make_shared<Section>();
    test_section->rooms = {room_a};
    test_section->start = make_shared<Position>(0, 0, 0);
    test_section->end   = make_shared<Position>(1, 1, 1);
    test_section->scale = 1;
    test_section->vertical_scale = 1;

    Grid<shared_ptr<Section>> test_sg(1, 1, 1);
    test_sg[{0, 0, 0}] = test_section;
    StyleGen::apply_roof_style(roof, test_sg, grid_blocks, 1234);

    // column
    shared_ptr<ColumnStyleElement> column = make_shared<ColumnStyleElement>();
    column->round = true;
    column->frame = nullptr;
    column->lines = false;
    shared_ptr<ColumnStyleElement> top_column_style_element = make_shared<ColumnStyleElement>();
    top_column_style_element->round = false;
    top_column_style_element->frame = nullptr;
    top_column_style_element->lines = false;
    column->top_column_style_element = top_column_style_element;
    shared_ptr<ColumnStyleElement> bottom_column_style_element = make_shared<ColumnStyleElement>();
    bottom_column_style_element->round = false;
    bottom_column_style_element->frame = nullptr;
    bottom_column_style_element->lines = false;
    column->bottom_column_style_element = bottom_column_style_element;
    Grid<shared_ptr<Block>> column_blocks = StyleGen::make_column(column, 7, 40);
    
    
    // frame
    shared_ptr<FrameStyleElement> frame = make_shared<FrameStyleElement>();
    frame->cross_section_grid = Grid<bool>(2,2,2);
    frame->cross_section_grid[{0, 0, 0}] = true;
    frame->cross_section_grid[{1, 0, 0}] = true;
    frame->cross_section_grid[{0, 0, 1}] = true;
    frame->cross_section_grid[{0, 1, 0}] = true;
    frame->cross_section_grid[{1, 1, 0}] = true;
    frame->sides = {0,1,2,3};
    StyleGen::apply_frame_style(
        frame,
        room_a->start,
        make_shared<Position>(room_a->end->x, room_a->end->y, room_a->end->z),
        1,
        grid_blocks
    );


    // wall
    shared_ptr<WallDesignElement> design = StyleGen::gen_wall_design_style(1234L, 15);
    shared_ptr<WallStyleElement> wall = make_shared<WallStyleElement>();
    wall->wall_design = design;
    wall->type = StyleElementType::WALL;
    wall->thickness = 3;
    wall->outward_slope = 0.5;
    wall->inward_slope = 0.25;

    auto start = make_shared<Position>(60, 0, 0);
    Grid<shared_ptr<Block>> wall_blocks = StyleGen::make_wall(WallAngle::W, wall, start, 15, 10);



    // jut out
    shared_ptr<JutOutStyleElement> jut_out = make_shared<JutOutStyleElement>();
    jut_out->bottom_support = nullptr;
    jut_out->bottom_support_height = 5;
    jut_out->smoothed_bottom_support = true;
    jut_out->roof = roof;

    shared_ptr<ColumnStyleElement> jcolumn = make_shared<ColumnStyleElement>();
    jcolumn->round = true;
    jcolumn->frame = nullptr;
    jcolumn->lines = false;
    jut_out->column = jcolumn;

    shared_ptr<WallStyleElement> jwall = make_shared<WallStyleElement>();
    jwall->type = StyleElementType::WALL;
    jwall->thickness = 1;
    jwall->outward_slope = 0;
    jwall->inward_slope = 0;
    jut_out->railing_or_wall = jwall;

    auto jut_out_section = make_shared<Section>();
    jut_out_section->start = room_a->start;
    jut_out_section->end = room_a->end;
    jut_out_section->rooms = {room_a};

    Grid<shared_ptr<Block>> jut_out_blocks = StyleGen::make_jut_out(jut_out, jut_out_section, false, 2, 10, 8, 4);

    // render
    vector<shared_ptr<Block>> blocks = grid_blocks.values();
    for (shared_ptr<Block> block : column_blocks.values()) {
        block->position->x += room_b->end->x;
        blocks.push_back(block);
    }
    for (shared_ptr<Block> block : wall_blocks.values()) {
        blocks.push_back(block);
    }
    for (shared_ptr<Block> block : jut_out_blocks.values()) {
        blocks.push_back(block);
    }
    // auto t_blocks = jut_out_blocks.values();
    IsometricRenderer::set_position_double_on_blocks(blocks);
    IsometricRenderer::render_and_save_png(blocks, textures, IsoAngle::NE, "src/TEST/output/RENDER.png", 32);
    cout << "Roof test: " << blocks.size() << " blocks rendered" << endl;
}

void random_test() {



    // across range
    vector<pair<int64_t, int64_t>> values;
    for (int i = -1000; i < 1000; i++) {
        values.push_back(pair<int64_t, int64_t>(i, Random::randInt(i, -1000, 1000)));
    }
    Grapher::graph_points(values, "src/TEST/output/across_range.bmp");

    // with zero seed
    values.clear();
    for (int i = 0; i < 1000; i++) {
        values.push_back(pair<int64_t, int64_t>(i, Random::randInt(0, 0, i)));
    }
    Grapher::graph_points(values, "src/TEST/output/rand_seed_zero_test.bmp");


    // using min and max int64_t as min and max
    bool has_one_outside_half_range = false;
    values.clear();
    for (int i = -1000; i < 1000; i++) {
        int64_t rand = Random::randInt(i, Util::MIN_INT64_T, Util::MAX_INT64_T);
        values.push_back(pair<int64_t, int64_t>(i, rand));
        if (abs(rand) > Util::MAX_INT64_T/2) has_one_outside_half_range = true;   
    }
    Grapher::graph_points(values, "src/TEST/output/max_range_test.bmp");
    ASSERT(true, has_one_outside_half_range);


    // assert respects min and max
    int64_t quarter = Util::MAX_INT64_T/4;
    int64_t min = Util::MIN_INT64_T+quarter;
    int64_t max = Util::MAX_INT64_T-quarter;
    for (int i = -1000; i < 1000; i++) {
        int64_t rand = Random::randInt(i, min, max);
        ASSERT(true, rand > min && rand < max);
    }

    int64_t seed = 56635234455;
    unordered_map<int, int> counts;
    int range = 12;
    int iterations = 100;
    for (int i = 0; i < range; i++) {
        counts[i] = 0;
    }
    for (int i = 0; i < iterations; i++) {
        int64_t rand = Random::randInt(seed+i, 0, range);
        counts[rand] = counts[rand] + 1;
    }
    for (int i = 0; i < range; i++) {
        // cout << i << ": " << counts[i] << endl;
        ASSERT(true, counts[i] > 0);
    }

    /*
    9f106f1
    fd0295c
    dd0384e
    */


    // average test
    values.clear();
    double sum = 0;
    for (int i = 0; i < 1000; i++) {
        int64_t value = Random::randInt(i, 0, 10, 10000);
        values.push_back(pair<int64_t, int64_t>(i, value));
        sum += value;
    }
    int64_t avg = sum / 1000;
    cout << avg << endl;
    Grapher::graph_points(values, "src/TEST/output/rand_avg_test.bmp");


}

void map_test() {

    shared_ptr<World> world = make_shared<World>();
    shared_ptr<Planet> planet = make_planet();
    
    int64_t root_square = 40;
    planet->hasLife = true;
    planet->worldSize = root_square * Chunk::CHUNK_SIZES[ChunkType::CHUNK_2097152];
    planet->radius = planet->worldSize / (2*M_PI);
    world->rootChunk = make_shared<RootChunk>(root_square, 0, planet);
    world->rootChunk->rootChunk = world->rootChunk;
    planet->rootChunk = world->rootChunk;
    world->rootChunk->init(planet);
    world->init(planet);
    world->genType = GenType::TERRAIN;

    world->start();

    this_thread::sleep_for(chrono::minutes(100));

    
}

void stellar_coordinate_test() {
    int64_t test_seeds[] = {0, 1, -1, 1000000, -1000000, 2642245, -2642245,
                            999999999999LL, -999999999999LL, 9223362092153785818LL};
    for (int64_t s : test_seeds) {
        auto sc = StellarCoordinate::from_seed_old(s);
        int64_t rt = StellarCoordinate::seed_from_coordinates_old(sc);
        ASSERT(s, rt);
    }
    cout << "  seed round-trip OK" << endl;
}

void build_gen_test() {

    shared_ptr<Planet> planet = make_planet();


    shared_ptr<City> city = make_shared<City>();
    city->largest_citizens = 4;
    BuildingGen::gen_city_style_groups(
        planet->seed,
        1234L,
        city
    );

    double time = Util::time();
    vector<shared_ptr<Block>> blocks = BuildingGen::gen_building(
        planet->seed,
        1234L,
        BuildingType::HOME,
        50,
        50,
        80,
        planet,
        city
    );
    cout << Util::time() - time << "s" << endl;

    // render
    IsometricRenderer::render_s(blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 16);
    cout << "Build Gen Test: " << blocks.size() << " blocks rendered" << endl;


}

void voxel_planet_test() {

    Timer::start();
    cout << "voxel_planet_test: start" << endl;

    Timer::start();

    shared_ptr<StellarCoordinate> view_pos = make_shared<StellarCoordinate>();
    view_pos->quadrant_x = 207641;
    view_pos->quadrant_y = 306468;
    view_pos->quadrant_z = -404013;
    view_pos->light_year_x = 0;
    view_pos->light_year_y = 0;
    view_pos->light_year_z = 0;
    view_pos->km_x = 100000;
    view_pos->km_y = 0;
    view_pos->km_z = 0;

    shared_ptr<Planet> planet = make_planet();
    planet->hasLife = true;
    int64_t root_square = 40;
    planet->worldSize = root_square * Chunk::CHUNK_SIZES[ChunkType::CHUNK_2097152];
    planet->radius = planet->worldSize / (2*M_PI);
    planet->seaLevel = -10000;
    root_square = (int64_t)ceil(planet->worldSize / (double)Chunk::CHUNK_SIZES[Chunk::topTypeFor(planet->worldSize)]);
    if (root_square < 1) root_square = 1;

    shared_ptr<StellarCoordinate> planet_pos = make_shared<StellarCoordinate>();
    planet_pos->quadrant_x = 207641;
    planet_pos->quadrant_y = 306468;
    planet_pos->quadrant_z = -404013;
    planet_pos->light_year_x = 0;
    planet_pos->light_year_y = 0;
    planet_pos->light_year_z = 0;
    planet_pos->km_x = 0;
    planet_pos->km_y = 0;
    planet_pos->km_z = 0;

    cout << "Creating World..." << endl;
    shared_ptr<World> world = make_shared<World>();
    cout << "Creating RootChunk..." << endl;
    world->rootChunk = make_shared<RootChunk>(root_square, 0, planet);
    cout << "Setting up root chunk pointers..." << endl;
    world->rootChunk->rootChunk = world->rootChunk;
    planet->rootChunk = world->rootChunk;
    cout << "Calling rootChunk->init..." << endl;
    world->rootChunk->init(planet);
    cout << "Calling world->init..." << endl;
    world->init(planet);
    // shell renders in the stellar LY frame; pick a scale so the test planet (~13.4M m
    // radius → ~1.4e-9 LY) spans a visible multi-block shell like the old meters/1e6 grid
    world->game_scale = 1e10;

    cout << "Calling setup_skybox_planet_view..." << endl;
    auto [viewer_vec3, center_vec3, real_dist_m] = world->setup_skybox_planet_view(view_pos, planet_pos, planet);
    cout << "Done setup_skybox_planet_view" << endl;

    cout << "Generating voxel planet..." << endl;
    vector<shared_ptr<Block>> blocks = world->generate_voxel_planet(500000);
    cout << "Generated " << blocks.size() << " voxel blocks" << endl;

    IsometricRenderer::render_s(blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 16);

    float* image = new float[SkyGen::TOTAL_FLOATS]();
    SkyGen::generate_sky_box(image, blocks, 0, 0, 200);
    stbi_write_hdr("src/TEST/output/SKY.hdr", SkyGen::IMAGE_WIDTH, SkyGen::IMAGE_HEIGHT, SkyGen::FLOATS_PER_PIXEL, image);

    cout << "voxel_planet_test: done" << endl;
    Timer::stop("plent gen time");
}

void vehicle_gen_test() {
    Timer::start();
    shared_ptr<VehiclePart> vehicle = VehicleGen::gen_vehicle(
        12345L,
        VehiclePurpose::PERSONAL_TRANSIT,
        {VehicleTravelType::TERRAIN},
        4,
        10,
        1,
        .01,
        .01
    );
    Timer::stop("vehicle gen time");

    vector<shared_ptr<Block>> blocks = vehicle->get_all_blocks();
    IsometricRenderer::render_s(blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 32);
}

void plant_gen_test() {
    Timer::start();

    shared_ptr<Planet> planet = make_planet();
    planet->hasLife = true;
    planet->evolutionStage = 5;
    planet->worldSize = 40 * Chunk::CHUNK_SIZES[ChunkType::CHUNK_2097152];
    planet->wood_materials = {
        WoodMaterialType::WOOD,
        // WoodMaterialType::FIBERS, 
        // WoodMaterialType::SPONGE, 
        // WoodMaterialType::VEIN_STONE, 
        // WoodMaterialType::RESIN
    };
    planet->seed = CryptoRandom::rand(0, Util::MAX_INT64_T);
    Util::print("seed", planet->seed);

    shared_ptr<Plant> plant = make_shared<Plant>(0, PlantType::FOREST, planet);
    shared_ptr<Plant> translated_plant = plant->translate(make_shared<Position>(0, 0, 0));
    vector<shared_ptr<Block>> blocks = *translated_plant->blocks;
    Timer::stop("plant gen time");

    IsometricRenderer::render_s(blocks, "src/TEST/output/PLANT.png", IsoAngle::NE, 32);
    cout << "plant_gen_test: " << blocks.size() << " blocks rendered" << endl;
}

void voxel_galaxy_test() {

    shared_ptr<Universe> universe = SpaceGen::generate_universe();

    shared_ptr<StellarCoordinate> galaxy_pos = make_shared<StellarCoordinate>(207641, 306468, -404013, 0, 0, 0, 0, 0, 0 );
    auto galaxy = make_shared<Galaxy>();
    galaxy->location = galaxy_pos;
    SpaceGen::generate_galaxy_properties(galaxy, galaxy_pos);

    shared_ptr<StellarCoordinate> view_pos = make_shared<StellarCoordinate>(207651, 306468, -704013, 0, 0, 0, 0, 0, 0 );
    vector<shared_ptr<Block>> blocks;
    SpaceGen::generate_galaxy_blocks(galaxy, view_pos, 1'000'000'000, blocks);

    // Blocks are now in LY space — use skybox renderer (handles LY positions)
    float* image = new float[SkyGen::TOTAL_FLOATS]();
    SkyGen::generate_sky_box(image, blocks, 0, 0, 200);
    stbi_write_hdr("src/TEST/output/SKY.hdr", SkyGen::IMAGE_WIDTH, SkyGen::IMAGE_HEIGHT, SkyGen::FLOATS_PER_PIXEL, image);
}

void space_sky_test() {
    
    // inside galaxy
    shared_ptr<StellarCoordinate> player_location = std::make_shared<StellarCoordinate>(
        10966621603165340, 
        -89418746164600166, 
        62932456601437093, 
        0, 
        0, 
        0, 
        0, 
        0,
        0
    );

    // // above galaxy
    // shared_ptr<StellarCoordinate> player_location = std::make_shared<StellarCoordinate>(
    //     10966621603165360, // 20 quadrants out
    //     -89418746164600166,
    //     62932456601437093, 
    //     0, 
    //     0, 
    //     0, 
    //     0, 
    //     0, 
    //     0
    // );

    
    shared_ptr<SpaceGen::SpaceGenResult> result = SpaceGen::generate_space(player_location, true);

    stbi_write_hdr("src/TEST/output/SKY.hdr", SkyGen::IMAGE_WIDTH, SkyGen::IMAGE_HEIGHT, SkyGen::FLOATS_PER_PIXEL, result->sky_box);
}

void server_binary_vs_readable_storage() {

    /*

        FINDINGS (10 million blocks):
            file size:
            
            2026-07-15 11:19:15     260.00 MB     block_data_binary.m
            2026-07-15 11:19:15     350.00 MB     block_data_readable.m
            2026-07-15 11:22:06     630.47 KB     binary.zip
            2026-07-15 11:22:14     1.02   MB     readable.zip

            read/write speed:

            Binary: 0.407526s
            Readable: 8.06458s
        
        FINDINGS (1 million blocks):
            file size:

            2026-07-20 11:19:00     26.00  MB     block_data_binary.m
            2026-07-20 11:19:01     35.00  MB     block_data_readable.m
            2026-07-20 11:20:35     63.25  KB     binary.zip
            2026-07-20 11:20:46     102.05 KB     readable.zip

            read/write speed:

            Binary: 0.046931s
            Readable: 0.832649s


        So file size is comparable, but read/write speed is not.
        A typical 64 chunk could have 0-40 million + block edits. So there could be an insane
        amount of block changes to load. However that would be about 40 mb of data to transfer
        once compressed so that's probably not too bad actually

        We'll probably also want to store digging as just ranges that have been dug. That way a huge hole 
        is a just one value instead of like 10,000 blocks or something. Then normal blocks will still
        be sent back as normal blocks

        We'll probably want the server to maintain some in memory data around where players are. And then fallback to
        files otherwise. 


    */

    #pragma pack(push, 1)
    struct OutBlock {
        int64_t x, y, z;   // 8 bytes each = 24 bytes
        uint8_t size;       // 1 byte
        uint8_t material;   // 1 byte
    };
    #pragma pack(pop)

    OutBlock t = {122,125143,83886080,1,1};

    // int iterations = 10'000'000;
    int iterations = 128*128*128;


    // BINARY 
    Timer::start();
    string binary_file = "src/TEST/block_data_binary.m"; 
    filesystem::remove(binary_file);
    
    // write
    {
        ofstream out_b(binary_file, ios::binary);
        for (int i = 0; i < iterations; ++i) {
            out_b.write(reinterpret_cast<const char*>(&t), sizeof(OutBlock));
        }
    }

    // read
    ifstream in_b(binary_file, ios::binary);
    if (!in_b) throw runtime_error("Could not open " + binary_file);

    in_b.seekg(0, ios::end);
    streamsize fileSize = in_b.tellg();
    in_b.seekg(0, ios::beg);

    if (fileSize % sizeof(OutBlock) != 0) {
        throw runtime_error("File size is not a multiple of struct size — corrupt file?");
    }

    size_t count = fileSize / sizeof(OutBlock);
    vector<OutBlock> blocks(count);
    in_b.read(reinterpret_cast<char*>(blocks.data()), fileSize);
    Timer::stop("Binary");



    // READABLE
    Timer::start();
    string readable_file = "src/TEST/block_data_readable.m";
    filesystem::remove(readable_file);

    // write
    {
        ofstream out_r(readable_file);
        for (int i = 0; i < iterations; ++i) {
            out_r << t.x << ',' << t.y << ',' << t.z << ' '
                << "leaves_green" << ' '
                << static_cast<int>(t.size) << '\n';
        }
    }


    // read
    vector<OutBlock> back_blocks(iterations);
    ifstream in_r(readable_file);
    string line;
    while (getline(in_r, line)) {
        int idx = 0;
        if (line.empty()) continue;
        OutBlock b{};
        const char* p = line.data();
        char* end;
        b.x = strtoll(p, &end, 10); p = end + 1;
        b.y = strtoll(p, &end, 10); p = end + 1;
        b.z = strtoll(p, &end, 10); p = end + 1;
        while (*p == ' ') ++p;
        while (*p != ' ') ++p;  // skip material
        while (*p == ' ') ++p;
        b.size = strtoul(p, &end, 10);
        b.material = 1;
        back_blocks[idx++] = b;
    }
    Timer::stop("Readable");

}

void space_gen_to_blocks_test() {

    // 219106, 309968, -410448
    shared_ptr<StellarCoordinate> view_pos = make_shared<StellarCoordinate>(219106, 309968, -410400, 0, 0, 0, 0, 0, 0 );
    shared_ptr<SpaceGen::SpaceGenResult> res = SpaceGen::generate_space(view_pos);
    
    IsometricRenderer::scale_to_fit_in_small_image(res->blocks);
    IsometricRenderer::render_s(res->blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 8);
    stbi_write_hdr("src/TEST/output/SKY.hdr", SkyGen::IMAGE_WIDTH, SkyGen::IMAGE_HEIGHT, SkyGen::FLOATS_PER_PIXEL, res->sky_box);
}

void server_test() {
    string test_world = "test_world";
    filesystem::create_directories(test_world + "/players");
    filesystem::create_directories(test_world + "/op");

    int test_port = 21000;
    Settings::WORLD_FOLDER = test_world;
    Settings::PORT = test_port;
    Settings::WORKER_THREADS = 2;
    Settings::TIMEOUT_MS = 1000;

    Server server(test_port, 1000, 2);
    thread server_thread([&server]() { server.run(); });
    this_thread::sleep_for(chrono::milliseconds(500));

    NetworkClient client;
    int ping_res = client.ping();
    cout << "Ping result: " << ping_res << endl;

    server.data.running = false;
    server_thread.join();
    filesystem::remove_all(test_world);
    cout << "Server test done" << endl;
}

void server_keepalive_test() {
    string test_world = "test_world";
    filesystem::create_directories(test_world + "/players");
    filesystem::create_directories(test_world + "/op");

    int test_port = 21000;
    Settings::WORLD_FOLDER = test_world;
    Settings::PORT = test_port;
    Settings::WORKER_THREADS = 2;
    Settings::TIMEOUT_MS = 1000;

    Server server(test_port, 1000, 2);
    thread server_thread([&server]() { server.run(); });
    this_thread::sleep_for(chrono::milliseconds(500));

    NetworkClient client;
    int p1 = client.ping();
    this_thread::sleep_for(chrono::milliseconds(600)); // stay under the PING rate limit (500ms)
    int p2 = client.ping();
    this_thread::sleep_for(chrono::milliseconds(600));
    int p3 = client.ping();
    cout << "Pings: " << p1 << ", " << p2 << ", " << p3 << endl;
    cout << "Active TCP connections: " << server.active_tcp_connections << endl;

    // all pings answered (server sends status 7, truncated to 1 bit -> 1)
    ASSERT(p1 == 1, true);
    ASSERT(p2 == 1, true);
    ASSERT(p3 == 1, true);
    // three requests reused one persistent connection
    ASSERT(server.active_tcp_connections == 1, true);

    // puzzle response is 75*75*3 = 16875 bytes - exercises the 18-bit image
    // size field and framing across multiple TCP segments
    Interface::Puzzle puzz = client.puzzle();
    cout << "Puzzle image bytes: " << puzz.image.size() << endl;
    ASSERT(puzz.image.size() == (size_t)puzzle_square * puzzle_square * 3, true);

    server.data.running = false;
    server_thread.join();
    filesystem::remove_all(test_world);
    cout << "Server keepalive test done" << endl;
}

void binary_test() {
    uint8_t bits[4];
    uint32_t offset = 0;
    BitUtil::pack(bits, offset, 4, 4);
    BitUtil::pack(bits, offset, 123, 11);

    BitUtil::print(bits, 4, {{"local", 4}, {"player_id", 11}});
}

void cosmic_river_test() {
    auto universe = SpaceGen::generate_universe();
    auto& rivers = universe->main_cosmic_rivers;

    int64_t min_x = numeric_limits<int64_t>::max();
    int64_t max_x = numeric_limits<int64_t>::min();
    int64_t min_y = numeric_limits<int64_t>::max();
    int64_t max_y = numeric_limits<int64_t>::min();
    int64_t min_z = numeric_limits<int64_t>::max();
    int64_t max_z = numeric_limits<int64_t>::min();

    for (auto& river : rivers) {
        for (auto& pt : river->path_points) {
            if (pt->quadrant_x < min_x) min_x = pt->quadrant_x;
            if (pt->quadrant_x > max_x) max_x = pt->quadrant_x;
            if (pt->quadrant_y < min_y) min_y = pt->quadrant_y;
            if (pt->quadrant_y > max_y) max_y = pt->quadrant_y;
            if (pt->quadrant_z < min_z) min_z = pt->quadrant_z;
            if (pt->quadrant_z > max_z) max_z = pt->quadrant_z;
        }
    }

    double range_x = double(max_x - min_x);
    double range_y = double(max_y - min_y);
    double range_z = double(max_z - min_z);
    double max_span = max({range_x, range_y, range_z});
    double scale = 128.0 / max_span;

    vector<shared_ptr<Block>> blocks;
    for (auto& river : rivers) {
        for (auto& pt : river->path_points) {
            double x = double(pt->quadrant_x - min_x) * scale;
            double y = double(pt->quadrant_y - min_y) * scale;
            double z = double(pt->quadrant_z - min_z) * scale;
            blocks.push_back(make_block(make_shared<PositionDouble>(x, y, z), BMaterial::ANDESTITE));
        }
    }

    cout << "Cosmic River Test: " << rivers.size() << " rivers, "
         << blocks.size() << " path points" << endl;

    IsometricRenderer::render_s(blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 4);
}

void economy_test() {
    cout << "\n=== Economy Simulation Test ===\n" << endl;

    // create 3 shared products so economies can trade with each other
    auto food = make_shared<Product>();
    food->name = "Food";
    food->category = Product::ProductCategory::MANUFACTURED_GOOD;
    food->population_tied = true;

    auto ore = make_shared<Product>();
    ore->name = "Ore";
    ore->category = Product::ProductCategory::RAW_MATERIAL;

    auto tools = make_shared<Product>();
    tools->name = "Tools";
    tools->category = Product::ProductCategory::MANUFACTURED_GOOD;

    vector<shared_ptr<Product>> catalog = {food, ore, tools};

    // create 3 economies with different specializations and locations
    // Unbalanced setup: overcapacity in specialties, uniform prices
    auto eco_a = make_shared<Economy>();
    eco_a->technological_advancement = 60;
    eco_a->max_storage_capacity = 50000;
    eco_a->cash = 100000;
    eco_a->location = make_shared<StellarCoordinate>(0, 0, 0, 0, 0, 0, 0, 0, 0);
    eco_a->economy_seed = eco_a->location->to_seed();
    eco_a->population = 200;
    eco_a->total_productive_capacity = 215;
    eco_a->production_amounts[food] = 200;
    eco_a->max_production[food] = 300;
    eco_a->desired_amounts[food] = 10;
    eco_a->production_amounts[ore] = 5;
    eco_a->max_production[ore] = 20;
    eco_a->desired_amounts[ore] = 100;
    eco_a->production_amounts[tools] = 10;
    eco_a->max_production[tools] = 30;
    eco_a->desired_amounts[tools] = 50;
    for (auto& p : catalog) {
        eco_a->local_prices[p] = 50.0;
        eco_a->stored_amounts[p] = 20.0;
    }

    auto eco_b = make_shared<Economy>();
    eco_b->technological_advancement = 40;
    eco_b->max_storage_capacity = 50000;
    eco_b->cash = 80000;
    eco_b->location = make_shared<StellarCoordinate>(0, 0, 0, 100, 0, 0, 0, 0, 0);
    eco_b->economy_seed = eco_b->location->to_seed();
    eco_b->population = 300;
    eco_b->total_productive_capacity = 315;
    eco_b->production_amounts[food] = 10;
    eco_b->max_production[food] = 30;
    eco_b->desired_amounts[food] = 80;
    eco_b->production_amounts[ore] = 300;
    eco_b->max_production[ore] = 500;
    eco_b->desired_amounts[ore] = 5;
    eco_b->production_amounts[tools] = 5;
    eco_b->max_production[tools] = 15;
    eco_b->desired_amounts[tools] = 30;
    for (auto& p : catalog) {
        eco_b->local_prices[p] = 50.0;
        eco_b->stored_amounts[p] = 20.0;
    }

    auto eco_c = make_shared<Economy>();
    eco_c->technological_advancement = 80;
    eco_c->max_storage_capacity = 50000;
    eco_c->cash = 120000;
    eco_c->location = make_shared<StellarCoordinate>(0, 0, 0, 0, 200, 0, 0, 0, 0);
    eco_c->economy_seed = eco_c->location->to_seed();
    eco_c->population = 250;
    eco_c->total_productive_capacity = 175;
    eco_c->production_amounts[food] = 15;
    eco_c->max_production[food] = 40;
    eco_c->desired_amounts[food] = 40;
    eco_c->production_amounts[ore] = 10;
    eco_c->max_production[ore] = 30;
    eco_c->desired_amounts[ore] = 80;
    eco_c->production_amounts[tools] = 150;
    eco_c->max_production[tools] = 250;
    eco_c->desired_amounts[tools] = 5;
    for (auto& p : catalog) {
        eco_c->local_prices[p] = 50.0;
        eco_c->stored_amounts[p] = 20.0;
    }

    // ANSI color helpers
    auto rst = "\033[0m";
    auto bold = "\033[1m";
    auto dim = "\033[2m";
    auto red = "\033[31m";
    auto green = "\033[32m";
    auto yellow = "\033[33m";
    auto blue = "\033[34m";
    auto magenta = "\033[35m";
    auto cyan = "\033[36m";

    auto price_color = [&](double p) -> const char* {
        if (p > 200) return red;
        if (p > 80) return yellow;
        if (p < 20) return green;
        return "";
    };
    auto cap_color = [&](double used, double max) -> const char* {
        if (used > max * 0.95) return red;
        if (used > max * 0.75) return yellow;
        return "";
    };
    auto tag_color = [&](const string& name) -> const char* {
        if (name == "A") return cyan;
        if (name == "B") return yellow;
        return green;
    };

    vector<shared_ptr<Economy>> economies = {eco_a, eco_b, eco_c};

    struct EcoInfo { shared_ptr<Economy> e; string name; string tag; };
    vector<EcoInfo> infos = {
        {eco_a, "A", "food"},
        {eco_b, "B", "ore"},
        {eco_c, "C", "tools"},
    };

    auto print_state = [&](const string& label) {
        cout << "\n" << bold << label << rst << "\n";
        for (auto& [e, name, tag] : infos) {
            double stored = 0;
            for (auto& [p, a] : e->stored_amounts) stored += a;
            auto tc = tag_color(name);
            auto cc = cap_color(stored, e->max_storage_capacity);
            cout << "  " << tc << bold << name << rst << " (" << tag << ")"
                 << dim << " tech=" << e->technological_advancement << rst
                 << " " << bold << "cash=" << tc << e->cash << rst
                 << " stored=" << cc << stored << rst << "/" << e->max_storage_capacity << "\n";
            for (auto& p : catalog) {
                double price = e->local_prices[p];
                auto pc = price_color(price);
                cout << "    " << bold << p->name << rst
                     << ":  " << dim << "prod=" << rst << e->production_amounts[p]
                     << "/" << e->max_production[p]
                     << "  " << dim << "want=" << rst << e->desired_amounts[p]
                     << "  " << dim << "eff=" << rst << e->effective_demand(p)
                     << "  " << dim << "store=" << rst << e->stored_amounts[p]
                     << "  " << dim << "price=" << rst << pc << bold << price << rst << "\n";
            }
        }
    };

    // initial distances
    cout << "\n" << bold << "Distances (LY):" << rst << "\n";
    cout << "  " << cyan << "A→B" << rst << ": " << Economy::distance_between_ptr(eco_a.get(), eco_b.get()) << "\n";
    cout << "  " << cyan << "A→C" << rst << ": " << Economy::distance_between_ptr(eco_a.get(), eco_c.get()) << "\n";
    cout << "  " << yellow << "B→C" << rst << ": " << Economy::distance_between_ptr(eco_b.get(), eco_c.get()) << "\n";

    print_state("=== Initial State (Unbalanced) ===");

    // balance each economy to near-equilibrium
    cout << "\n" << bold << "Balancing..." << rst << endl;
    for (auto& eco : economies) {
        eco->balance(30);
    }
    print_state("=== After Balance ===");

    // tracking
    vector<double> cash_a, cash_b, cash_c;
    vector<double> price_food_a, price_ore_b, price_tools_c;

    int ticks = 1000;
    for (int t = 0; t < ticks; ++t) {
        for (auto& eco : economies) {
            eco->tick(economies);
        }

        // track and print every 50 ticks
        if (t % 50 == 0 || t == ticks - 1) {
            cash_a.push_back(eco_a->cash);
            cash_b.push_back(eco_b->cash);
            cash_c.push_back(eco_c->cash);
            price_food_a.push_back(eco_a->local_prices[food]);
            price_ore_b.push_back(eco_b->local_prices[ore]);
            price_tools_c.push_back(eco_c->local_prices[tools]);
            print_state("--- Tick " + to_string(t) + " ---");
        }

        // inject war at tick 500
        if (t == 500) {
            cout << "\n" << bold << red << "=== WAR EVENT at tick 500 ===" << rst << "\n";
            eco_a->register_event({EconomyEvent::Type::TECH_LOSS, 15.0, nullptr});
            eco_a->register_event({EconomyEvent::Type::PRODUCTION_SHOCK, 0.3, nullptr});
            eco_b->register_event({EconomyEvent::Type::TECH_LOSS, 20.0, nullptr});
            eco_b->register_event({EconomyEvent::Type::PRODUCTION_SHOCK, 0.4, tools});
        }

        // track metrics every 10 ticks for graphs
        if (t % 10 == 0 && t % 50 != 0) {
            cash_a.push_back(eco_a->cash);
            cash_b.push_back(eco_b->cash);
            cash_c.push_back(eco_c->cash);
            price_food_a.push_back(eco_a->local_prices[food]);
            price_ore_b.push_back(eco_b->local_prices[ore]);
            price_tools_c.push_back(eco_c->local_prices[tools]);
        }
    }

    // count total trades by summing absolute cash changes from initial
    double init_cash_total = 100000 + 80000 + 120000;
    double final_cash_total = eco_a->cash + eco_b->cash + eco_c->cash;
    // government leakage and taxes reduce total cash — that's expected
    double cash_leakage = init_cash_total - final_cash_total;

    // print results
    cout << "\n" << bold << "=== Results after " << ticks << " ticks ===" << rst << "\n";
    auto pr = [&](double p) { auto c = price_color(p); cout << c << bold << p << rst; };
    cout << cyan << bold << "  A" << rst << " (food)  cash=" << eco_a->cash << "  prices: food="; pr(eco_a->local_prices[food]); cout << " ore="; pr(eco_a->local_prices[ore]); cout << " tools="; pr(eco_a->local_prices[tools]); cout << "\n";
    cout << yellow << bold << "  B" << rst << " (ore)   cash=" << eco_b->cash << "  prices: food="; pr(eco_b->local_prices[food]); cout << " ore="; pr(eco_b->local_prices[ore]); cout << " tools="; pr(eco_b->local_prices[tools]); cout << "\n";
    cout << green << bold << "  C" << rst << " (tools) cash=" << eco_c->cash << "  prices: food="; pr(eco_c->local_prices[food]); cout << " ore="; pr(eco_c->local_prices[ore]); cout << " tools="; pr(eco_c->local_prices[tools]); cout << "\n";
    cout << dim << "Cash leakage (tax+corruption): " << cash_leakage << rst << "\n";

    // plot price history
    vector<pair<double, double>> food_a_plot, ore_b_plot, tools_c_plot;
    for (size_t i = 0; i < price_food_a.size(); ++i) {
        food_a_plot.push_back({static_cast<double>(i * 10), price_food_a[i]});
        ore_b_plot.push_back({static_cast<double>(i * 10), price_ore_b[i]});
        tools_c_plot.push_back({static_cast<double>(i * 10), price_tools_c[i]});
    }
    Grapher::graph_points(food_a_plot, "src/TEST/output/eco_food_price_a.bmp");
    Grapher::graph_points(ore_b_plot, "src/TEST/output/eco_ore_price_b.bmp");
    Grapher::graph_points(tools_c_plot, "src/TEST/output/eco_tools_price_c.bmp");

    // cash plots
    vector<pair<double, double>> cash_a_plot, cash_b_plot, cash_c_plot;
    for (size_t i = 0; i < cash_a.size(); ++i) {
        cash_a_plot.push_back({static_cast<double>(i * 10), cash_a[i]});
        cash_b_plot.push_back({static_cast<double>(i * 10), cash_b[i]});
        cash_c_plot.push_back({static_cast<double>(i * 10), cash_c[i]});
    }
    Grapher::graph_points(cash_a_plot, "src/TEST/output/eco_cash_a.bmp");
    Grapher::graph_points(cash_b_plot, "src/TEST/output/eco_cash_b.bmp");
    Grapher::graph_points(cash_c_plot, "src/TEST/output/eco_cash_c.bmp");

    // === ASSERTIONS ===

    // 1. Prices changed (no stagnant economy)
    bool prices_moved = false;
    for (auto& eco : economies) {
        for (auto& [p, price] : eco->local_prices) {
            if (fabs(price - 50.0) > 1.0) prices_moved = true;
        }
    }
    ASSERT(prices_moved, true);
    cout << "  PASS: prices changed from initial 50.0\n";

    // 2. Cash differs per economy (trade rebalanced wealth)
    bool cash_diverged = (eco_a->cash != eco_b->cash)
                      && (eco_a->cash != eco_c->cash)
                      && (eco_b->cash != eco_c->cash);
    ASSERT(cash_diverged, true);
    cout << "  PASS: cash diverged across economies\n";

    // 3. Storage respected (no economy wildly over capacity)
    for (auto& eco : economies) {
        double stored = 0;
        for (auto& [p, amt] : eco->stored_amounts) stored += amt;
        ASSERT(stored <= eco->max_storage_capacity * 1.05, true);
    }
    cout << "  PASS: all economies within storage capacity\n";

    // 4. At least one economy changed cash by > 10% (meaningful trade happened)
    double a_change = fabs(eco_a->cash - 100000) / 100000;
    double b_change = fabs(eco_b->cash - 80000) / 80000;
    double c_change = fabs(eco_c->cash - 120000) / 120000;
    bool meaningful_trade = a_change > 0.1 || b_change > 0.1 || c_change > 0.1;
    ASSERT(meaningful_trade, true);
    cout << "  PASS: meaningful trade occurred\n";

    // 5. Leakage is not too extreme
    ASSERT(cash_leakage < 50000, true);
    cout << "  PASS: cash leakage within tolerance\n";

    // 6. Products moved between economies (goods transferred)
    double total_food = 0, total_ore = 0, total_tools = 0;
    for (auto& eco : economies) {
        total_food += eco->stored_amounts[food] + eco->production_amounts[food];
        total_ore += eco->stored_amounts[ore] + eco->production_amounts[ore];
        total_tools += eco->stored_amounts[tools] + eco->production_amounts[tools];
    }
    // total goods in the system should be positive
    ASSERT(total_food > 0, true);
    ASSERT(total_ore > 0, true);
    ASSERT(total_tools > 0, true);
    cout << "  PASS: goods circulating in the system\n";

    cout << "\n=== Economy Simulation Test Complete ===\n";
}

// pack -> unpack round trips for every redesigned Interface message, plus the
// message_length framing that the TCP layer uses to extract messages
void protocol_roundtrip_test() {
    uint8_t buf[4096];

    // tiered ids: smallest fitting tier is always chosen
    ASSERT(Interface::id_tier_bits(0) == 8, true);
    ASSERT(Interface::id_tier_bits(1) == 13, true);
    ASSERT(Interface::id_tier_bits(2) == 16, true);
    ASSERT(Interface::id_tier_bits(3) == 64, true);
    for (uint64_t id : {0ULL, 1ULL, 255ULL, 256ULL, 8191ULL, 65535ULL, 65536ULL, 9999999999999ULL}) {
        uint8_t ib[16];
        uint32_t off = 0;
        Interface::pack_id(ib, off, id);
        off = 0;
        ASSERT(Interface::unpack_id(ib, off), id);
    }
    for (int64_t v : {0LL, 1LL, -1LL, 123456789LL, -123456789LL, 1LL << 52, -(1LL << 52)}) {
        ASSERT(Interface::unpack_pneg(Interface::pack_pneg(v)), v);
    }

    // ClientLocalPositionUpdate (fixed short message)
    {
        Interface::ClientLocalPositionUpdate m;
        m.x = 512; m.y = 300; m.z = 4000;
        m.rot_x = 7; m.rot_y = 2; m.rot_z = 15;
        uint32_t bits = m.pack(buf);
        ASSERT(bits, (uint32_t)Interface::ClientLocalPositionUpdate::BIT_LENGTH);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::ClientLocalPositionUpdate::unpack(buf);
        ASSERT(u.x, 512); ASSERT(u.y, 300); ASSERT(u.z, 4000);
        ASSERT(u.rot_z, 15);
    }

    // LocalBatchPositionUpdate (short, tiered ids)
    {
        Interface::BatchPositionUpdate m;
        Interface::BatchPositionUpdate::Entry e;
        e.player_id = 5000; e.x = 1; e.y = 2; e.z = 3; e.rot_x = 1; e.rot_y = 2; e.rot_z = 3;
        m.entries.push_back(e);
        e.player_id = 999999999ULL; e.x = 100; e.y = 200; e.z = 300; e.rot_x = 9; e.rot_y = 1; e.rot_z = 0;
        m.entries.push_back(e);
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::BatchPositionUpdate::unpack(buf);
        ASSERT(u.entries.size(), 2);
        ASSERT(u.entries[0].player_id, 5000);
        ASSERT(u.entries[1].player_id, 999999999ULL);
        ASSERT(u.entries[1].z, 300);
    }

    // PositionUpdate: all four encodings (terrain/quadrant/lightyear/km)
    {
        Interface::PositionUpdate m;
        m.space = false; m.t_x = 1000; m.t_y = -500; m.t_z = 42;
        uint32_t bits = m.pack(buf);
        ASSERT(bits, (uint32_t)Interface::PositionUpdate::BIT_LENGTH_TERRAIN);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::PositionUpdate::unpack(buf);
        ASSERT(u.space, false); ASSERT(u.t_x, 1000); ASSERT(u.t_y, -500); ASSERT(u.t_z, 42);

        m.space = true; m.space_change = 0;
        m.q_x = 123; m.q_y = -456; m.q_z = 789;
        m.ly_x = 5; m.ly_y = 6; m.ly_z = 7;
        m.km_x = 1; m.km_y = 2; m.km_z = 3;
        bits = m.pack(buf);
        ASSERT(bits, (uint32_t)Interface::PositionUpdate::BIT_LENGTH_SPACE_QUADRANT);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        u = Interface::PositionUpdate::unpack(buf);
        ASSERT(u.space_change, 0); ASSERT(u.q_x, 123); ASSERT(u.ly_y, 6); ASSERT(u.km_x, 1);

        m.space_change = 1; m.q_x = m.q_y = m.q_z = 0;
        bits = m.pack(buf);
        ASSERT(bits, (uint32_t)Interface::PositionUpdate::BIT_LENGTH_SPACE_LIGHTYEAR);
        u = Interface::PositionUpdate::unpack(buf);
        ASSERT(u.space_change, 1);

        m.space_change = 2; m.ly_x = m.ly_y = m.ly_z = 0;
        m.km_x = 12345; m.km_y = 9999999999ULL; m.km_z = 0;
        bits = m.pack(buf);
        ASSERT(bits, (uint32_t)Interface::PositionUpdate::BIT_LENGTH_SPACE_KM);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        u = Interface::PositionUpdate::unpack(buf);
        ASSERT(u.space_change, 2); ASSERT(u.km_y, 9999999999ULL);
    }

    // Hit / Attack / Dead / Resync
    {
        Interface::Hit h;
        h.velocity = 8191; h.target_entity = 7000;
        uint32_t bits = h.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto uh = Interface::Hit::unpack(buf);
        ASSERT(uh.velocity, 8191); ASSERT(uh.target_entity, 7000);

        Interface::Attack a;
        a.player_id = 123; a.angle_provided = true; a.rot_x = 3; a.rot_y = 4; a.rot_z = 5;
        bits = a.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto ua = Interface::Attack::unpack(buf);
        ASSERT(ua.player_id, 123); ASSERT(ua.angle_provided, true); ASSERT(ua.rot_z, 5);

        Interface::Dead d;
        d.player_id = 99999;
        bits = d.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto ud = Interface::Dead::unpack(buf);
        ASSERT(ud.player_id, 99999);

        Interface::Resync r;
        Interface::Resync::Entry e;
        e.player_id = 5; e.alive = 1;
        r.entries.push_back(e);
        e.player_id = 100000; e.alive = 0;
        r.entries.push_back(e);
        bits = r.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto ur = Interface::Resync::unpack(buf);
        ASSERT(ur.entries.size(), 2);
        ASSERT(ur.entries[0].alive, 1); ASSERT(ur.entries[1].alive, 0);
    }

    // Schemata: bit-packed path (few blocks)
    {
        Interface::Schemata m;
        m.schemata_id = 77;
        m.blocks.push_back(make_block(make_shared<PositionDouble>(0, 0, 0), "ICE"));
        m.blocks.push_back(make_block(make_shared<PositionDouble>(1, 0, 0), "BDOG"));
        m.blocks.push_back(make_block(make_shared<PositionDouble>(0, 2, 3), "DEFAULT"));
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::Schemata::unpack(buf);
        ASSERT(u.schemata_id, 77);
        ASSERT(u.blocks.size(), 3);
        ASSERT(u.blocks[0]->material, "ICE");
        ASSERT(u.blocks[2]->position_double->y, 2);
    }

    // Schemata: range-compressed path (>20 blocks)
    {
        Interface::Schemata m;
        m.schemata_id = 78;
        for (int x = 0; x < 25; x++)
            m.blocks.push_back(make_block(make_shared<PositionDouble>(x, x % 3, x % 5), x % 2 == 0 ? "ICE" : "BDOG"));
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::Schemata::unpack(buf);
        ASSERT(u.blocks.size(), 25);
        for (int i = 0; i < 25; i++) {
            ASSERT(u.blocks[i]->position_double->x, i);
            ASSERT(u.blocks[i]->position_double->y, i % 3);
            ASSERT(u.blocks[i]->material, i % 2 == 0 ? "ICE" : "BDOG");
        }
    }

    // Blocks: chunk offset + blocks + schemata placements
    {
        Interface::Blocks m;
        m.chunk_x = 100; m.chunk_y = -200; m.chunk_z = 300;
        m.blocks.push_back(make_block(make_shared<PositionDouble>(100, -200, 300), "ICE"));
        m.blocks.push_back(make_block(make_shared<PositionDouble>(105, -200, 300), "VOID"));
        Interface::Blocks::SchemataPlacement p;
        p.schemata_id = 500; p.x = 10; p.y = 11; p.z = 12; p.rotation = 3;
        m.placements.push_back(p);
        p.schemata_id = 6000000; p.x = 1; p.y = 2; p.z = 3; p.rotation = 0;
        m.placements.push_back(p);
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::Blocks::unpack(buf);
        ASSERT(u.chunk_x, 100); ASSERT(u.chunk_y, -200); ASSERT(u.chunk_z, 300);
        ASSERT(u.blocks.size(), 2);
        ASSERT(u.blocks[1]->material, "VOID");
        ASSERT(u.blocks[1]->position_double->x, 105);
        ASSERT(u.placements.size(), 2);
        ASSERT(u.placements[0].rotation, 3);
        ASSERT(u.placements[1].schemata_id, 6000000);
    }

    // Vehicle
    {
        Interface::Vehicle m;
        m.chunk_x = -5; m.chunk_y = 0; m.chunk_z = 7;
        m.blocks.push_back(make_block(make_shared<PositionDouble>(-5, 0, 7), "ICE"));
        m.meta_data = {1, 2, 3, 250};
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::Vehicle::unpack(buf);
        ASSERT(u.chunk_x, -5);
        ASSERT(u.blocks.size(), 1);
        ASSERT(u.meta_data.size(), 4);
        ASSERT(u.meta_data[3], 250);
    }

    // Placement: all three variants
    {
        Interface::Placement m;
        m.vehicle = false; m.schemata = false;
        m.pos_x = 1234; m.pos_y = -99; m.pos_z = 5000; m.material = 5000;
        uint32_t bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        auto u = Interface::Placement::unpack(buf);
        ASSERT(u.pos_x, 1234); ASSERT(u.pos_y, -99); ASSERT(u.material, 5000);

        m.schemata = true; m.schemata_id = 42; m.rotation = 2;
        bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        u = Interface::Placement::unpack(buf);
        ASSERT(u.schemata, true); ASSERT(u.schemata_id, 42); ASSERT(u.rotation, 2);

        m.schemata = false; m.vehicle = true;
        m.vehicle_id = 9000000; m.vchunk_x = 1; m.vchunk_y = -2; m.vchunk_z = 3;
        m.vx = 100; m.vy = 200; m.vz = 300;
        bits = m.pack(buf);
        ASSERT(Interface::message_length((char*)buf, (bits + 7) / 8), (int)((bits + 7) / 8));
        u = Interface::Placement::unpack(buf);
        ASSERT(u.vehicle, true); ASSERT(u.vehicle_id, 9000000); ASSERT(u.vchunk_y, -2); ASSERT(u.vz, 300);
    }

    // framing must return 0 until the whole message is buffered, then exactly its byte length
    {
        Interface::Blocks m;
        m.chunk_x = 1; m.chunk_y = 2; m.chunk_z = 3;
        for (int i = 0; i < 5; i++)
            m.blocks.push_back(make_block(make_shared<PositionDouble>(i, i, i), "ICE"));
        uint32_t bits = m.pack(buf);
        int full = (bits + 7) / 8;
        for (int cut = 0; cut < full; cut++)
            ASSERT(Interface::message_length((char*)buf, cut), 0);
        ASSERT(Interface::message_length((char*)buf, full), full);
    }

    cout << "Protocol roundtrip test done" << endl;
}

void km_coordinate_test() {
    cout << "km_coordinate_test: start" << endl;

    // 1. Sub-light-year offset survives offset_coordinate + ly_offset round-trip
    auto base = make_shared<StellarCoordinate>();
    base->quadrant_x = 7; base->quadrant_y = -3; base->quadrant_z = 42;
    base->light_year_x = 5000; base->light_year_y = 100; base->light_year_z = 9999;

    double km_off = 0.25 * SpaceGen::KM_PER_LY;
    auto moved = SpaceGen::offset_coordinate(base, km_off / SpaceGen::KM_PER_LY, 0, 0);
    ASSERT(moved->quadrant_x, 7);
    ASSERT(moved->light_year_x, 5000);
    ASSERT(moved->km_x, (uint64_t)km_off);

    double ox, oy, oz;
    double dist = SpaceGen::viewer_dist(base, moved, ox, oy, oz);
    ASSERT(ox > 0.249999 && ox < 0.250001, true);
    ASSERT(fabs(dist - 0.25) < 1e-3, true);

    // 2. Negative km offset borrows a light-year (from light_year 0, quadrant decrements)
    auto edge = make_shared<StellarCoordinate>();
    edge->quadrant_x = 10; edge->light_year_x = 0;
    auto neg = SpaceGen::offset_coordinate(edge, -km_off / SpaceGen::KM_PER_LY, 0, 0);
    ASSERT(neg->quadrant_x, 9);
    ASSERT(neg->light_year_x, 9999);
    ASSERT(neg->km_x, (uint64_t)llround(0.75 * SpaceGen::KM_PER_LY));
    double ex, ey, ez;
    SpaceGen::ly_offset(edge, neg, ex, ey, ez);
    ASSERT(ex > -0.250001 && ex < -0.249999, true);

    // 3. Integer-LY offset behavior preserved when km are zero
    auto li = SpaceGen::offset_coordinate(base, 3.0, -2.0, 0.0);
    ASSERT(li->light_year_x, 5003);
    ASSERT(li->light_year_y, 98);

    // 4. offset_coordinate_ly (systems and larger) always keeps km = 0
    auto ly_only = SpaceGen::offset_coordinate_ly(base, 0.5, -1.5, 2.0);
    ASSERT(ly_only->km_x, 0ULL);
    ASSERT(ly_only->km_y, 0ULL);
    ASSERT(ly_only->km_z, 0ULL);
    ASSERT(ly_only->light_year_x, 5000); // 0.5 truncated
    ASSERT(ly_only->light_year_y, 99);   // 100 + (-1)

    // 5. generate_solar_system gives each star/planet a distinct km coordinate
    auto sys_pos = make_shared<StellarCoordinate>();
    sys_pos->quadrant_x = 1; sys_pos->light_year_x = 200;
    auto sys = SpaceGen::generate_solar_system(999, sys_pos, 1.0);
    unordered_set<int64_t> seen;
    bool all_distinct = true;
    auto key = [](shared_ptr<StellarCoordinate> c) {
        return SpaceGen::seed_from_coordinate(c);
    };
    for (auto& star : sys->stars)
        if (!seen.insert(key(star->location)).second) all_distinct = false;
    for (auto& p : sys->planets)
        if (!seen.insert(key(p->location)).second) all_distinct = false;
    ASSERT(all_distinct, true);

    cout << "km_coordinate_test: done" << endl;
}

int main() {

    print_pid();

    // proximity_grid_test();
    // protocol_roundtrip_test();

    // server_keepalive_test();
    // random_test();
    // space_sky_test();
    plant_gen_test();
    // km_coordinate_test();
    // voxel_galaxy_test();
    // voxel_planet_test();
    // server_test();
    // space_gen_to_blocks_test();
    // economy_test();

    // server_binary_vs_readable_storage();

    return 0;
}
