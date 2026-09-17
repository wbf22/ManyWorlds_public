#pragma once


#include "util/span_array.hpp"
#include "util/BMaterial.hpp"
#include "../../blocks/Block.h"
#include "util/stb_image.h"
#include "util/stb_image_write.h"

#include <memory>
#include <filesystem>
#include <vector>

using namespace std;


enum class IsoAngle {
    NE,
    SE,
    SW,
    NW
};

enum class BlockSide {
    TOP,
    RIGHT,
    BACK
};


struct IsometricRenderer {


    inline static int IMAGE_TEXTURE_SIZE = 32;
    

    // MAIN FUNCTIONS

    /**
     * For rendering a group of blocks in a single image
     * 
     * Iterates through the list of blocks to determine the resulting image dimensions, so shouldn't be used for maps,
     * rather for standalone renders (for maps use 'render_tile')
     * 
     * Probably best for tests as well as renders of schemata or vehicles in game. And aliens if desired.
     */
    static uint8_t* render(vector<shared_ptr<Block>>& blocks, unordered_map<string, uint8_t*>& textures_rgba, IsoAngle angle, int& result_size, double scale=16.0) {

        // return an empty image if no blocks are passe din
        uint8_t background[4] = {10,10,10,255};
        if (blocks.empty()) {
            int64_t image_size = 40 * scale * 1;
            uint8_t* image = make_background_image(background, image_size);
            result_size = image_size;
            return image;
        }
        
        // make default texture if it doesn't exist
        if (textures_rgba.find("") == textures_rgba.end()) {
            int length = IsometricRenderer::IMAGE_TEXTURE_SIZE * IsometricRenderer::IMAGE_TEXTURE_SIZE * 4;
            uint8_t* texture = new uint8_t[length];
            for (int i = 0; i < length; i+=4) {
                // i = (y * image_width + x) * 4;
                int ind = i / 4;
                if (
                    ind % IsometricRenderer::IMAGE_TEXTURE_SIZE == 0 || 
                    ind / IsometricRenderer::IMAGE_TEXTURE_SIZE == 0 ||
                    (ind+1) % IsometricRenderer::IMAGE_TEXTURE_SIZE == 0 || 
                    ind / IsometricRenderer::IMAGE_TEXTURE_SIZE == IsometricRenderer::IMAGE_TEXTURE_SIZE-1
                ) {
                    texture[i] = 210;
                    texture[i+1] = 210;
                    texture[i+2] = 210;
                }
                else {
                    texture[i] = 150;
                    texture[i+1] = 150;
                    texture[i+2] = 150;
                }
                texture[i+3] = 255;
            }
            textures_rgba[""] = texture;
        }

        double min_range, min_y, avg_x, avg_y, avg_z;
        get_range_stats(blocks, min_range, min_y, avg_x, avg_y, avg_z);

        double block_size = blocks[0]->size;
        int64_t square_blocks = (min_range / block_size) * 2;


        // determine bottom left corner position
        double image_bot_left_corner_x, image_bot_left_corner_z;
        if (angle == IsoAngle::NE) { // x+ z- to front
            image_bot_left_corner_x = avg_x + (-min_y*0.5);
            image_bot_left_corner_z = avg_z - (-min_y*0.5) - square_blocks*block_size;
        }
        else if (angle == IsoAngle::SE) { // x+ z+ to front
            image_bot_left_corner_x = avg_x + (-min_y*0.5) + square_blocks*block_size;
            image_bot_left_corner_z = avg_z + (-min_y*0.5);
        }
        else if (angle == IsoAngle::SW) { // x- z+ to front
            image_bot_left_corner_x = avg_x - (-min_y*0.5);
            image_bot_left_corner_z = avg_z + (-min_y*0.5) + square_blocks*block_size;
        }
        else if (angle == IsoAngle::NW) { // x- z- to front
            image_bot_left_corner_x = avg_x - (-min_y*0.5) - square_blocks*block_size;
            image_bot_left_corner_z = avg_z - (-min_y*0.5);
        }


        uint8_t* rendered_image = IsometricRenderer::render_tile(
            image_bot_left_corner_x,
            image_bot_left_corner_z,
            square_blocks,
            blocks,
            textures_rgba,
            background,
            angle,
            scale
        );

        result_size = square_blocks * scale * blocks[0]->size;
        return rendered_image;
    }

