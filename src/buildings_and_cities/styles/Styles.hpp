#pragma once

#include "util/Grid.hpp"
#include "../../world/maps/IsometricRenderer.hpp"

using namespace std;




enum class StyleElementType {
    FRAME,
    // X PORCH,
    JUT_OUT,
    ROOF,
    WALL,
    COLUMN,
    STAIR,
    // X TOWER,
    // X SPIRE,
    CHIMNEY,
    // X FORTIFICATION,
    LIGHT,
    WINDOW,
    // HANGING_DECORATION,
    // SIGN,
    WALL_DESIGN,
    // WIRE_ROPE_FLAG,
    // + BED,
    // + CHAIR,
    // + TABLE,
    // + COUNTER,
    // + SINK
};


struct StyleElement {
    StyleElementType type;
    vector<string> materials;
};


struct FrameStyleElement : StyleElement {
    Grid<bool> cross_section_grid;
    vector<int> sides; // top right bottom left => 0, 1, 2, 3

    void render_cross_section() {
        vector<shared_ptr<Block>> blocks;
        for (int y = 0; y < cross_section_grid.height; ++y) {
            for (int z = 0; z < cross_section_grid.depth; ++z) {
                for (int x = 0; x < cross_section_grid.width; ++x) {
                    shared_ptr<Block> block = std::make_shared<Block>();
                    block->position_double = std::make_shared<PositionDouble>(x, y, z);
                    block->size = 1;
                    block->material = "";
                    blocks.push_back(block);
                }
            }
        }
        IsometricRenderer::render_and_save_png(blocks, {}, IsoAngle::NE, "src/TEST/output/CROSS_SECTION.png", 32);
    }
};

struct RoofStyleElement : StyleElement {
    double pitch; // 0 flat 100 super steep
    double curvature; // 0 curve, 50 straight, 100 dome
    int overhang; // overhang in blocks
    int layers; // roofs over roofs thing like in asian countries
    Grid<bool> surface_texture_profile; // a small repeatable section to give the roof texture
};

struct ColumnStyleElement : StyleElement {
    bool round;
    shared_ptr<FrameStyleElement> frame;
    bool lines;

    shared_ptr<ColumnStyleElement> top_column_style_element;
    shared_ptr<ColumnStyleElement> bottom_column_style_element;

};

struct WallDesignElement : StyleElement {
    Grid<bool> pattern; // XY pattern: pattern[{x, 0, y}] = true => decorative block
};

enum class WallAngle { N, NE, E, SE, S, SW, W, NW };
struct WallStyleElement : StyleElement {
    int thickness;
    double outward_slope; // slope of the wall, 0 is straight up, 0.5 being a 45 degree slope
    double inward_slope; // same as outward slope but on the inside of the wall
    shared_ptr<FrameStyleElement> frame;
    shared_ptr<WallDesignElement> wall_design = nullptr;
};

struct JutOutStyleElement : StyleElement {
    shared_ptr<RoofStyleElement> roof = nullptr; // optionally has roof
    shared_ptr<ColumnStyleElement> column = nullptr; // column shaped is stretched or shrunk by adding or removing layers in the middle

    // these are stretched as well to fit the jut out size
    shared_ptr<FrameStyleElement> bottom_support = nullptr; // optionally has bottom supports
    bool smoothed_bottom_support;
    int bottom_support_height;
    shared_ptr<WallStyleElement> railing_or_wall; // can be empty
};

struct StairStyleElement : StyleElement {
    int stair_size_in_blocks = 1;
    shared_ptr<WallStyleElement> railing_or_wall; // can be empty

    bool solid_bottom;
};

struct ChimneyStyleElement : StyleElement {
    shared_ptr<ColumnStyleElement> interior_column;
    shared_ptr<ColumnStyleElement> exterior_column;

    int fire_place_width;
    int fire_place_height;
    bool round;
};

struct LightStyleElement : StyleElement {
    string light_block_material;
    string frame_material;
    Grid<string> shape;
};

struct WindowStyleElement : StyleElement {
    int min_width = 1;
    int max_width = 3;
    int min_height = 1;
    int max_height = 2;
    int sill_height = 1;
    string frame_material;
};

struct StyleGroup {
    shared_ptr<FrameStyleElement> frame;
    shared_ptr<RoofStyleElement> roof;
    shared_ptr<ColumnStyleElement> column;
    shared_ptr<WallStyleElement> wall;
    shared_ptr<JutOutStyleElement> jut_out;
    shared_ptr<StairStyleElement> stair = nullptr;
    shared_ptr<ChimneyStyleElement> chimney = nullptr;
    shared_ptr<LightStyleElement> light = nullptr;
    shared_ptr<LightStyleElement> light_small = nullptr;
    shared_ptr<LightStyleElement> sconce = nullptr;
    shared_ptr<WindowStyleElement> window = nullptr;
    shared_ptr<WallDesignElement> wall_design = nullptr;

};



struct Room;
struct StyleBlock {
    shared_ptr<Block> block;
    shared_ptr<Room> room;

    StyleBlock(shared_ptr<Block> block, shared_ptr<Room> room) {
        this->block = block;
        this->room = room;
    }
};