    static void render_and_save_png(vector<shared_ptr<Block>>& blocks, unordered_map<string, uint8_t*> textures_rgba, IsoAngle angle, string path, double scale=16.0) {

        int image_size;
        uint8_t* rendered_image = IsometricRenderer::render(
            blocks,
            textures_rgba,
            angle,
            image_size,
            scale
        );
        // cout << image_size << endl;

        stbi_write_png(path.c_str(), image_size, image_size, 4, rendered_image, 0);

    }

    /**
     * Produces a tile from the provided blocks for a iso map.
     * 
     * Given a start coordinate for the bottom left corner of the tile, renders the blocks in the tile
     * 'background' should be a 4 uint8_t array representing a RGBA value with values 0-255
     * 
     * Returns a square image of square_blocks*scale pixels
     */
    static uint8_t* render_tile(
        double image_bot_left_corner_x, 
        double image_bot_left_corner_z, 
        int64_t square_blocks, 
        vector<shared_ptr<Block>>& blocks,
        unordered_map<string, uint8_t*>& textures_rgba,
        uint8_t* background,
        IsoAngle angle,
        double scale=16.0 // pixels per 1m block
    ) {

        if (blocks[0]->size == 0) {
            cout << "\x1b[38;2;255;0;0mIsometricRenderer: got block with size of zero, can't render this" << endl;
            return nullptr;
        }

        // set up image
        int64_t image_size = square_blocks * scale * blocks[0]->size;
        uint8_t* image = make_background_image(background, image_size);

        // get unique blocks
        unordered_set<string> block_poses;
        unordered_map<string, shared_ptr<Block>> top_blocks;
        vector<shared_ptr<Block>> unique_blocks;
        for (shared_ptr<Block> block : blocks) {
            int x = round(block->position_double->x/block->size);
            int y = round(block->position_double->y/block->size);
            int z = round(block->position_double->z/block->size);
            string tag = std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
            if (block_poses.find(tag) != block_poses.end()) {
                // cout << Util::RED << "IsometricRenderer -- duplicate block at: " << block->position_double->toString() << endl;
            }
            else {
                block_poses.insert(tag);
                unique_blocks.push_back(block);

                string xy_tag = m_xy_tag(block);
                shared_ptr<Block> block_in_same_spot = top_blocks[xy_tag];
                if (block_in_same_spot == nullptr || block_in_same_spot->position_double->y < block->position_double->y) {
                    top_blocks[xy_tag] = block;
                }
            }
        }
        block_poses.clear();


        // sort blocks by closeness to viewer unless transparent, then opposite
        std::sort(unique_blocks.begin(), unique_blocks.end(),
            [&](const shared_ptr<Block>& a, const shared_ptr<Block>& b) {
                
                // double a_x = a->position_double->x - image_bot_left_corner_x;
                // double a_y = a->position_double->y;
                // double a_z = a->position_double->z - image_bot_left_corner_z;
                // int a_image_x, a_image_y;
                // IsometricRenderer::block_x_y_z_to_image_x_y(a_x, a_y, a_z, scale, angle, image_size, a_image_x, a_image_y);


                // double b_x = b->position_double->x - image_bot_left_corner_x;
                // double b_y = b->position_double->y;
                // double b_z = b->position_double->z - image_bot_left_corner_z;
                // int b_image_x, b_image_y;
                // IsometricRenderer::block_x_y_z_to_image_x_y(b_x, b_y, b_z, scale, angle, image_size, b_image_x, b_image_y);

                // return a_image_y < b_image_y;
                // double dist_a = sqrt(pow(a_image_x, 2) + pow(a_image_y-image_size, 2));
                // double dist_b = sqrt(pow(b_image_x, 2) + pow(b_image_y-image_size, 2));
                // return dist_a > dist_b;

                /*
                xz closeness to viewer
                y height

                */

                double dist_a, dist_b;
                if (angle == IsoAngle::NE) { // x+ z- to front
                    double max_x = std::max(a->position_double->x, b->position_double->x);
                    double min_z = std::min(a->position_double->z, b->position_double->z);
                    double max_coor = std::max(abs(max_x), abs(min_z));

                    dist_a = abs(max_coor - a->position_double->x) + abs(-max_coor - a->position_double->z);
                    dist_b = abs(max_coor - b->position_double->x) + abs(-max_coor - b->position_double->z);
                }
                else if (angle == IsoAngle::SE) {  // x+ z+ to front
                    double max_x = std::max(a->position_double->x, b->position_double->x);
                    double max_z = std::max(a->position_double->z, b->position_double->z);
                    double max_coor = std::max(max_x, max_z);

                    dist_a = abs(max_coor - a->position_double->x) + abs(max_coor - a->position_double->z);
                    dist_b = abs(max_coor - b->position_double->x) + abs(max_coor - b->position_double->z);
                }
                else if (angle == IsoAngle::SW) {  // x- z+ to front
                    double min_x = std::min(a->position_double->x, b->position_double->x);
                    double max_z = std::max(a->position_double->z, b->position_double->z);
                    double max_coor = std::max(abs(min_x), abs(max_z));

                    dist_a = abs(-max_coor - a->position_double->x) + abs(max_coor - a->position_double->z);
                    dist_b = abs(-max_coor - b->position_double->x) + abs(max_coor - b->position_double->z);

                }
                else if (angle == IsoAngle::NW) {  // x- z- to front
                    double min_x = std::min(a->position_double->x, b->position_double->x);
                    double min_z = std::min(a->position_double->z, b->position_double->z);
                    double min_coor = std::min(min_x, min_z);

                    dist_a = abs(min_coor - a->position_double->x) + abs(min_coor - a->position_double->z);
                    dist_b = abs(min_coor - b->position_double->x) + abs(min_coor - b->position_double->z);
                }


                // handle transparency
                // XXX: test this, probably wrong
                // bool is_transparent_a = textures_rgba[b->material][3] != 255;
                // if (is_transparent_a) dist_a += Util::MAX_DOUBLE;
                // bool is_transparent_b = textures_rgba[b->material][3] != 255;
                // if (is_transparent_b) dist_b += Util::MAX_DOUBLE;

                 
                if (Util::equals(dist_a, dist_b)) {
                    return a->position_double->y > b->position_double->y;
                }
                return dist_a <= dist_b; // if a is closer, it should come first

            }
        );


        // for each block get the image location and render
        for (shared_ptr<Block> block : unique_blocks) {

            double x = block->position_double->x - image_bot_left_corner_x;
            double y = block->position_double->y;
            double z = block->position_double->z - image_bot_left_corner_z;
            int image_x, image_y;
            IsometricRenderer::block_x_y_z_to_image_x_y(x, y, z, scale, angle, image_size, image_x, image_y);

            string xy_tag = m_xy_tag(block);
            shared_ptr<Block> block_in_same_spot = top_blocks[xy_tag];
            bool has_block_above = block_in_same_spot != nullptr && block_in_same_spot->position_double->y > block->position_double->y;

            uint8_t* texture;
            if (textures_rgba.find(block->material) == textures_rgba.end()) {
                texture = textures_rgba[""];
            }
            else {
                texture = textures_rgba[block->material];
            }

            // apply block sides to image
            apply_texture_to_image(
                image,
                image_x,
                image_y,
                scale,
                block->size,
                BlockSide::TOP,
                image_size,
                texture,
                IsometricRenderer::IMAGE_TEXTURE_SIZE,
                background,
                has_block_above
            );
            apply_texture_to_image(
                image,
                image_x,
                image_y,
                scale,
                block->size,
                BlockSide::RIGHT,
                image_size,
                texture,
                IsometricRenderer::IMAGE_TEXTURE_SIZE,
                background,
                has_block_above
            );
            apply_texture_to_image(
                image,
                image_x,
                image_y,
                scale,
                block->size,
                BlockSide::BACK,
                image_size,
                texture,
                IsometricRenderer::IMAGE_TEXTURE_SIZE,
                background,
                has_block_above
            );
            // if (has_block_above) {
            // stbi_write_png("src/TEST/output/NE.png", image_size, image_size, 4, image, 0);
            // cout << xy_tag << endl;
            // }
        }

        return image;
    }

    static void set_position_double_on_blocks(vector<shared_ptr<Block>>& blocks) {
        for(shared_ptr<Block> block : blocks) {
            if (block->position_double == nullptr) {
                block->position_double = block->position->to_position_double();
            }
        }
    }


    static void render_s(vector<shared_ptr<Block>> blocks, string path, IsoAngle angle, double scale=16.0) {

        unordered_map<string, uint8_t*> textures;
        int width, height, n;
        auto load_texture = [&](const string& material) {
            if (textures.find(material) == textures.end()) {
                string texture_path = BMaterial::path(material);
                if (!std::filesystem::exists(texture_path)) return;

                uint8_t* texture = stbi_load(texture_path.c_str(), &width, &height, &n, 4);
                if (texture != nullptr) textures[material] = texture;
            }
        };
        for (const string& material : BMaterial::leaves) load_texture(material);
        for (const string& material : BMaterial::wood) load_texture(material);
        IsometricRenderer::set_position_double_on_blocks(blocks);
        IsometricRenderer::render_and_save_png(blocks, textures, angle, path, scale);
    }


    // util

    static void block_x_y_z_to_image_x_y(
        double x_from_bot_left_corner,
        double y, 
        double z_from_bot_left_corner,
        double scale,
        IsoAngle angle,
        int image_size,
        int& image_x, 
        int& image_y
    ) {
        x_from_bot_left_corner *= scale;
        y *= scale;
        z_from_bot_left_corner *= scale;

        if (angle == IsoAngle::NE) { // + +
            image_x = round(0.5*x_from_bot_left_corner + 0.5*z_from_bot_left_corner);
            image_y = round(0.25*x_from_bot_left_corner - 0.25*z_from_bot_left_corner - 0.5*y);
        }
        else if (angle == IsoAngle::SE) {  // + -
            image_x = round(-0.5*x_from_bot_left_corner + 0.5*z_from_bot_left_corner);
            image_y = round(0.25*x_from_bot_left_corner + 0.25*z_from_bot_left_corner - 0.5*y);
        }
        else if (angle == IsoAngle::SW) {  // - -
            image_x = round(-0.5*x_from_bot_left_corner - 0.5*z_from_bot_left_corner);
            image_y = round(-0.25*x_from_bot_left_corner + 0.25*z_from_bot_left_corner - 0.5*y);
        }
        else if (angle == IsoAngle::NW) {  // - +
            image_x = round(0.5*x_from_bot_left_corner - 0.5*z_from_bot_left_corner);
            image_y = round(-0.25*x_from_bot_left_corner - 0.25*z_from_bot_left_corner - 0.5*y);
        }

        image_y = image_size + image_y - 1;

    }

    static void apply_texture_to_image(
        uint8_t* image, 
        int image_x,
        int image_y,
        double scale,
        double block_size,
        BlockSide block_side,
        int image_size,
        uint8_t* texture,
        int image_texture_size,
        uint8_t* background,
        bool has_block_above
    ) {

        int start_index = pixel_index(image_x, image_y, image_size);
        

        int start_x, start_y, start_z, end_x, end_y, end_z;
        int block_size_pixels = block_size * scale;
        if (block_side == BlockSide::TOP) {
            start_x = 0;
            start_y = block_size_pixels-1;
            start_z = 0;
            end_x = block_size_pixels;
            end_y = block_size_pixels;
            end_z = block_size_pixels;
        }
        else if (block_side == BlockSide::RIGHT) {
            start_x = block_size_pixels-1;
            start_y = 0;
            start_z = 0;
            end_x = block_size_pixels;
            end_y = block_size_pixels;
            end_z = block_size_pixels;
        }
        else if (block_side == BlockSide::BACK) {
            start_x = 0;
            start_y = 0;
            start_z = 0;
            end_x = block_size_pixels;
            end_y = block_size_pixels;
            end_z = 1;
        }

        double x_range = abs(end_x - start_x);
        double y_range = abs(end_y - start_y);
        double z_range = abs(end_z - start_z);
        
        for(double x_p = start_x; x_p < end_x; ++x_p) {
            for(double y_p = start_y; y_p < end_y; ++y_p) {
                for(double z_p = start_z; z_p < end_z; ++z_p) {

                    // get offset from block pixel
                    double offset_x, offset_y;
                    pixel_smaple_x_y_z_to_image_x_y(x_p, y_p, z_p, offset_x, offset_y, scale);
                    int pixel_x = image_x+offset_x;
                    int pixel_y = image_y+offset_y;
                    if (pixel_x >= 0 && pixel_x < image_size && pixel_y >= 0 && pixel_y < image_size) {
                        int64_t image_index = pixel_index(pixel_x, pixel_y, image_size);

                        // map to pixel from texture
                        int tex_index;
                        if (block_side == BlockSide::TOP) {
                            tex_index = pixel_index(
                                image_texture_size * abs(x_p-start_x) / x_range, 
                                image_texture_size * abs(z_p-start_z) / z_range, 
                                image_texture_size
                            );
                        }
                        else if (block_side == BlockSide::RIGHT) {
                            tex_index = pixel_index(
                                image_texture_size * abs(z_p-start_z) / z_range, 
                                image_texture_size * abs(y_p-start_y) / y_range, 
                                image_texture_size
                            );
                        }
                        else if (block_side == BlockSide::BACK) {
                            tex_index = pixel_index(
                                image_texture_size * abs(x_p-start_x) / x_range, 
                                image_texture_size * abs(y_p-start_y) / y_range, 
                                image_texture_size
                            );
                        }


                        // only render if necessary
                        bool current_is_background = image[image_index] == background[0] && 
                            image[image_index+1] == background[1] && 
                            image[image_index+2] == background[2] && 
                            image[image_index+3] == background[3];
                        if (current_is_background) {


                            // apply dimness for sides of block
                            double side_mult = 1;
                            if (block_side == BlockSide::RIGHT) {
                                side_mult = 0.6;
                            }
                            else if (block_side == BlockSide::BACK) {
                                side_mult = 0.8;
                            }
                            else if (block_side == BlockSide::TOP && has_block_above) {
                                side_mult = 0.6;
                            }

                            // alpha composite onto the image
                            uint8_t r, g, b, a;
                            alpha_composite(
                                image[image_index],
                                image[image_index+1],
                                image[image_index+2],
                                image[image_index+3],
                                texture[tex_index] * side_mult,
                                texture[tex_index+1] * side_mult,
                                texture[tex_index+2] * side_mult,
                                texture[tex_index+3],
                                r,
                                g,
                                b,
                                a
                            );
                            image[image_index] = r;
                            image[image_index+1] = g;
                            image[image_index+2] = b;
                            image[image_index+3] = a;
                        }



                    }
                }
            }
        }
    }

    static void pixel_smaple_x_y_z_to_image_x_y(double x, double y, double z, double& image_x, double& image_y, double scale) {
        /*

        slope x = 1/2
        slope z = -1/2


        image_x = 1/2x + 1/2z
        image_y = -1/4x + 11 + 1/4z - 1/2y

        TESTS:
        - 15,8,0
            image_x = 7.5
            image_y = -7.5 + 11 - 4

        - 0,0,0
            image_x = 0
            image_y = 11
        - 16,0,0
            image_x = 0
            image_y = 3
        - 16,0,16
            image_x = 16
            image_y = 11
        - 0,0,16
            image_x = 8
            image_y = 15
        */
        
        image_x = 0.5*x + 0.5*z;
        image_y = 0.25*x - 0.25*z - 0.5*y;

    }

    /*
        Combines two colors with transparency. 
    */
    static void alpha_composite(
        uint8_t r_background, 
        uint8_t g_backround, 
        uint8_t b_background, 
        uint8_t a_background, 
        uint8_t r_new, 
        uint8_t g_new, 
        uint8_t b_new, 
        uint8_t a_new,
        uint8_t& r,
        uint8_t& g,
        uint8_t& b,
        uint8_t& a
    ) {
        
        // Calculate the resulting alpha
        a = a_new + a_background * (255 - a_new) / 255;

        r = (r_new * a_new + r_background * a_background * (255 - a_new)) / a;
        g = (g_new * a_new + g_backround * a_background * (255 - a_new)) / a;
        b = (b_new * a_new + b_background * a_background * (255 - a_new)) / a;
    }

    static int64_t pixel_index(int x, int y, int64_t image_width) {
        return (y * image_width + x) * 4;
    }

    static string m_xy_tag(shared_ptr<Block> block) {
        stringstream ss;
        ss << (int)(block->position_double->x/block->size);
        ss << " ";
        ss << (int)(block->position_double->z/block->size);

        return ss.str();
    }

    static uint8_t* make_background_image(uint8_t* background, int64_t image_size) {


        uint8_t* image = new uint8_t[image_size * image_size * 4]{0};

        // apply background
        if (background[0] != 0 || background[1] != 0 || background[2] != 0 || background[3] != 0) {
            for (int64_t i = 0; i < image_size * image_size * 4; i+=4) {
                image[i] = background[0];
                image[i+1] = background[1];
                image[i+2] = background[2];
                image[i+3] = background[3];
            }
        }

        return image;
    }


    static void get_range_stats(vector<shared_ptr<Block>> blocks, double& min_range, double& min_y, double& avg_x, double& avg_y, double& avg_z) {

        double min_x = blocks[0]->position_double->x;
        double max_x = blocks[0]->position_double->x;
        min_y = blocks[0]->position_double->y;
        double max_y = blocks[0]->position_double->y;
        double min_z = blocks[0]->position_double->z;
        double max_z = blocks[0]->position_double->z;
        
        avg_x = 0;
        avg_y = 0;
        avg_z = 0;
        for (shared_ptr<Block> block : blocks) {
            if (block->position_double->x < min_x) {
                min_x = block->position_double->x;
            }
            if (block->position_double->x > max_x) {
                max_x = block->position_double->x;
            }
            if (block->position_double->y < min_y) {
                min_y = block->position_double->y;
            }
            if (block->position_double->y > max_y) {
                max_y = block->position_double->y;
            }
            if (block->position_double->z < min_z) {
                min_z = block->position_double->z;
            }
            if (block->position_double->z > max_z) {
                max_z = block->position_double->z;
            }

            avg_x += block->position_double->x;
            avg_y += block->position_double->y;
            avg_z += block->position_double->z;
        }
        avg_x /= blocks.size();
        avg_y /= blocks.size();
        avg_z /= blocks.size();

        // add buffer
        max_x += 1;
        max_y += 1;
        max_z += 1;
        min_x -= 1;
        min_y -= 1;
        min_z -= 1;

        double x_range = max(max_x - min_x, 1.0);
        double y_range = max(max_y - min_y, 1.0);
        double z_range = max(max_z - min_z, 1.0);
        double horizontal_range = 0.5 * x_range + 0.5 * z_range;
        double vertical_range = 0.5 * x_range + 0.5 * z_range + y_range;
        min_range = max(horizontal_range, vertical_range/2); // vertical is divided by 2 since iso view is 2x as tall as wide block wise

    }

    

    static void scale_to_fit_in_small_image(vector<shared_ptr<Block>>& blocks) {

        double min_range, min_y, avg_x, avg_y, avg_z;
        get_range_stats(blocks, min_range, min_y, avg_x, avg_y, avg_z);

        double block_size = blocks[0]->size;
        // int64_t square_blocks = (min_range / block_size) * 2;

        int64_t GOAL_DESIRED_RANGE = 200;
        if (min_range > GOAL_DESIRED_RANGE) {
            double scale = GOAL_DESIRED_RANGE / min_range;

            for (shared_ptr<Block> block : blocks) {
                block->position_double->x *= scale;
                block->position_double->y *= scale;
                block->position_double->z *= scale;
            }
        }
    }
};
